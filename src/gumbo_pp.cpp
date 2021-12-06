// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/gumbo_pp
//

#include <daw/gumbo_pp.h>
#include <daw/gumbo_pp/details/gumbo_pp.h>

#include "daw/gumbo_pp/gumbo_node_iterator.h"
#include <daw/daw_move.h>

namespace daw::gumbo::details {
	void gumbo_pp_library( ) {}
} // namespace daw::gumbo::details

namespace daw::gumbo {
	gumbo_range::gumbo_range( GumboHandle &&handle )
	  : m_handle( DAW_MOVE( handle ) ) {}

	gumbo_range::gumbo_range( std::string_view html_document,
	                          GumboOptions options )
	  : m_handle( gumbo_parse_with_options( &options,
	                                        html_document.data( ),
	                                        html_document.size( ) ) )
	  , m_first( m_handle->root ) {}

	gumbo_range::gumbo_range( std::string_view html_document )
	  : gumbo_range( html_document, kGumboDefaultOptions ) {}
} // namespace daw::gumbo
