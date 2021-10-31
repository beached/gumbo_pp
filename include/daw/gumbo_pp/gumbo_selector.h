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
	enum class select_type { all, contains_prefix, contains };

	template<select_type Type>
	struct select_tag_t {
		static constexpr select_type type = Type;
	};
	template<select_type Type>
	inline constexpr select_tag_t<Type> select_tag = { };

	class selector {
		gumbo_node_iterator_t m_first{ };
		gumbo_node_iterator_t m_last{ };

	public:
		constexpr selector( gumbo_node_iterator_t first )
		  : m_first( first ) {}
		constexpr selector( gumbo_node_iterator_t first,
		                    gumbo_node_iterator_t last )
		  : m_first( first )
		  , m_last( last ) {}

		inline std::vector<GumboNode *>
		select( select_tag_t<select_type::all> ) const {
			std::vector<GumboNode *> result{ };
			auto first = m_first;
			auto last = m_last;
			while( first != last ) {
				result.push_back( &*first );
				++first;
			}
			return result;
		}

		inline std::vector<GumboNode *>
		select( select_tag_t<select_type::contains_prefix>,
		        daw::string_view name,
		        daw::string_view value ) const {

			std::vector<GumboNode *> result{ };
			auto const pred = [&]( GumboAttribute const &attr ) {
				daw::string_view attr_value = attr.value;
				return daw::nsc_and(
				  name == attr.name,
				  attr_value.starts_with( value ),
				  attr_value.substr( value.size( ) ).starts_with( '-' ) );
			};
			auto first = m_first;
			auto last = m_last;
			auto sresult = find_node_by_attribute_if( first, last, pred );
			first = sresult.iter;
			while( first != last ) {
				result.push_back( &*first );
				++first;
				sresult = find_node_by_attribute_if( first, last, pred );
				first = sresult.iter;
			}
			return result;
		}

		inline std::vector<GumboNode *> select( select_tag_t<select_type::contains>,
		                                        daw::string_view name,
		                                        daw::string_view value ) const {

			std::vector<GumboNode *> result{ };
			auto const pred = [&]( GumboAttribute const &attr ) {
				daw::string_view attr_value = attr.value;
				return daw::nsc_and( name == attr.name,
				                     attr_value.find( value ) !=
				                       daw::string_view::npos );
			};

			auto first = m_first;
			auto last = m_last;
			auto sresult = find_node_by_attribute_if( first, last, pred );
			first = sresult.iter;
			while( first != last ) {
				result.push_back( &*first );
				++first;
				sresult = find_node_by_attribute_if( first, last, pred );
				first = sresult.iter;
			}
			return result;
		}
	};
} // namespace daw::gumbo
