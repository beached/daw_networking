// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "daw/networking/tcp_client.h"
#include "daw/networking/network_socket.h"

#include <memory>
#include <string_view>

namespace daw::networking {
	unique_tcp_client::unique_tcp_client( )
	  : m_socket( std::make_unique<network_socket>( address_family::Unspecified,
	                                               socket_types::Stream ) ) {}

	shared_tcp_client::shared_tcp_client( )
	  : m_socket( std::make_shared<network_socket>( address_family::Unspecified,
	                                               socket_types::Stream ) ) {}

	unique_tcp_client::unique_tcp_client( std::string_view host,
	                                      std::uint16_t port )
	  : unique_tcp_client( ) {

		m_socket->connect( host, port );
	}

	shared_tcp_client::shared_tcp_client( std::string_view host,
	                                      std::uint16_t port )
	  : shared_tcp_client( ) {

		m_socket->connect( host, port );
	}

	void unique_tcp_client::connect( std::string_view host, std::uint16_t port ) {
		m_socket->connect( host, port );
	}

	void shared_tcp_client::connect( std::string_view host, std::uint16_t port ) {
		m_socket->connect( host, port );
	}

	void unique_tcp_client::close( ) {
		m_socket->close( );
	}

	void shared_tcp_client::close( ) {
		m_socket->close( );
	}

	shared_tcp_client::shared_tcp_client( unique_tcp_client &&other )
	  : m_socket( other.m_socket.release( ) ) {}

	async_result<void> unique_tcp_client::connect_async( std::string_view host,
	                                                     std::uint16_t port ) {
		return m_socket->connect_async( host, port );
	}

	async_result<void>
	unique_tcp_client::connect_async( std::string_view host, std::uint16_t port,
	                                  std::function<void( )> on_completion ) {

		return m_socket->connect_async( host, port, std::move( on_completion ) );
	}

	async_result<void> unique_tcp_client::close_async( ) {
		return m_socket->close_async( );
	}

	std::size_t unique_tcp_client::write( daw::span<const char> buffer ) {
		return m_socket->send( buffer );
	}

	std::size_t unique_tcp_client::read( daw::span<char> buffer ) {
		return m_socket->receive( buffer );
	}

	async_result<void>
	unique_tcp_client::write_async( daw::span<const char> buffer ) {
		return m_socket->send_async( buffer );
	}

	async_result<std::size_t>
	unique_tcp_client::read_async( daw::span<char> buffer ) {
		return m_socket->receive_async( buffer );
	}

	async_result<void> unique_tcp_client::read_async(
	  daw::span<char> buffer,
	  std::function<std::optional<daw::span<char>>( daw::span<char>,
	                                                std::size_t )>
	    on_completion ) {
		return m_socket->read_async( buffer, std::move( on_completion ) );
	}

	std::size_t shared_tcp_client::write( daw::span<const char> buffer ) {
		return m_socket->send( buffer );
	}

	std::size_t shared_tcp_client::read( daw::span<char> buffer ) {
		return m_socket->receive( buffer );
	}

	async_result<void>
	shared_tcp_client::write_async( daw::span<const char> buffer ) {
		return m_socket->send_async( buffer );
	}

	async_result<std::size_t>
	shared_tcp_client::read_async( daw::span<char> buffer ) {
		return m_socket->receive_async( buffer );
	}

	async_result<void> shared_tcp_client::read_async(
	  daw::span<char> buffer,
	  std::function<std::optional<daw::span<char>>( daw::span<char>,
	                                                std::size_t )>
	    on_completion ) {
		return m_socket->read_async( buffer, std::move( on_completion ) );
	}

	async_result<void> shared_tcp_client::close_async( ) {
		return m_socket->close_async( );
	}

	async_result<void> shared_tcp_client::connect_async( std::string_view host,
	                                                     std::uint16_t port ) {
		return m_socket->connect_async( host, port );
	}

	async_result<void>
	shared_tcp_client::connect_async( std::string_view host, std::uint16_t port,
	                                  std::function<void( )> on_completion ) {

		return m_socket->connect_async( host, port, std::move( on_completion ) );
	}
} // namespace daw::networking
