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
	static void find_all_if_oneach( gumbo_node_iterator_t first,
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
} // namespace daw::gumbo
