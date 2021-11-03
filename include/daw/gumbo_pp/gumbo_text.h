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
#include "gumbo_util.h"
#include "gumbo_vector_iterator.h"

#include <daw/daw_string_view.h>

#include <gumbo.h>
#include <string>

namespace daw::gumbo {
	inline std::string node_content_text( GumboNode const &node ) {
		switch( node.type ) {
		case GumboNodeType::GUMBO_NODE_ELEMENT: {
			std::string result{ };
			for( auto const &child : daw::gumbo::details::GumboVectorIterator(
			       node.v.element.children ) ) {
				if( child->type == GUMBO_NODE_TEXT ) {
					result += std::string_view( child->v.text.text );
				} else {
					result += node_content_text( *child );
				}
			}
			return result;
		}
		case GumboNodeType::GUMBO_NODE_DOCUMENT: {
			std::string result{ };
			for( auto const &child : daw::gumbo::details::GumboVectorIterator(
			       node.v.document.children ) ) {
				if( child->type == GUMBO_NODE_TEXT ) {
					result += std::string_view( child->v.text.text );
				} else {
					result += node_content_text( *child );
				}
			}
			return result;
		}
		default:
			return static_cast<std::string>( std::string_view( node.v.text.text ) );
		}
	}

	constexpr daw::string_view node_outer_text( GumboNode const &node,
	                                            daw::string_view html_doc ) {
		switch( node.type ) {
		case GumboNodeType::GUMBO_NODE_ELEMENT: {
			char const *start = node.v.element.original_tag.data;
			auto len = static_cast<std::size_t>(
			  node.v.element.original_end_tag.data +
			  node.v.element.original_end_tag.length - start );
			return daw::string_view( start, len );
		}
		case GumboNodeType::GUMBO_NODE_DOCUMENT: {
			return html_doc;
		}
		default:
			return { node.v.text.text };
		}
	}

	constexpr daw::string_view node_inner_text( GumboNode const &node,
	                                            daw::string_view html_doc ) {
		switch( node.type ) {
		case GumboNodeType::GUMBO_NODE_ELEMENT: {
			char const *start =
			  node.v.element.original_tag.data + node.v.element.original_tag.length;
			auto len = static_cast<std::size_t>(
			  node.v.element.original_end_tag.data - start );
			return daw::string_view( start, len );
		}
		case GumboNodeType::GUMBO_NODE_DOCUMENT: {
			auto const child_count = get_children_count( node );
			if( child_count == 0 ) {
				return { };
			}
			GumboNode const &first_child = *get_child_node_at( node, 0 );
			GumboNode const &last_child = *get_child_node_at( node, child_count - 1 );
			auto start_pos = node_start_offset( first_child );
			auto end_pos = node_end_offset( last_child );
			return daw::string_view( html_doc.data( ) + start_pos,
			                         end_pos - start_pos );
		}
		case GumboNodeType::GUMBO_NODE_TEXT:
		case GumboNodeType::GUMBO_NODE_CDATA:
		case GumboNodeType::GUMBO_NODE_COMMENT:
		case GumboNodeType::GUMBO_NODE_WHITESPACE:
		case GumboNodeType::GUMBO_NODE_TEMPLATE:
		default:
			return { node.v.text.text };
		}
	}
} // namespace daw::gumbo
