// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "daw/networking/tcp_client.h"

#include <iostream>

int main( ) {
	// auto client = daw::networking::unique_tcp_client( "localhost", 10240 );
	auto client = daw::networking::unique_tcp_client( );
	client.connect_async( "localhost", 10240,
	                      [] { std::cout << "Connected\n\n"; } );
	client.write_async(
	  daw::span<char const>( "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" ) );
	std::string message;
	message.resize( 1024 );
	client.read_async(
	  { message.data( ), message.size( ) },
	  [&]( daw::span<char> buff,
	       std::size_t count ) -> std::optional<daw::span<char>> {
		  if( count > 0 ) {
			  auto const old_size = message.size( ) - ( 1024 - count );
			  message.resize( old_size );
			  message.resize( message.size( ) + 1024 );
			  return daw::span<char>( message.data( ) + old_size, 1024 );
		  }
		  if( count == 0 ) {
			  message.resize( message.size( ) - 1024 );
		  }
		  return { };
	  } );
	client.close_async( ).wait( );
	std::cout << message.size( ) << " bytes\n==============\n\n";
	std::cout << message << "\n\n";
}
