// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <cstddef>
#include <exception>
#include <string>
#include <string_view>
#include <utility>

namespace daw::networking {
	class network_exception : std::exception {
		std::string m_reason;
		long long m_error_code;

	public:
		inline network_exception( std::string reason, long long error_code )
		  : m_reason( std::move( reason ) )
		  , m_error_code( error_code ) {}

		inline long long error_code( ) const {
			return m_error_code;
		}
	};
} // namespace daw::networking
