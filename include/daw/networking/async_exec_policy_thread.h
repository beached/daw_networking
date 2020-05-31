// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include "details/locked_queue.h"
#include "task_token.h"
#include "third_party/jthread.hpp"

#include <daw/daw_scope_guard.h>
#include <daw/daw_utility.h>

namespace daw {
	/***
	 * If queue is cleared witout running task, this ensures that the token
	 * allows the waiters through
	 */
	struct packaged_task {
		struct state_t {
			mutable std::function<void( )> m_function;
			mutable task_token m_token;

			state_t( std::function<void( )> function, task_token token )
			  : m_function( std::move( function ) )
			  , m_token( std::move( token ) ) {}
		};
		std::unique_ptr<state_t> m_state;

		~packaged_task( ) {
			if( m_state and not m_state->m_token.try_wait( ) ) {
				m_state->m_token.notify( );
			}
		}
		packaged_task( packaged_task const & ) = delete;
		packaged_task( packaged_task && ) noexcept = default;
		packaged_task &operator=( packaged_task const & ) = delete;
		packaged_task &operator=( packaged_task && ) noexcept = default;

		packaged_task( std::function<void( )> task, task_token token )
		  : m_state( std::make_unique<state_t>( std::move( task ),
		                                        std::move( token ) ) ) {}

		void operator( )( ) const noexcept {
			try {
				std::move( m_state->m_function )( );
			} catch( ... ) {}
		}
	};

	class async_exec_policy_thread {
		daw::locked_queue<packaged_task> m_queue =
		  daw::locked_queue<packaged_task>( );
		std::jthread m_thread;

	public:
		~async_exec_policy_thread( );
		async_exec_policy_thread( );
		task_token add_task( std::function<void( )> tsk );
		void wait( ) const;
	};
} // namespace daw
