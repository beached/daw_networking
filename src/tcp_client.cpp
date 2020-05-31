// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "daw/networking/tcp_client.h"
#ifndef _MSC_VER_
#include "daw/networking/details/tcp_client_bsd.h"
#endif

#include <memory>
#include <string_view>

namespace daw::networking {
	unique_tcp_client::unique_tcp_client( )
	  : m_state( std::make_unique<tcp_client_details::tcp_client_state>( ) ) {}

	shared_tcp_client::shared_tcp_client( )
	  : m_state( std::make_shared<tcp_client_details::tcp_client_state>( ) ) {}

	unique_tcp_client::unique_tcp_client( std::string_view host,
	                                      std::uint16_t port )
	  : unique_tcp_client( ) {

		m_state->connect( host, port );
	}

	shared_tcp_client::shared_tcp_client( std::string_view host,
	                                      std::uint16_t port )
	  : shared_tcp_client( ) {

		m_state->connect( host, port );
	}

	unique_tcp_client::~unique_tcp_client( ) = default;
	shared_tcp_client::~shared_tcp_client( ) = default;

	void unique_tcp_client::connect( std::string_view host, std::uint16_t port ) {
		m_state->connect( host, port );
	}

	void shared_tcp_client::connect( std::string_view host, std::uint16_t port ) {
		m_state->connect( host, port );
	}

	void unique_tcp_client::close( ) {
		m_state->close( );
	}

	void shared_tcp_client::close( ) {
		m_state->close( );
	}

	shared_tcp_client::shared_tcp_client( unique_tcp_client &&other )
	  : m_state( other.m_state.release( ) ) {}

	async_result<void> unique_tcp_client::connect_async( std::string_view host,
	                                                     std::uint16_t port ) {
		return m_state->connect_async( host, port );
	}

	async_result<void>
	unique_tcp_client::connect_async( std::string_view host, std::uint16_t port,
	                                  std::function<void( )> on_completion ) {

		return m_state->connect_async( host, port, std::move( on_completion ) );
	}

	async_result<void> unique_tcp_client::close_async( ) {
		return m_state->close_async( );
	}

	std::size_t unique_tcp_client::write( daw::span<const char> buffer ) {
		return tcp_client_details::write( *m_state, buffer );
	}

	std::size_t unique_tcp_client::read( daw::span<char> buffer ) {
		return tcp_client_details::read( *m_state, buffer );
	}

	async_result<void>
	unique_tcp_client::write_async( daw::span<const char> buffer ) {
		return tcp_client_details::write_async( *m_state, buffer );
	}

	async_result<std::size_t>
	unique_tcp_client::read_async( daw::span<char> buffer ) {
		return tcp_client_details::read_async( *m_state, buffer );
	}

	async_result<void> unique_tcp_client::read_async(
	  daw::span<char> buffer,
	  std::function<std::optional<daw::span<char>>( daw::span<char>,
	                                                std::size_t )>
	    on_completion ) {
		return tcp_client_details::read_async( *m_state, buffer,
		                                       std::move( on_completion ) );
	}

	std::size_t shared_tcp_client::write( daw::span<const char> buffer ) {
		return tcp_client_details::write( *m_state, buffer );
	}

	std::size_t shared_tcp_client::read( daw::span<char> buffer ) {
		return tcp_client_details::read( *m_state, buffer );
	}

	async_result<void>
	shared_tcp_client::write_async( daw::span<const char> buffer ) {
		return tcp_client_details::write_async( *m_state, buffer );
	}

	async_result<std::size_t>
	shared_tcp_client::read_async( daw::span<char> buffer ) {
		return tcp_client_details::read_async( *m_state, buffer );
	}

	async_result<void> shared_tcp_client::read_async(
	  daw::span<char> buffer,
	  std::function<std::optional<daw::span<char>>( daw::span<char>,
	                                                std::size_t )>
	    on_completion ) {
		return tcp_client_details::read_async( *m_state, buffer,
		                                       std::move( on_completion ) );
	}

	async_result<void> shared_tcp_client::close_async( ) {
		return m_state->close_async( );
	}

	async_result<void> shared_tcp_client::connect_async( std::string_view host,
	                                                     std::uint16_t port ) {
		return m_state->connect_async( host, port );
	}

	async_result<void>
	shared_tcp_client::connect_async( std::string_view host, std::uint16_t port,
	                                  std::function<void( )> on_completion ) {

		return m_state->connect_async( host, port, std::move( on_completion ) );
	}
} // namespace daw::networking
