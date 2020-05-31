// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include "network_exception.h"
#include "task_token.h"

#include <daw/daw_visit.h>

#include <stdexcept>
#include <type_traits>
#include <variant>

namespace daw {

	template<typename T>
	struct async_result_state {
		task_token token;
		std::variant<std::monostate, T, std::exception_ptr> result{ };

		void set_value( T const &value ) {
			result = value;
			token.set_latch( );
		}

		void set_value( T &&value ) {
			result = std::move( value );
			token.set_latch( );
		}

		void set_exception( std::exception_ptr eptr ) {
			result = eptr;
			token.set_latch( );
		}

		void set_exception( ) {
			set_exception( std::current_exception( ) );
		}
	};

	template<>
	struct async_result_state<void> {
		struct void_value_t {};
		task_token token;
		std::variant<std::monostate, void_value_t, std::exception_ptr> result{ };

		void set_value( ) {
			result = void_value_t( );
			token.set_latch( );
		}

		void set_exception( std::exception_ptr eptr ) {
			result = eptr;
			token.set_latch( );
		}

		void set_exception( ) {
			set_exception( std::current_exception( ) );
		}
	};

	template<typename T>
	class async_result {
		std::shared_ptr<async_result_state<T>> m_state;

	public:
		inline async_result( std::shared_ptr<async_result_state<T>> state ) noexcept
		  : m_state( std::move( state ) ) {}

		[[nodiscard]] bool try_wait( ) const {
			return m_state->token.try_wait( );
		}

		inline void wait( ) const {
			m_state->token.wait( );
		}

		template<typename Rep, typename Period>
		[[nodiscard]] decltype( auto )
		wait_for( std::chrono::duration<Rep, Period> const &rel_time ) const {
			return m_state->token.wait_for( rel_time );
		}

		template<typename Clock, typename Duration>
		[[nodiscard]] decltype( auto ) wait_until(
		  std::chrono::time_point<Clock, Duration> const &timeout_time ) const {
			return m_state->token.wait_until( timeout_time );
		}

		bool is_valid( ) const {
			wait( );
			return m_state->result.index( ) == 1 or m_state->result.index( ) == 2;
		}

		T &get( ) {
			wait( );
			return daw::visit_nt(
			  m_state->result,
			  []( std::monostate ) -> T & {
				  throw std::runtime_error( "Attempt to access an empty result" );
			  },
			  []( T &value ) -> T & { return value; },
			  []( std::exception_ptr ep ) -> T & { std::rethrow_exception( ep ); } );
		}

		T const &get( ) const {
			wait( );
			return daw::visit_nt(
			  m_state->result,
			  []( std::monostate ) -> T const & {
				  throw std::runtime_error( "Attempt to access an empty result" );
			  },
			  []( T &value ) -> T const & { return value; },
			  []( std::exception_ptr ep ) -> T const & {
				  std::rethrow_exception( ep );
			  } );
		}
	};

	template<>
	class async_result<void> {
		std::shared_ptr<async_result_state<void>> m_state;

	public:
		inline async_result(
		  std::shared_ptr<async_result_state<void>> state ) noexcept
		  : m_state( std::move( state ) ) {}

		[[nodiscard]] inline bool try_wait( ) const {
			return m_state->token.try_wait( );
		}

		inline void wait( ) const {
			m_state->token.wait( );
		}

		template<typename Rep, typename Period>
		[[nodiscard]] decltype( auto )
		wait_for( std::chrono::duration<Rep, Period> const &rel_time ) const {
			return m_state->token.wait_for( rel_time );
		}

		template<typename Clock, typename Duration>
		[[nodiscard]] decltype( auto ) wait_until(
		  std::chrono::time_point<Clock, Duration> const &timeout_time ) const {
			return m_state->token.wait_until( timeout_time );
		}

		bool is_valid( ) const {
			wait( );
			return m_state->result.index( ) == 1 or m_state->result.index( ) == 2;
		}

		void get( ) const {
			wait( );
			return daw::visit_nt(
			  m_state->result,
			  []( std::monostate ) -> void {
				  throw std::runtime_error( "Attempt to access an empty result" );
			  },
			  []( async_result_state<void>::void_value_t ) -> void {},
			  []( std::exception_ptr ep ) -> void { std::rethrow_exception( ep ); } );
		}
	};
} // namespace daw
