// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include "async_result.h"
#include "network_socket.h"
#include <daw/daw_span.h>

#include <functional>
#include <memory>
#include <string_view>

namespace daw::networking {
	struct shared_tcp_client;

	class unique_tcp_client {
		std::unique_ptr<network_socket> m_socket;

		friend class ::daw::networking::shared_tcp_client;

	public:
		unique_tcp_client( );
		unique_tcp_client( std::string_view host, std::uint16_t port );

		void connect( std::string_view host, std::uint16_t port );
		void close( );

		async_result<void> connect_async( std::string_view host,
		                                  std::uint16_t port );

		async_result<void> connect_async( std::string_view host, std::uint16_t port,
		                                  std::function<void( )> on_completion );

		async_result<void> close_async( );

		std::size_t write( daw::span<char const> buffer );
		async_result<void> write_async( daw::span<char const> buffer );
		async_result<void> write_async(
		  daw::span<char const> buffer,
		  std::function<std::optional<daw::span<char const>>( daw::span<char const>,
		                                                      std::size_t )>
		    on_completion );
		std::size_t read( daw::span<char> buffer );
		async_result<std::size_t> read_async( daw::span<char> buffer );
		async_result<void>
		read_async( daw::span<char> buffer,
		            std::function<std::optional<daw::span<char>>( daw::span<char>,
		                                                          std::size_t )>
		              on_completion );
	};

	class shared_tcp_client {
		std::shared_ptr<network_socket> m_socket;

	public:
		shared_tcp_client( );
		shared_tcp_client( std::string_view host, std::uint16_t port );
		explicit shared_tcp_client( unique_tcp_client &&other );

		void connect( std::string_view host, std::uint16_t port );
		void close( );

		async_result<void> connect_async( std::string_view host,
		                                  std::uint16_t port );

		async_result<void> connect_async( std::string_view host, std::uint16_t port,
		                                  std::function<void( )> on_completion );

		async_result<void> close_async( );

		std::size_t write( daw::span<char const> buffer );
		async_result<void> write_async( daw::span<char const> buffer );
		async_result<void> write_async(
			daw::span<char const> buffer,
			std::function<std::optional<daw::span<char const>>( daw::span<char const>,
																													std::size_t )>
			on_completion );
		std::size_t read( daw::span<char> buffer );
		async_result<std::size_t> read_async( daw::span<char> buffer );
		async_result<void>
		read_async( daw::span<char> buffer,
		            std::function<std::optional<daw::span<char>>( daw::span<char>,
		                                                          std::size_t )>
		              on_completion );
	};

	inline unique_tcp_client &operator<<( unique_tcp_client &client,
	                                      std::string_view message ) {
		client.write( { message.data( ), message.size( ) } );
		return client;
	}

	inline shared_tcp_client &operator<<( shared_tcp_client &client,
	                                      std::string_view message ) {
		client.write( { message.data( ), message.size( ) } );
		return client;
	}

} // namespace daw::networking
