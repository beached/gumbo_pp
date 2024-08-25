// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/gumbo_pp
//

#include <daw/gumbo_pp.h>
#include <daw/gumbo_pp/details/gumbo_pp.h>

#include "daw/gumbo_pp/gumbo_node_iterator.h"
#include <daw/daw_string_view.h>

#include <iterator>
#include <utility>

namespace daw::gumbo::details {
	void gumbo_pp_library( ) {}
} // namespace daw::gumbo::details
// namespace daw::gumbo::details

namespace daw::gumbo {
	gumbo_range::gumbo_range( GumboHandle &&handle )
	  : m_handle( std::move( handle ) ) {}

	gumbo_range::gumbo_range( daw::string_view html_document,
	                          GumboOptions options )
	  : m_handle( gumbo_parse_with_options(
	      &options, html_document.data( ), html_document.size( ) ) )
	  , m_first( m_handle->root ) {}

	gumbo_range::gumbo_range( daw::string_view html_document )
	  : gumbo_range( html_document, kGumboDefaultOptions ) {}

	namespace {
		[[nodiscard]] gumbo_node_iterator_t
		get_first_child( GumboNode const &parent_node ) {
			auto const child_count = get_children_count( parent_node );
			if( child_count == 0 ) {
				return gumbo_node_iterator_t( parent_node );
			}
			return gumbo_node_iterator_t( get_child_node_at( parent_node, 0 ) );
		}

		[[nodiscard]] gumbo_node_iterator_t
		get_last_child( GumboNode const &parent_node ) {
			auto const child_count = get_children_count( parent_node );
			if( child_count == 0 ) {
				return gumbo_node_iterator_t( parent_node );
			}
			return std::next( gumbo_node_iterator_t(
			  get_child_node_at( parent_node, child_count - 1 ) ) );
		}
	} // namespace

	gumbo_child_range::gumbo_child_range( GumboNode const &parent_node )
	  : m_first( get_first_child( parent_node ) )
	  , m_last( get_last_child( parent_node ) ) {}
} // namespace daw::gumbo
