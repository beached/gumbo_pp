// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/gumbo_pp
//

#pragma once

#include "../gumbo_node_iterator.h"
#include "../gumbo_util.h"

#include <cstddef>
#include <gumbo.h>

namespace daw::gumbo {
	struct find_attribute_result_t {
		bool found;
		std::size_t index;
	};

	namespace details {
		template<typename Predicate>
		inline constexpr find_attribute_result_t
		find_attribute_if_impl( gumbo_node_iterator_t it, Predicate pred ) {
			if( it->type == GumboNodeType::GUMBO_NODE_ELEMENT ) {
				auto const attr_count = get_attribute_count( *it );
				for( unsigned n = 0; n < attr_count; ++n ) {
					auto *attr = get_attribute_node_at( *it, n );
					if( pred( *attr ) ) {
						return { true, n };
					}
				}
				return { false, attr_count };
			}
			return { false, 0 };
		}
	} // namespace details
} // namespace daw::gumbo