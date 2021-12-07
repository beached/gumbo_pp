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
	<body><div class='hello'><b>Hey folks!</b></div> <a href="https://www.google.com">Google</a></body>
</html>)html";

	auto doc_range = daw::gumbo::gumbo_range( html );

	std::cout << "****************\n";
	std::cout << daw::gumbo::node_content_text( *doc_range.document( ) ) << '\n';
	std::cout << "****************\n";

	namespace match = daw::gumbo::match;
	daw::algorithm::for_each_if(
	  doc_range.begin( ),
	  doc_range.end( ),
	  match::tag::DIV,
	  [&]( GumboNode const &node ) {
		  std::cout << "****************\n";
		  std::cout << "node text:\n";
		  std::cout << "****************\n";
		  std::cout << daw::gumbo::node_content_text( node ) << '\n';
		  std::cout << "****************\n";
		  std::cout << "node inner text:\n";
		  std::cout << "****************\n";
		  std::cout << daw::gumbo::node_inner_text( node, html ) << '\n';
		  std::cout << "****************\n";
	  } );

	std::cout << "****************\n";
	std::cout << "All div.hello 's\n";

	daw::algorithm::for_each_if(
	  doc_range.begin( ),
	  doc_range.end( ),
	  /*match::tag::DIV and*/ match::class_type::is( "hello" ),
	  [&]( GumboNode const &node ) {
		  std::cout << "node text: " << daw::gumbo::node_outer_text( node, html )
		            << '\n';
	  } );
	std::cout << "****************\n";

	auto pos = std::find_if( doc_range.begin( ),
	                         doc_range.end( ),
	                         match::attribute::value::is( "class", "hello" ) );
	if( pos ) {
		std::cout << "Class hello outer text:\n";
		std::cout << "****************\n";
		std::cout << daw::gumbo::node_outer_text( *pos, html ) << '\n';
	}

	std::cout << "****************\n";
	pos = std::find_if( doc_range.begin( ), doc_range.end( ), match::tag::A );
	if( pos ) {
		std::cout << "Anchor content text:\n";
		std::cout << "****************\n";
		std::cout << daw::gumbo::node_content_text( *pos ) << '\n';
	}
	std::cout << "****************\n";

	constexpr daw::string_view html2 =
	  R"html(<p id="example">This is an <strong>example</strong> paragraph</p>)html";
	daw::gumbo::GumboHandle html2_hnd =
	  gumbo_parse_with_options( &kGumboDefaultOptions,
	                            html2.data( ),
	                            html2.size( ) );
	auto html2_rng = daw::gumbo::gumbo_node_iterator_t( html2_hnd->root );
	auto html2_example_pos =
	  std::find_if( html2_rng.begin( ),
	                html2_rng.end( ),
	                match::tag::P and match::id::is( "example" ) );

	assert( html2_example_pos != html2_rng.end( ) );
	auto txt = daw::gumbo::node_content_text( *html2_example_pos );
	std::cout << "****************\n";
	std::cout << "example text: '" << txt << "'\n";
	auto some = daw::algorithm::find_some( html2_rng.begin( ),
	                                       html2_rng.end( ),
	                                       match::tag::P,
	                                       match::id::is( "example" ) );
	assert( some.position != html2_rng.end( ) );
	assert( std::find( some.results.begin( ), some.results.end( ), false ) ==
	        some.results.end( ) );
	std::cout << "****************\n";
}
