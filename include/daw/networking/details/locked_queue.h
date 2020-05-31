//////////////////////////////////////////////////////////////////////////
/// Summary:	concurrent queue by Anthony Williams from
///				https://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
///
//////////////////////////////////////////////////////////////////////////
/***
 * Modified to use stop_token supplied in ctor
 */
#pragma once

#include "third_party/stop_token.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

namespace daw {
	template<typename Data>
	class locked_queue {
	private:
		std::queue<Data> m_queue{ };
		mutable std::mutex m_mutex{ };
		mutable std::condition_variable m_condition{ };
		std::stop_token m_should_stop{ };

	public:
		locked_queue( ) = default;

		locked_queue( std::stop_token should_stop )
		  : m_should_stop( std::move( should_stop ) ) {}

		void push( Data const &data ) {
			auto lock = std::unique_lock( m_mutex );
			m_queue.push( data );
			lock.unlock( );
			m_condition.notify_one( );
		}

		void push( Data &&data ) {
			auto lock = std::unique_lock( m_mutex );
			m_queue.push( std::move( data ) );
			lock.unlock( );
			m_condition.notify_one( );
		}

		bool empty( ) const {
			auto const lock = std::unique_lock( m_mutex );
			return m_queue.empty( );
		}

		std::optional<Data> try_pop( ) {
			auto const lock = std::unique_lock( m_mutex );
			if( m_queue.empty( ) ) {
				return { };
			}
			auto const oe = on_scope_exit( [&] { m_queue.pop( ); } );
			return std::move( m_queue.front( ) );
		}

		std::optional<Data> wait_and_pop( ) {
			auto lock = std::unique_lock( m_mutex );
			m_condition.wait( lock, [&] {
				return m_should_stop.stop_requested( ) or not m_queue.empty( );
			} );
			if( m_should_stop.stop_requested( ) ) {
				return { };
			}
			auto const oe = on_scope_exit( [&] { m_queue.pop( ); } );
			return std::move( m_queue.front( ) );
		}

		void wait( ) const {
			auto lock = std::unique_lock( m_mutex );
			m_condition.wait( lock, [&] {
				return m_should_stop.stop_requested( ) or not m_queue.empty( );
			} );
		}

		void clear( ) {
			auto const lock = std::unique_lock( m_mutex );
			m_queue = std::queue<Data>( );
		}

		void reset( std::stop_token should_stop ) {
			auto const lock = std::unique_lock( m_mutex );
			m_queue = std::queue<Data>( );
			m_should_stop = std::move( should_stop );
		}

		void notify_all( ) {
			m_condition.notify_all( );
		}
	}; // class locked_queue
} // namespace daw
