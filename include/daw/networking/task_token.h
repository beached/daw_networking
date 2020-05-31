// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <daw/parallel/daw_latch.h>

namespace daw {
	struct task_token : private daw::shared_latch {
		inline task_token( )
		  : daw::shared_latch( 1 ) {}

		using daw::shared_latch::notify;
		using daw::shared_latch::operator bool;
		using daw::shared_latch::set_latch;
		using daw::shared_latch::try_wait;
		using daw::shared_latch::wait;
		using daw::shared_latch::wait_for;
		using daw::shared_latch::wait_until;
	};
}


