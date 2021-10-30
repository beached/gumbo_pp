// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/gumbo_pp
//

#pragma once

#include "details/gumbo_pp.h"

#include <gumbo.h>
#include <memory>

namespace daw::gumbo {
	struct GumboDeleter {
		constexpr GumboDeleter( ) = default;

		inline void operator( )( GumboOutput *output ) const {
			if( not output ) {
				return;
			}
			gumbo_destroy_output( &kGumboDefaultOptions, output );
		}
	};

	struct GumboHandle : std::unique_ptr<GumboOutput, GumboDeleter> {
		using base = std::unique_ptr<GumboOutput, GumboDeleter>;
		inline GumboHandle( GumboOutput *ptr )
		  : base( ptr ) {}
	};
} // namespace daw::gumbo
