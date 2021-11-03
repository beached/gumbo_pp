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

	[[nodiscard]] inline GumboAttribute *
	get_attribute_node_at( GumboNode &node, std::size_t index ) noexcept {
		switch( node.type ) {
		case GumboNodeType::GUMBO_NODE_ELEMENT: {
			GumboAttribute **const ary =
			  reinterpret_cast<GumboAttribute **>( node.v.element.attributes.data );
			GumboAttribute *result = ary[index];
			return result;
		}
		default:
			return nullptr;
		}
	}

	[[nodiscard]] inline GumboAttribute const *
	get_attribute_node_at( GumboNode const &node, std::size_t index ) noexcept {
		switch( node.type ) {
		case GumboNodeType::GUMBO_NODE_ELEMENT: {
			GumboAttribute **const ary =
			  reinterpret_cast<GumboAttribute **>( node.v.element.attributes.data );
			GumboAttribute *result = ary[index];
			return result;
		}
		default:
			return nullptr;
		}
	}

	[[nodiscard]] constexpr std::size_t
	get_attribute_count( GumboNode const &node ) noexcept {
		switch( node.type ) {
		case GumboNodeType::GUMBO_NODE_ELEMENT:
			return node.v.element.attributes.length;
		default:
			return 0;
		}
	}

	[[nodiscard]] constexpr bool
	attribute_exists( GumboNode const &node, daw::string_view name ) noexcept {
		auto const sz = get_attribute_count( node );
		for( std::size_t n = 0; n < sz; ++n ) {
			if( daw::string_view( get_attribute_node_at( node, n )->name ) == name ) {
				return true;
			}
		}
		return false;
	}

	constexpr unsigned node_start_offset( GumboNode const &node ) {
		switch( node.type ) {
		case GumboNodeType::GUMBO_NODE_ELEMENT: {
			return node.v.element.start_pos.offset;
		}
		case GumboNodeType::GUMBO_NODE_DOCUMENT: {
			return 0U;
		}
		default:
			return node.v.text.start_pos.offset;
		}
	}

	constexpr unsigned node_end_offset( GumboNode const &node ) {
		switch( node.type ) {
		case GumboNodeType::GUMBO_NODE_ELEMENT: {
			return node.v.element.end_pos.offset;
		}
		case GumboNodeType::GUMBO_NODE_DOCUMENT: {
			return 0U;
		}
		default:
			return std::char_traits<char>::length( node.v.text.text );
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
