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

#include <iostream>

int main( ) {
	constexpr std::string_view html =
	  R"html(
<html>
	<head>
		<title>Test</title>
	</head>
	<body><div class='hello'>Hey folks!</div></body>
</html>)html";
	daw::gumbo::GumboHandle output =
	  gumbo_parse_with_options( &kGumboDefaultOptions,
	                            html.data( ),
	                            html.size( ) );

	daw::gumbo::find_all_oneach(
	  output->root,
	  { },
	  GUMBO_TAG_DIV,
	  [&]( GumboNode const &node ) {
		  std::cout << "node text: " << daw::gumbo::node_text( node ) << '\n';
		  std::cout << "node inner text: "
		            << daw::gumbo::node_inner_text( node, html ) << '\n';
	  } );

	auto pos = daw::gumbo::find_node_by_attribute_value( output->root,
	                                                     { },
	                                                     "class",
	                                                     "hello" );
	if( pos ) {
		std::cout << "Class hello outer text: "
		          << daw::gumbo::node_outter_text( *pos, html ) << '\n';
	}
}
