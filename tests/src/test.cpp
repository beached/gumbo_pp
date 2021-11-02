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
	<body><div class='hello'><b>Hey folks!</b></div><a href="http://www.google.com">Google</a></body>
</html>)html";
	using daw::gumbo::match;
	daw::gumbo::GumboHandle output =
	  gumbo_parse_with_options( &kGumboDefaultOptions,
	                            html.data( ),
	                            html.size( ) );

	std::cout << "******\n";

	auto const first = daw::gumbo::gumbo_node_iterator_t( output->root );
	auto const last = daw::gumbo::gumbo_node_iterator_t( );

	daw::algorithm::for_each_if(
	  first,
	  last,
	  match::tag::types<GUMBO_TAG_DIV>,
	  [&]( GumboNode const &node ) {
		  std::cout << "node text: " << daw::gumbo::node_content_text( node )
		            << '\n';
		  std::cout << "node inner text: "
		            << daw::gumbo::node_inner_text( node, html ) << '\n';
	  } );

	std::cout << "******\n";
	std::cout << "All div.hello 's\n";
	daw::algorithm::for_each_if(
	  first,
	  last,
	  match::tag::types<GUMBO_TAG_DIV> and match::class_type::is( "hello" ),
	  [&]( GumboNode const &node ) {
		  std::cout << "node text: " << daw::gumbo::node_outter_text( node, html )
		            << '\n';
	  } );

	std::cout << "******\n";
	auto pos = std::find_if( first,
	                         last,
	                         match::attribute::value::is( "class", "hello" ) );

	if( pos ) {
		std::cout << "Class hello outer text: "
		          << daw::gumbo::node_outter_text( *pos, html ) << '\n';
	}

	pos = std::find_if( first, last, match::tag::types<GUMBO_TAG_A> );
	if( pos ) {
		std::cout << "Anchor content text: "
		          << daw::gumbo::node_content_text( *pos ) << '\n';
	}
}
