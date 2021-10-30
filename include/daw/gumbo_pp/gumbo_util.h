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
		case GumboNodeType::GUMBO_NODE_DOCUMENT:
			for( auto child : daw::gumbo::details::GumboVectorIterator(
			       node.v.document.children ) ) {
				if( child->type == GUMBO_NODE_TEXT ) {
					return { child->v.text.text };
				}
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
