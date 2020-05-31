// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include "../../../third_party/jthread.hpp"
#include "../async_exec_policy_thread.h"
#include "../async_result.h"
#include "../network_exception.h"

#include <daw/daw_exception.h>
#include <daw/parallel/daw_shared_mutex.h>

#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <mutex>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace daw::networking::tcp_client_details {
	struct address_info {
		::addrinfo *m_addresses = nullptr;

		~address_info( ) noexcept {
			reset( );
		}

		address_info( address_info const & ) = delete;
		address_info &operator=( address_info const & ) = delete;

		constexpr address_info( address_info &&other ) noexcept
		  : m_addresses( daw::exchange( other.m_addresses, nullptr ) ) {}

		constexpr address_info &operator=( address_info &&rhs ) noexcept {
			if( this != &rhs ) {
				reset( daw::exchange( rhs.m_addresses, nullptr ) );
			}
			return *this;
		}

		constexpr address_info( ) noexcept = default;

		constexpr void reset( ) noexcept {
			if( m_addresses ) {
				::freeaddrinfo( m_addresses );
			}
			m_addresses = nullptr;
		}

		constexpr void reset( addrinfo *ai ) noexcept {
			reset( );
			m_addresses = ai;
		}

		::addrinfo const *operator->( ) const noexcept {
			return m_addresses;
		}

		::addrinfo *operator->( ) noexcept {
			return m_addresses;
		}
	};

	struct tcp_client_state {
		using async_exec_policy = async_exec_policy_thread;
		int m_socket = -1;
		mutable daw::shared_mutex m_mutex{ };
		async_exec_policy m_exec{ };

		tcp_client_state( ) = default;
		inline void connect( std::string_view host, std::uint16_t port );
		inline void close( );

		inline bool is_open( ) const {
			auto const lck = std::unique_lock( m_mutex );
			return m_socket >= 0;
		}

		inline bool is_open_no_lock( ) const {
			return m_socket >= 0;
		}

		[[nodiscard]] inline async_result<void>
		connect_async( std::string_view host, std::uint16_t port );

		[[nodiscard]] inline async_result<void>
		connect_async( std::string_view host, std::uint16_t port,
		               std::function<void( )> on_completion );

		[[nodiscard]] inline async_result<void> close_async( );
	};

	inline static void connect_impl( int &socket, std::string host,
	                                 std::uint16_t port ) {
		auto const port_str = std::to_string( port );
		auto hints = ::addrinfo( );
		hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6, whichever
		hints.ai_socktype = SOCK_STREAM;
		auto res = address_info( );
		if( getaddrinfo( host.c_str( ), port_str.c_str( ), &hints,
		                 &res.m_addresses ) < 0 ) {
			throw network_exception( "Error resolving addresses", errno );
		}
		socket = ::socket( res->ai_family, res->ai_socktype, res->ai_protocol );
		if( socket < 0 ) {
			throw network_exception( "Error creating socket", errno );
		}

		if( ::connect( socket, res->ai_addr, res->ai_addrlen ) < 0 ) {
			(void)::close( socket );
			throw network_exception( "error connecting", errno );
		}
	}

	void tcp_client_state::connect( std::string_view host, std::uint16_t port ) {
		auto const lck = std::unique_lock( m_mutex );
		m_exec.wait( );
		daw::exception::dbg_precondition_check( not is_open_no_lock( ),
		                                        "Expecting disconnected socket" );
		connect_impl( m_socket, static_cast<std::string>( host ), port );
	}

	async_result<void> tcp_client_state::connect_async( std::string_view host,
	                                                    std::uint16_t port ) {

		auto state = std::make_shared<async_result_state<void>>( );
		m_exec.add_task(
		  [&, host = static_cast<std::string>( host ), port, state]( ) noexcept {
			  try {
				  daw::exception::dbg_precondition_check(
				    not is_open_no_lock( ), "Expecting disconnected socket" );
				  connect_impl( m_socket, host, port );
			  } catch( ... ) { state->set_exception( ); }
			  state->set_value( );
		  } );
		return async_result<void>( std::move( state ) );
	}

	async_result<void>
	tcp_client_state::connect_async( std::string_view host, std::uint16_t port,
	                                 std::function<void( )> on_completion ) {

		auto state = std::make_shared<async_result_state<void>>( );
		m_exec.add_task( [&, host = static_cast<std::string>( host ), port, state,
		                  on_completion = daw::mutable_capture(
		                    std::move( on_completion ) )]( ) noexcept {
			try {
				daw::exception::dbg_precondition_check(
				  not is_open_no_lock( ), "Expecting disconnected socket" );
				connect_impl( m_socket, host, port );
				( std::move( *on_completion ) )( );
			} catch( ... ) { state->set_exception( ); }
			state->set_value( );
		} );
		return async_result<void>( std::move( state ) );
	}

	void tcp_client_state::close( ) {
		auto const lck = std::unique_lock( m_mutex );
		m_exec.wait( );
		daw::exception::dbg_precondition_check( is_open_no_lock( ),
		                                        "Expecting connected socket" );
		::close( m_socket );
		m_socket = -1;
	}

	async_result<void> tcp_client_state::close_async( ) {
		auto const lck = std::unique_lock( m_mutex );

		auto state = std::make_shared<async_result_state<void>>( );
		m_exec.add_task( [&, state]( ) noexcept {
			try {
				daw::exception::dbg_precondition_check( is_open_no_lock( ),
				                                        "Expecting connected socket" );
				::close( m_socket );
				m_socket = -1;
			} catch( ... ) { state->set_exception( ); }
			state->set_value( );
		} );
		return async_result<void>( std::move( state ) );
	}

	[[nodiscard]] std::size_t write( tcp_client_state &client,
	                                 daw::span<char const> buffer ) {
		auto const lck = std::unique_lock( client.m_mutex );
		daw::exception::dbg_precondition_check( client.is_open_no_lock( ),
		                                        "Expecting connected socket" );
		client.m_exec.wait( );
		auto result = ::send( client.m_socket, buffer.data( ), buffer.size( ), 0 );
		if( result < 0 ) {
			throw network_exception{ "write error", errno };
		}
		return static_cast<std::size_t>( result );
	}

	[[nodiscard]] async_result<void> write_async( tcp_client_state &client,
	                                              daw::span<char const> buffer ) {
		auto const lck = std::unique_lock( client.m_mutex );
		auto state = std::make_shared<async_result_state<void>>( );

		client.m_exec.add_task( [&client, buffer = daw::mutable_capture( buffer ),
		                         state]( ) noexcept {
			daw::exception::dbg_precondition_check( client.is_open_no_lock( ),
			                                        "Expecting connected socket" );
			std::size_t const expected_total = buffer->size( );
			while( not buffer->empty( ) ) {
				auto r = ::send( client.m_socket, buffer->data( ), buffer->size( ), 0 );
				if( r < 0 ) {
					state->set_exception( std::make_exception_ptr(
					  network_exception{ "write error", errno } ) );
					return;
				}
				buffer->remove_prefix( r );
			}
			state->set_value( );
		} );
		return { std::move( state ) };
	}

	[[nodiscard]] std::size_t read( tcp_client_state &client,
	                                daw::span<char> buffer ) {
		auto const lck = std::unique_lock( client.m_mutex );
		client.m_exec.wait( );
		daw::exception::dbg_precondition_check( client.is_open_no_lock( ),
		                                        "Expecting connected socket" );
		auto result = ::read( client.m_socket, buffer.data( ), buffer.size( ) );
		if( result < 0 ) {
			throw network_exception{ "write error", errno };
		}
		return static_cast<std::size_t>( result );
	}

	[[nodiscard]] async_result<std::size_t> read_async( tcp_client_state &client,
	                                                    daw::span<char> buffer ) {
		auto const lck = std::unique_lock( client.m_mutex );
		auto state = std::make_shared<async_result_state<std::size_t>>( );

		client.m_exec.add_task( [&client, buffer = daw::mutable_capture( buffer ),
		                         state]( ) noexcept {
			daw::exception::dbg_precondition_check( client.is_open_no_lock( ),
			                                        "Expecting connected socket" );
			std::size_t const expected_total = buffer->size( );
			::ssize_t r = 1;
			std::size_t total = 0;
			while( r > 0 and not buffer->empty( ) ) {
				auto r = ::recv( client.m_socket, buffer->data( ), buffer->size( ), 0 );
				if( r < 0 ) {
					state->set_exception( std::make_exception_ptr(
					  network_exception{ "write error", errno } ) );
					return;
				}
				total += static_cast<std::size_t>( r );
				buffer->remove_prefix( static_cast<std::size_t>( r ) );
			}
			state->set_value( total );
		} );
		return { std::move( state ) };
	}

	async_result<void>
	read_async( tcp_client_state &client, daw::span<char> buffer,
	            std::function<std::optional<daw::span<char>>( daw::span<char>,
	                                                          std::size_t )>
	              on_completion ) {
		auto const lck = std::unique_lock( client.m_mutex );
		auto state = std::make_shared<async_result_state<void>>( );

		client.m_exec.add_task(
		  [&client, buff = daw::mutable_capture( buffer ),
		   on_completion = daw::mutable_capture( std::move( on_completion ) ),
		   state]( ) noexcept {
			  daw::exception::dbg_precondition_check( client.is_open_no_lock( ),
			                                          "Expecting connected socket" );
			  daw::span<char> buffer = *buff;
			  std::size_t const expected_total = buffer.size( );
			  ::ssize_t r = -1;
			  auto on_completion_result = std::optional<daw::span<char>>( );
			  do {
				  auto r = ::recv( client.m_socket, buffer.data( ), buffer.size( ), 0 );
				  if( r < 0 ) {
					  state->set_exception( std::make_exception_ptr(
					    network_exception{ "write error", errno } ) );
					  return;
				  }
				  on_completion_result =
				    ( *on_completion )( buffer, static_cast<std::size_t>( r ) );
				  if( on_completion_result ) {
					  buffer = *on_completion_result;
				  }
			  } while( r != 0 and on_completion_result );
			  state->set_value( );
		  } );
		return { std::move( state ) };
	}
} // namespace daw::networking::tcp_client_details
