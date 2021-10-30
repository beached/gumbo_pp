// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/gumbo_pp
//
//

#include <daw/gumbo_pp/gumbo_algorithms.h>
#include <daw/gumbo_pp/gumbo_handle.h>
#include <daw/gumbo_pp/gumbo_node_iterator.h>
#include <daw/gumbo_pp/gumbo_util.h>

int main( ) {
	std::string const html = "<html></html>";
	daw::gumbo::GumboHandle output =
	  gumbo_parse_with_options( &kGumboDefaultOptions,
	                            html.c_str( ),
	                            html.size( ) );
}
