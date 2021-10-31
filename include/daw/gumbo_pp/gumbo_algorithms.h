// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/gumbo_pp
//

#pragma once

#include "details/gumbo_pp.h"
#include "gumbo_node_iterator.h"

#include <daw/daw_move.h>

namespace daw::gumbo {
	template<typename OutputIterator, typename Predicate>
	static OutputIterator find_all_if( gumbo_node_iterator_t first,
	                                   gumbo_node_iterator_t last,
	                                   OutputIterator out_it,
	                                   Predicate pred ) {
		while( first != last ) {
			if( first.get( ) and pred( *first ) ) {
				*out_it = *first;
				++out_it;
			}
			++first;
		}
	}

	template<typename Predicate, typename OnEach>
	static void find_all_if_each( gumbo_node_iterator_t first,
	                              gumbo_node_iterator_t last,
	                              Predicate pred,
	                              OnEach onEach ) {
		while( first != last ) {
			if( first.get( ) and pred( *first ) ) {
				onEach( *first );
			}
			++first;
		}
	}

	template<typename OutputIterator>
	static OutputIterator find_all( gumbo_node_iterator_t first,
	                                gumbo_node_iterator_t last,
	                                OutputIterator out_it,
	                                GumboTag tag ) {
		return find_all_if( first, last, out_it, [tag]( GumboNode const &node ) {
			return node.type == GUMBO_NODE_ELEMENT and node.v.element.tag == tag;
		} );
	}

	template<typename OnEach>
	static void find_all_oneach( gumbo_node_iterator_t first,
	                             gumbo_node_iterator_t last,
	                             GumboTag tag,
	                             OnEach onEach ) {
		find_all_if_each(
		  first,
		  last,
		  [tag]( GumboNode const &node ) {
			  return node.type == GUMBO_NODE_ELEMENT and node.v.element.tag == tag;
		  },
		  DAW_MOVE( onEach ) );
	}

	struct attribute_search_result_t {
		gumbo_node_iterator_t position;
		unsigned attribute_position;
	};

	inline attribute_search_result_t
	find_node_by_attribute_name( gumbo_node_iterator_t first,
	                             gumbo_node_iterator_t last,
	                             daw::string_view attribute_name ) {
		while( first != last ) {
			if( first->type == GumboNodeType::GUMBO_NODE_ELEMENT ) {
				auto const attr_count = get_attribute_count( *first );
				for( unsigned n = 0; n < attr_count; ++n ) {
					auto *attr = get_attribute_node_at( *first, n );
					if( attr and daw::string_view( attr->name ) == attribute_name ) {
						return { first, n };
					}
				}
			}
			++first;
		}
		return { first, 0 };
	}

	template<typename Compare = std::equal_to<>>
	gumbo_node_iterator_t
	find_node_by_attribute_value( gumbo_node_iterator_t first,
	                              gumbo_node_iterator_t last,
	                              daw::string_view attribute_name,
	                              daw::string_view attribute_value,
	                              Compare cmp = { } ) {
		auto sresult = find_node_by_attribute_name( first, last, attribute_name );
		first = sresult.position;
		while( first != last ) {
			auto &attr = *get_attribute_node_at( *first, sresult.attribute_position );
			if( cmp( attr.value, attribute_value ) ) {
				return first;
			}
			++first;
			sresult = find_node_by_attribute_name( first, last, attribute_name );
			first = sresult.position;
		}
		return first;
	}
} // namespace daw::gumbo
