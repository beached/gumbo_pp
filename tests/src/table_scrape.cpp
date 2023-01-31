// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/gumbo_pp
//

#include <daw/daw_string_view.h>
#include <daw/gumbo_pp.h>
#include <daw/iterator/daw_find_iterator.h>

#include <algorithm>
#include <iostream>

inline constexpr daw::string_view test_doc = R"html(
<html>
<head>
	<title>Table example</title>
</head>
<body>
	<div id="important_table">
		<table>
			<tbody>
				<tr>
					<td><strong>Item</strong></td>
					<td><strong>Quantity</strong></td>
				</tr>
				<tr>
					<td>Plate</td>
					<td>10</td>
				</tr>
				<tr>
					<td>Bowl</td>
					<td>5</td>
				</tr>
			</tbody>
		</table>
	</div>
</body>
)html";

int main( ) {
	auto html = daw::gumbo::gumbo_range( test_doc );
	namespace match = daw::gumbo::match;
	auto parent_div =
	  std::find_if( html.begin( ),
	                html.end( ),
	                match::tag::DIV and match::id::is( "important_table" ) );
	assert( parent_div != html.end( ) );
	auto tbl =
	  std::find_if( parent_div.begin( ), parent_div.end( ), match::tag::TBODY );
	assert( tbl != html.end( ) );
	for( daw::gumbo::gumbo_node_iterator_t tr_it :
	     daw::find_iterator( tbl.children.begin( ),
	                         tbl.children.end( ),
	                         match::tag::TR ) ) {
		bool is_first_col = true;
		for( daw::gumbo::gumbo_node_iterator_t td_it :
		     daw::find_iterator( tr_it.children.begin( ),
		                         tr_it.children.end( ),
		                         match::tag::TD ) ) {
			if( not is_first_col ) {
				std::cout << ',';
			}
			is_first_col = false;
			std::cout << daw::gumbo::node_content_text( *td_it );
		}
		std::cout << '\n';
	}
}