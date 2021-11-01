// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/gumbo_pp
//
//

#include <daw/gumbo_pp.h>

#include <cassert>
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
	using daw::gumbo::match;
	daw::gumbo::GumboHandle output =
	  gumbo_parse_with_options( &kGumboDefaultOptions,
	                            html.data( ),
	                            html.size( ) );

	std::cout << "******\n";
	daw::gumbo::find_all_if_oneach(
	  output->root,
	  { },
	  [&]( GumboNode const &node ) {
		  std::cout << "node text: " << daw::gumbo::node_text( node ) << '\n';
		  std::cout << "node inner text: "
		            << daw::gumbo::node_inner_text( node, html ) << '\n';
	  },
	  match::tag::types<GUMBO_TAG_DIV> );

	std::cout << "******\n";
	std::cout << "All div.hello 's\n";
	daw::gumbo::find_all_if_oneach(
	  output->root,
	  { },
	  [&]( GumboNode const &node ) {
		  std::cout << "node text: " << daw::gumbo::node_outter_text( node, html )
		            << '\n';
	  },
	  match::tag::types<GUMBO_TAG_DIV>,
	  match::class_type::is( "hello" ) );

	std::cout << "******\n";
	auto pos =
	  daw::gumbo::find_if( output->root,
	                       { },
	                       match::attribute::value::is( "class", "hello" ) );

	if( pos ) {
		std::cout << "Class hello outer text: "
		          << daw::gumbo::node_outter_text( *pos, html ) << '\n';
	}
}
