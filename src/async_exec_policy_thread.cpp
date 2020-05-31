// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "daw/networking/async_exec_policy_thread.h"

namespace daw {
	async_exec_policy_thread::~async_exec_policy_thread( ) {
		m_thread.request_stop( );
		m_queue.clear( );
		m_queue.notify_all( );
	}

	async_exec_policy_thread::async_exec_policy_thread( )
	  : m_thread( [&]( std::stop_token should_stop ) {
		  while( not should_stop.stop_requested( ) ) {
			  auto tsk = m_queue.wait_and_pop( );
			  if( tsk ) {
				  ( *tsk )( );
			  }
		  }
	  } ) {
		m_queue.reset( m_thread.get_stop_token( ) );
	}

	void async_exec_policy_thread::wait( ) const {
		m_queue.wait( );
	}

	task_token async_exec_policy_thread::add_task( std::function<void( )> tsk ) {
		auto tok = task_token( );
		m_queue.push( packaged_task( std::move( tsk ), tok ) );
		return tok;
	}
} // namespace daw
