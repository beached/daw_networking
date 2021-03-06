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
#include <daw/daw_span.h>
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

namespace daw::networking {
	enum class socket_types : int {
		Stream = SOCK_STREAM,
		Dgram = SOCK_DGRAM,
		Raw = SOCK_RAW
	};

	enum class address_family : int {
		Unspecified = AF_UNSPEC,
		Unix = AF_UNIX,
		IPv4 = AF_INET,
		IPv6 = AF_INET6
	};

	enum class shutdown_how : int {
		DisallowReceive = 0,
		DisallowSend = 1,
		DisallowSendReceive = 2
	};

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

	template<typename ExecPolicy>
	struct basic_network_socket {
		using async_exec_policy = ExecPolicy;
		async_exec_policy m_exec{ };
		std::mutex m_mutex{ };
		int m_socket = -1;
		address_family m_family;
		socket_types m_socket_type;
		void connect_impl( std::string host, std::uint16_t port );

	public:
		basic_network_socket( address_family af, socket_types st );
		void connect( std::string_view host, std::uint16_t port );
		void close( );
		int shutdown( shutdown_how how );

		bool is_open( ) const {
			auto const lck = std::unique_lock( m_mutex );
			return m_socket >= 0;
		}

		bool is_open_no_lock( ) const {
			return m_socket >= 0;
		}

		[[nodiscard]] async_result<void> connect_async( std::string_view host,
		                                                std::uint16_t port );

		[[nodiscard]] async_result<void>
		connect_async( std::string_view host, std::uint16_t port,
		               std::function<void( )> on_completion );

		[[nodiscard]] async_result<void> close_async( );

		[[nodiscard]] std::size_t send( daw::span<char const> buffer,
		                                int flags = 0 );

		[[nodiscard]] async_result<void> send_async( daw::span<char const> buffer,
		                                             int flags = 0 );

		[[nodiscard]] std::size_t receive( daw::span<char> buffer, int flags = 0 );

		[[nodiscard]] async_result<std::size_t>
		receive_async( daw::span<char> buffer, int flags = 0 );

		async_result<void> receive_async(
		  daw::span<char> buffer,
		  std::function<std::optional<daw::span<char>>( daw::span<char>,
		                                                std::size_t )>
		    on_completion,
		  int flags = 0 );

		async_result<void> send_async(
		  daw::span<char const> buffer,
		  std::function<std::optional<daw::span<char const>>( daw::span<char const>,
		                                                      std::size_t )>
		    on_completion,
		  int flags = 0 );
	};

	using network_socket = basic_network_socket<async_exec_policy_thread>;

	template<typename ExecPolicy>
	void basic_network_socket<ExecPolicy>::connect_impl( std::string host,
	                                                     std::uint16_t port ) {
		auto const port_str = std::to_string( port );
		auto hints = ::addrinfo( );
		hints.ai_family = static_cast<int>( m_family );
		hints.ai_socktype = static_cast<int>( m_socket_type );
		auto res = address_info( );
		if( getaddrinfo( host.c_str( ), port_str.c_str( ), &hints,
		                 &res.m_addresses ) < 0 ) {
			throw network_exception( "Error resolving addresses", errno );
		}
		m_socket = ::socket( res->ai_family, res->ai_socktype, res->ai_protocol );
		if( m_socket < 0 ) {
			throw network_exception( "Error creating socket", errno );
		}

		if( ::connect( m_socket, res->ai_addr, res->ai_addrlen ) < 0 ) {
			(void)::close( m_socket );
			throw network_exception( "error connecting", errno );
		}
	}

	template<typename ExecPolicy>
	void basic_network_socket<ExecPolicy>::connect( std::string_view host,
	                                                std::uint16_t port ) {
		auto const lck = std::unique_lock( m_mutex );
		m_exec.wait( );
		daw::exception::dbg_precondition_check( not is_open_no_lock( ),
		                                        "Expecting disconnected socket" );
		connect_impl( static_cast<std::string>( host ), port );
	}

	template<typename ExecPolicy>
	async_result<void>
	basic_network_socket<ExecPolicy>::connect_async( std::string_view host,
	                                                 std::uint16_t port ) {

		auto state = std::make_shared<async_result_state<void>>( );
		m_exec.add_task(
		  [&, host = static_cast<std::string>( host ), port, state]( ) noexcept {
			  try {
				  daw::exception::dbg_precondition_check(
				    not is_open_no_lock( ), "Expecting disconnected socket" );
				  connect_impl( host, port );
			  } catch( ... ) { state->set_exception( ); }
			  state->set_value( );
		  } );
		return async_result<void>( std::move( state ) );
	}

	template<typename ExecPolicy>
	async_result<void> basic_network_socket<ExecPolicy>::connect_async(
	  std::string_view host, std::uint16_t port,
	  std::function<void( )> on_completion ) {

		auto state = std::make_shared<async_result_state<void>>( );
		m_exec.add_task( [&, host = static_cast<std::string>( host ), port, state,
		                  on_completion = daw::mutable_capture(
		                    std::move( on_completion ) )]( ) noexcept {
			try {
				daw::exception::dbg_precondition_check(
				  not is_open_no_lock( ), "Expecting disconnected socket" );
				connect_impl( host, port );
				( std::move( *on_completion ) )( );
			} catch( ... ) { state->set_exception( ); }
			state->set_value( );
		} );
		return async_result<void>( std::move( state ) );
	}

	template<typename ExecPolicy>
	void basic_network_socket<ExecPolicy>::close( ) {
		auto const lck = std::unique_lock( m_mutex );
		m_exec.wait( );
		daw::exception::dbg_precondition_check( is_open_no_lock( ),
		                                        "Expecting connected socket" );
		::close( m_socket );
		m_socket = -1;
	}

	template<typename ExecPolicy>
	async_result<void> basic_network_socket<ExecPolicy>::close_async( ) {
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

	template<typename ExecPolicy>
	basic_network_socket<ExecPolicy>::basic_network_socket( address_family af,
	                                                        socket_types st )
	  : m_family( af )
	  , m_socket_type( st ) {}

	template<typename ExecPolicy>
	std::size_t
	basic_network_socket<ExecPolicy>::send( daw::span<const char> buffer,
	                                        int flags ) {
		auto const lck = std::unique_lock( m_mutex );
		daw::exception::dbg_precondition_check( is_open_no_lock( ),
		                                        "Expecting connected socket" );
		m_exec.wait( );
		auto result = ::send( m_socket, buffer.data( ), buffer.size( ), flags );
		if( result < 0 ) {
			throw network_exception{ "send error", errno };
		}
		return static_cast<std::size_t>( result );
	}

	// TODO: account for return 0, closure
	template<typename ExecPolicy>
	async_result<void>
	basic_network_socket<ExecPolicy>::send_async( daw::span<const char> buffer,
	                                              int flags ) {
		auto const lck = std::unique_lock( m_mutex );
		auto state = std::make_shared<async_result_state<void>>( );

		m_exec.add_task(
		  [&, buffer = daw::mutable_capture( buffer ), state, flags]( ) noexcept {
			  daw::exception::dbg_precondition_check( is_open_no_lock( ),
			                                          "Expecting connected socket" );
			  std::size_t const expected_total = buffer->size( );
			  while( not buffer->empty( ) ) {
				  auto r = ::send( m_socket, buffer->data( ), buffer->size( ), flags );
				  if( r < 0 ) {
					  state->set_exception( std::make_exception_ptr(
					    network_exception{ "send error", errno } ) );
					  return;
				  }
				  buffer->remove_prefix( r );
			  }
			  state->set_value( );
		  } );
		return { std::move( state ) };
	}

	template<typename ExecPolicy>
	async_result<void> basic_network_socket<ExecPolicy>::send_async(
	  daw::span<char const> buffer,
	  std::function<std::optional<daw::span<char const>>( daw::span<char const>,
	                                                      std::size_t )>
	    on_completion,
	  int flags ) {
		auto const lck = std::unique_lock( m_mutex );
		auto state = std::make_shared<async_result_state<void>>( );

		m_exec.add_task(
		  [&, buff = daw::mutable_capture( buffer ),
		   on_completion = daw::mutable_capture( std::move( on_completion ) ),
		   state, flags]( ) noexcept {
			  daw::exception::dbg_precondition_check( is_open_no_lock( ),
			                                          "Expecting connected socket" );
			  daw::span<char const> buffer = *buff;
			  std::size_t const expected_total = buffer.size( );
			  ::ssize_t r = -1;
			  auto on_completion_result = std::optional<daw::span<char const>>( );
			  do {
				  auto r = ::send( m_socket, buffer.data( ), buffer.size( ), flags );
				  if( r < 0 ) {
					  state->set_exception( std::make_exception_ptr(
					    network_exception{ "send error", errno } ) );
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

	template<typename ExecPolicy>
	std::size_t basic_network_socket<ExecPolicy>::receive( daw::span<char> buffer,
	                                                       int flags ) {
		auto const lck = std::unique_lock( m_mutex );
		m_exec.wait( );
		daw::exception::dbg_precondition_check( is_open_no_lock( ),
		                                        "Expecting connected socket" );
		auto result = ::recv( m_socket, buffer.data( ), buffer.size( ), flags );
		if( result < 0 ) {
			throw network_exception{ "receive error", errno };
		}
		return static_cast<std::size_t>( result );
	}

	template<typename ExecPolicy>
	async_result<std::size_t>
	basic_network_socket<ExecPolicy>::receive_async( daw::span<char> buffer,
	                                                 int flags ) {
		auto const lck = std::unique_lock( m_mutex );
		auto state = std::make_shared<async_result_state<std::size_t>>( );

		m_exec.add_task(
		  [&, buffer = daw::mutable_capture( buffer ), state, flags]( ) noexcept {
			  daw::exception::dbg_precondition_check( is_open_no_lock( ),
			                                          "Expecting connected socket" );
			  std::size_t const expected_total = buffer->size( );
			  ::ssize_t r = 1;
			  std::size_t total = 0;
			  while( r > 0 and not buffer->empty( ) ) {
				  auto r = ::recv( m_socket, buffer->data( ), buffer->size( ), flags );
				  if( r < 0 ) {
					  state->set_exception( std::make_exception_ptr(
					    network_exception{ "receive error", errno } ) );
					  return;
				  }
				  total += static_cast<std::size_t>( r );
				  buffer->remove_prefix( static_cast<std::size_t>( r ) );
			  }
			  state->set_value( total );
		  } );
		return { std::move( state ) };
	}

	template<typename ExecPolicy>
	async_result<void> basic_network_socket<ExecPolicy>::receive_async(
	  daw::span<char> buffer,
	  std::function<std::optional<daw::span<char>>( daw::span<char>,
	                                                std::size_t )>
	    on_completion,
	  int flags ) {
		auto const lck = std::unique_lock( m_mutex );
		auto state = std::make_shared<async_result_state<void>>( );

		m_exec.add_task(
		  [&, buff = daw::mutable_capture( buffer ),
		   on_completion = daw::mutable_capture( std::move( on_completion ) ),
		   state, flags]( ) noexcept {
			  daw::exception::dbg_precondition_check( is_open_no_lock( ),
			                                          "Expecting connected socket" );
			  daw::span<char> buffer = *buff;
			  std::size_t const expected_total = buffer.size( );
			  ::ssize_t r = -1;
			  auto on_completion_result = std::optional<daw::span<char>>( );
			  do {
				  auto r = ::recv( m_socket, buffer.data( ), buffer.size( ), flags );
				  if( r < 0 ) {
					  state->set_exception( std::make_exception_ptr(
					    network_exception{ "receive error", errno } ) );
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

	template<typename ExecPolicy>
	int basic_network_socket<ExecPolicy>::shutdown( shutdown_how how ) {
		return ::shutdown( m_socket, static_cast<int>( how ) );
	}

} // namespace daw::networking
