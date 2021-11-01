// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/gumbo_pp
//

#pragma once

#include "details/gumbo_pp.h"
#include "gumbo_algorithms.h"
#include "gumbo_node_iterator.h"

#include <daw/daw_logic.h>
#include <daw/daw_string_view.h>

namespace daw::gumbo {
	/// No op function object that is true for any Node
	struct match_all {
		template<typename Node>
		constexpr bool operator( )( Node const & ) const {
			return true;
		}
	};

	/// Match any node with named attribute who's value is either value or
	/// prefixed by value and a hyphen `-`
	struct match_attr_contains_prefix {
		daw::string_view name;
		daw::string_view value;

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const {
			return find_attribute_if(
			         gumbo_node_iterator_t( node ),
			         [&]( GumboAttribute const &attr ) {
				         daw::string_view attr_value = attr.value;
				         if( name != attr.name ) {
					         return false;
				         }
				         if( not attr_value.starts_with( value ) ) {
					         return value;
				         }
				         if( attr_value.size( ) == value.size( ) ) {
					         return true;
				         }
				         return attr_value.substr( value.size( ) ).starts_with( '-' );
			         } )
			  .found;
		}
	};

	/// Match any node with named attribute who's value contains the specified
	/// value
	struct match_attr_contains {
		daw::string_view name;
		daw::string_view value;

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const {
			return find_attribute_if( gumbo_node_iterator_t( node ),
			                          [&]( GumboAttribute const &attr ) {
				                          daw::string_view attr_value = attr.value;
				                          return daw::nsc_and(
				                            name == attr.name,
				                            attr_value.find( value ) !=
				                              daw::string_view::npos );
			                          } )
			  .found;
		}
	};
} // namespace daw::gumbo
