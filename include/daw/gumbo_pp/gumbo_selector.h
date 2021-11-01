// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/gumbo_pp
//

#pragma once

#include "details/find_attrib_if_impl.h"
#include "details/gumbo_pp.h"
#include "gumbo_algorithms.h"
#include "gumbo_node_iterator.h"

#include <daw/daw_logic.h>
#include <daw/daw_string_view.h>

namespace daw::gumbo {
	/// No op function object that is true for any Node
	struct match_all {
		template<typename Node>
		inline constexpr bool operator( )( Node const & ) const noexcept {
			return true;
		}
	};

	/// Match any node that has an attribute where the predicate returns true
	template<typename Predicate>
	struct match_attr_if : private Predicate {
		inline constexpr match_attr_if( Predicate p )
		  : Predicate( DAW_MOVE( p ) ) {}

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const {
			return details::find_attribute_if_impl(
			         gumbo_node_iterator_t( &node ),
			         [&]( GumboAttribute const &attr ) {
				         return Predicate::operator( )(
				           daw::string_view( attr.name ),
				           daw::string_view( attr.value ) );
			         } )
			  .found;
		}
	};

	// Match any node that has a matching attribute name
	struct match_attr_name {
		daw::string_view name;
		template<typename Node>
		constexpr bool operator( )( Node const &node ) const noexcept {
			return details::find_attribute_if_impl(
			         gumbo_node_iterator_t( node ),
			         [&]( GumboAttribute const &attr ) {
				         return daw::string_view( attr.name ) == name;
			         } )
			  .found;
		}
	};

	/// Match any node with named attribute who's value is either value or
	/// prefixed by value and a hyphen `-`
	struct match_attr_contains_prefix {
		daw::string_view name;
		daw::string_view value;

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const noexcept {
			return match_attr_if{
			  [&]( daw::string_view attr_name, daw::string_view attr_value ) {
				  if( name != attr_name ) {
					  return false;
				  }
				  if( not attr_value.starts_with( value ) ) {
					  return value;
				  }
				  if( attr_value.size( ) == value.size( ) ) {
					  return true;
				  }
				  return attr_value.substr( value.size( ) ).starts_with( '-' );
			  } }( node );
		}
	};

	/// Match any node with named attribute who's value contains the specified
	/// value
	struct match_attr_contains {
		daw::string_view name;
		daw::string_view value;

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const noexcept {
			return match_attr_if{
			  [&]( daw::string_view attr_name, daw::string_view attr_value ) {
				  return attr_name == name and
				         attr_value.find( value ) != daw::string_view::npos;
			  } }( node );
		}
	};

	/// Match any node with named attribute who's value starts with the
	/// specified value
	struct match_attr_starts_with {
		daw::string_view name;
		daw::string_view value;

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const noexcept {
			return match_attr_if{
			  [&]( daw::string_view attr_name, daw::string_view attr_value ) {
				  return attr_name == name and attr_value.starts_with( value );
			  } }( node );
		}
	};

	/// Match any node with named attribute who's value ends with the specified
	/// value
	struct match_attr_ends_with {
		daw::string_view name;
		daw::string_view value;

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const noexcept {
			return match_attr_if{
			  [&]( daw::string_view attr_name, daw::string_view attr_value ) {
				  return attr_name == name and attr_value.ends_with( value );
			  } }( node );
		}
	};

	/// Match any node with named attribute who's value equals with the specified
	/// value
	struct match_attr_equals {
		daw::string_view name;
		daw::string_view value;

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const noexcept {
			return match_attr_if{
			  [&]( daw::string_view attr_name, daw::string_view attr_value ) {
				  return attr_name == name and attr_value == value;
			  } }( node );
		}
	};

	/// Match any node with a class that returns true for the predicate
	template<typename Predicate>
	struct match_class_if : private Predicate {
		inline constexpr match_class_if( Predicate p )
		  : Predicate( DAW_MOVE( p ) ) {}

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const noexcept {
			return match_attr_if{
			  [&]( daw::string_view attr_name, daw::string_view attr_value ) {
				  return attr_name == "class" and Predicate::operator( )( attr_value );
			  } }( node );
		}
	};

	/// Match any node with a class name equal to name
	struct match_class_equals {
		daw::string_view name;

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const noexcept {
			return match_class_if{ [&]( daw::string_view class_name ) {
				return class_name == name;
			} }( node );
		}
	};

	/// Match any node with an id that returns true for the predicate
	template<typename Predicate>
	struct match_id_if : private Predicate {
		inline constexpr match_id_if( Predicate p )
		  : Predicate( DAW_MOVE( p ) ) {}

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const noexcept {
			return match_attr_if{
			  [&]( daw::string_view attr_name, daw::string_view attr_value ) {
				  return attr_name == "id" and Predicate::operator( )( attr_value );
			  } }( node );
		}
	};

	/// Match any node with an id equal to name
	struct match_id_equals {
		daw::string_view name;

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const noexcept {
			return match_class_if{
			  [&]( daw::string_view id_name ) { return id_name == name; } }( node );
		}
	};

	/// Match any node that has inner text where the predicate returns true
	template<typename Predicate>
	struct match_inner_text_if : private Predicate {
		inline constexpr match_inner_text_if( Predicate p )
		  : Predicate( DAW_MOVE( p ) ) {}

		daw::string_view html_doc;
		template<typename Node>
		constexpr bool operator( )( Node const &node ) const {
			return Predicate::operator( )( node_inner_text( node, html_doc ) );
		}
	};

	/// Match any node with inner text who's value contains the specified value
	struct match_inner_text_contains {
		daw::string_view value;

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const noexcept {
			return match_inner_text_if{ [&]( daw::string_view inner_text ) {
				return inner_text.find( value ) != daw::string_view::npos;
			} }( node );
		}
	};

	/// Match any node with inner text who's value starts with the specified value
	struct match_inner_text_starts_with {
		daw::string_view value;

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const noexcept {
			return match_inner_text_if{ [&]( daw::string_view inner_text_value ) {
				return inner_text_value.starts_with( value );
			} }( node );
		}
	};

	/// Match any node with inner text who's value ends with the specified value
	struct match_inner_text_ends_with {
		daw::string_view value;

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const noexcept {
			return match_inner_text_if{ [&]( daw::string_view inner_text_value ) {
				return inner_text_value.ends_with( value );
			} }( node );
		}
	};

	/// Match any node with inner text who's value equals with the specified value
	struct match_inner_text_equals {
		daw::string_view value;

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const noexcept {
			return match_inner_text_if{ [&]( daw::string_view inner_text_value ) {
				return inner_text_value == value;
			} }( node );
		}
	};

	/// Match any node with where the tag type where the Predicate returns true
	template<typename Predicate>
	struct match_tag_if : private Predicate {
		inline constexpr match_tag_if( Predicate p )
		  : Predicate( DAW_MOVE( p ) ) {}

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const {
			return node.type == GUMBO_NODE_ELEMENT and
			       Predicate::operator( )( node.v.element.tag );
		}
	};

	/// Match any node with where the tag type where the tag type matches on of
	/// the specified types
	template<GumboTag... tags>
	struct match_tag_types_t {
		static_assert( sizeof...( tags ) > 0, "Must specify at least one tag" );

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const noexcept {
			return match_tag_if{ [&]( GumboTag tag_value ) {
				return ( ( tag_value == tags ) | ... );
			} }( node );
		}
	};

	template<GumboTag... tags>
	inline constexpr match_tag_types_t<tags...> match_tag_types = { };

} // namespace daw::gumbo
