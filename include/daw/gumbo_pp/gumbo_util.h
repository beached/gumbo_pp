// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/gumbo_pp
//

#pragma once

#include "details/gumbo_pp.h"
#include "gumbo_vector_iterator.h"

#include <daw/daw_string_view.h>

#include <gumbo.h>
#include <string>

namespace daw::gumbo {
	[[nodiscard]] inline GumboNode const *
	get_child_node_at( GumboNode const &node, std::size_t index ) noexcept {
		switch( node.type ) {
		case GumboNodeType::GUMBO_NODE_ELEMENT: {
			GumboNode **const ary =
			  reinterpret_cast<GumboNode **>( node.v.element.children.data );
			GumboNode *result = ary[index];
			return result;
		}
		case GumboNodeType::GUMBO_NODE_DOCUMENT: {
			GumboNode **const ary =
			  reinterpret_cast<GumboNode **>( node.v.document.children.data );
			GumboNode *result = ary[index];
			return result;
		}
		default:
			return nullptr;
		}
	}

	[[nodiscard]] inline GumboNode *
	get_child_node_at( GumboNode &node, std::size_t index ) noexcept {
		switch( node.type ) {
		case GumboNodeType::GUMBO_NODE_ELEMENT: {
			GumboNode **const ary =
			  reinterpret_cast<GumboNode **>( node.v.element.children.data );
			GumboNode *result = ary[index];
			return result;
		}
		case GumboNodeType::GUMBO_NODE_DOCUMENT: {
			GumboNode **const ary =
			  reinterpret_cast<GumboNode **>( node.v.document.children.data );
			GumboNode *result = ary[index];
			return result;
		}
		default:
			return nullptr;
		}
	}

	[[nodiscard]] constexpr std::size_t
	get_children_count( GumboNode const &node ) noexcept {
		switch( node.type ) {
		case GumboNodeType::GUMBO_NODE_ELEMENT:
			return node.v.element.children.length;
		case GumboNodeType::GUMBO_NODE_DOCUMENT:
			return node.v.document.children.length;
		default:
			return 0;
		}
	}

	template<typename Visitor>
	constexpr decltype( auto ) visit( GumboNode &node, Visitor vis ) {
		switch( node.type ) {
		case GumboNodeType::GUMBO_NODE_DOCUMENT:
			return vis( node.v.document );
		case GumboNodeType::GUMBO_NODE_ELEMENT:
			return vis( node.v.element );
		case GumboNodeType::GUMBO_NODE_TEXT:
		case GumboNodeType::GUMBO_NODE_CDATA:
		case GumboNodeType::GUMBO_NODE_COMMENT:
		case GumboNodeType::GUMBO_NODE_WHITESPACE:
		case GumboNodeType::GUMBO_NODE_TEMPLATE:
			return vis( node.v.text );
		}
	}

	template<typename Visitor>
	constexpr decltype( auto ) visit( GumboNode const &node, Visitor vis ) {
		switch( node.type ) {
		case GumboNodeType::GUMBO_NODE_DOCUMENT:
			return vis( node.v.document );
		case GumboNodeType::GUMBO_NODE_ELEMENT:
			return vis( node.v.element );
		case GumboNodeType::GUMBO_NODE_TEXT:
		case GumboNodeType::GUMBO_NODE_CDATA:
		case GumboNodeType::GUMBO_NODE_COMMENT:
		case GumboNodeType::GUMBO_NODE_WHITESPACE:
		case GumboNodeType::GUMBO_NODE_TEMPLATE:
			return vis( node.v.text );
		}
	}

	constexpr daw::string_view node_text( GumboNode const &node ) {
		switch( node.type ) {
		case GumboNodeType::GUMBO_NODE_ELEMENT:
			for( auto child : daw::gumbo::details::GumboVectorIterator(
			       node.v.element.children ) ) {
				if( child->type == GUMBO_NODE_TEXT ) {
					return { child->v.text.text };
				}
			}
			break;
		case GumboNodeType::GUMBO_NODE_DOCUMENT:
			for( auto child : daw::gumbo::details::GumboVectorIterator(
			       node.v.document.children ) ) {
				if( child->type == GUMBO_NODE_TEXT ) {
					return { child->v.text.text };
				}
			}
			break;
		case GumboNodeType::GUMBO_NODE_TEXT:
		case GumboNodeType::GUMBO_NODE_CDATA:
		case GumboNodeType::GUMBO_NODE_COMMENT:
		case GumboNodeType::GUMBO_NODE_WHITESPACE:
		case GumboNodeType::GUMBO_NODE_TEMPLATE:
		default:
			return { node.v.text.text };
		}
		return { };
	}

	constexpr unsigned node_start_offset( GumboNode const &node ) {
		switch( node.type ) {
		case GumboNodeType::GUMBO_NODE_ELEMENT: {
			return node.v.element.start_pos.offset;
		}
		case GumboNodeType::GUMBO_NODE_DOCUMENT: {
			return 0U;
		}
		case GumboNodeType::GUMBO_NODE_TEXT:
		case GumboNodeType::GUMBO_NODE_CDATA:
		case GumboNodeType::GUMBO_NODE_COMMENT:
		case GumboNodeType::GUMBO_NODE_WHITESPACE:
		case GumboNodeType::GUMBO_NODE_TEMPLATE:
		default:
			return node.v.text.start_pos.offset;
		}
	}

	constexpr daw::string_view node_inner_text( GumboNode const &node,
	                                            daw::string_view html_doc ) {
		switch( node.type ) {
		case GumboNodeType::GUMBO_NODE_ELEMENT: {
			auto const start_pos = [&] {
				if( get_children_count( node ) > 0 ) {
					// Use start offset of first child
					return node_start_offset( *get_child_node_at( node, 0U ) );
				}
				return node.v.element.start_pos.offset;
			}( );
			auto const length = node.v.element.end_pos.offset - start_pos;
			return html_doc.substr( start_pos, length );
		}
		case GumboNodeType::GUMBO_NODE_DOCUMENT: {
			return html_doc;
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

	constexpr daw::string_view
	node_attribute_value( GumboNode const &node, std::string const &attribute ) {
		if( node.type != GUMBO_NODE_ELEMENT ) {
			return { };
		}
		GumboAttribute *href =
		  gumbo_get_attribute( &node.v.element.attributes, attribute.c_str( ) );
		if( not href ) {
			return { };
		}
		return { href->value };
	}
} // namespace daw::gumbo
