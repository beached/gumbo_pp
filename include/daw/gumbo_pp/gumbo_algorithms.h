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
#include "gumbo_node_iterator.h"

#include <daw/daw_move.h>
#include <daw/daw_string_view.h>

namespace daw::gumbo {
	/// When a node satisfies the predicates, copy the reference value to the
	/// Output range
	template<typename OutputIterator, typename... Predicates>
	static OutputIterator find_all_if( gumbo_node_iterator_t first,
	                                   gumbo_node_iterator_t last,
	                                   OutputIterator out_it,
	                                   Predicates... preds ) {
		while( first != last ) {
			if( first.get( ) and ( preds( *first ) and ... ) ) {
				*out_it = *first;
				++out_it;
			}
			++first;
		}
	}

	/// When a node satisfies the predicates, pass the reference value to the
	/// OnEach callback
	template<typename OnEach, typename... Predicates>
	static void find_all_if_each( gumbo_node_iterator_t first,
	                              gumbo_node_iterator_t last,
	                              OnEach onEach,
	                              Predicates... preds ) {
		while( first != last ) {
			if( first and ( preds( *first ) and ... ) ) {
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
		find_all_if_each( first,
		                  last,
		                  DAW_MOVE( onEach ),
		                  [tag]( GumboNode const &node ) {
			                  return node.type == GUMBO_NODE_ELEMENT and
			                         node.v.element.tag == tag;
		                  } );
	}

	struct attribute_search_result_t {
		gumbo_node_iterator_t iter;
		unsigned attrib_pos;
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

	template<typename... Predicates>
	static gumbo_node_iterator_t find_if( gumbo_node_iterator_t first,
	                                      gumbo_node_iterator_t last,
	                                      Predicates... preds ) {
		while( first != last ) {
			if( ( preds( *first ) and ... ) ) {
				return first;
			}
			++first;
		}
		return last;
	}

	template<typename Compare = std::equal_to<>>
	gumbo_node_iterator_t
	find_node_by_attribute_value( gumbo_node_iterator_t first,
	                              gumbo_node_iterator_t last,
	                              daw::string_view attribute_name,
	                              daw::string_view attribute_value,
	                              Compare cmp = { } ) {
		auto sresult = find_node_by_attribute_name( first, last, attribute_name );
		first = sresult.iter;
		while( first != last ) {
			auto &attr = *get_attribute_node_at( *first, sresult.attrib_pos );
			if( cmp( attr.value, attribute_value ) ) {
				return first;
			}
			++first;
			sresult = find_node_by_attribute_name( first, last, attribute_name );
			first = sresult.iter;
		}
		return first;
	}

	template<typename Predicate>
	constexpr find_attribute_result_t find_attribute_if( gumbo_node_iterator_t it,
	                                                     Predicate pred ) {
		return details::find_attribute_if_impl( DAW_MOVE( it ), DAW_MOVE( pred ) );
	}

	template<typename Predicate>
	attribute_search_result_t
	find_node_by_attribute_if( gumbo_node_iterator_t first,
	                           gumbo_node_iterator_t last,
	                           Predicate pred ) {
		while( first != last ) {
			auto [found, idx] = find_attribute_if( first, pred );
			if( found ) {
				return { first, idx };
			}
			++first;
		}
		return { first, 0 };
	}
} // namespace daw::gumbo
