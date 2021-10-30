// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/gumbo_pp
//

#pragma once

#include "details/gumbo_pp.h"
#include "gumbo_util.h"

#include <daw/daw_not_null.h>

#include <cstddef>
#include <gumbo.h>
#include <iterator>

namespace daw::gumbo {
	struct gumbo_node_iterator_t {
		using difference_type = std::ptrdiff_t;
		using size_type = unsigned int;
		using value_type = GumboNode;
		using pointer = value_type *;
		using const_pointer = value_type const *;
		using iterator_category = std::input_iterator_tag;
		using reference = value_type &;
		using const_reference = value_type const &;

	private:
		pointer m_node = nullptr;

	public:
		constexpr gumbo_node_iterator_t( ) noexcept = default;

		constexpr gumbo_node_iterator_t( GumboNode *node ) noexcept
		  : m_node( node ) {}

		[[nodiscard]] constexpr reference operator*( ) const {
			return *m_node;
		}

		[[nodiscard]] constexpr pointer get( ) const {
			return m_node;
		}

		[[nodiscard]] constexpr pointer operator->( ) const {
			return m_node;
		}

	private:
		[[nodiscard]] static pointer
		get_child_node_at( daw::not_null<pointer> node,
		                   std::size_t index ) noexcept {
			switch( node->type ) {
			case GumboNodeType::GUMBO_NODE_ELEMENT: {
				pointer *const ary =
				  reinterpret_cast<pointer *>( node->v.element.children.data );
				pointer result = ary[index];
				return result;
			}
			case GumboNodeType::GUMBO_NODE_DOCUMENT: {
				pointer *const ary =
				  reinterpret_cast<pointer *>( node->v.document.children.data );
				pointer result = ary[index];
				return result;
			}
			default:
				return nullptr;
			}
		}

		[[nodiscard]] static std::size_t
		get_children_count( daw::not_null<pointer> node ) noexcept {
			switch( node->type ) {
			case GumboNodeType::GUMBO_NODE_ELEMENT:
				return node->v.element.children.length;
			case GumboNodeType::GUMBO_NODE_DOCUMENT:
				return node->v.document.children.length;
			default:
				return 0;
			}
		}

	public:
		constexpr gumbo_node_iterator_t &operator++( ) & {
			// iterate to lowest indexed child, then next children if any.  If no more
			// children move to parent's, next child.
			if( not m_node ) {
				return *this;
			}
			daw::not_null<pointer> cur_node = m_node;
			// Check if we have any children.  Will always be first because they will
			// iterate through their parents children
			if( get_children_count( cur_node ) > 0 ) {
				if( pointer child_node = get_child_node_at( cur_node, 0 );
				    child_node ) {
					m_node = child_node;
					return *this;
				}
			}

			// No child nodes, go to parent and look for it's next child
			while( true ) {
				if( not cur_node->parent ) {
					// We have no parent
					m_node = nullptr;
					return *this;
				}
				daw::not_null<pointer> parent = cur_node->parent;
				auto cur_idx = cur_node->index_within_parent;
				auto const max_idx = get_children_count( parent );
				if( cur_idx + 1 < max_idx ) {
					// Parent node has more children left, choose next one
					pointer next_child = get_child_node_at( parent, cur_idx + 1 );
					assert( next_child );
					m_node = next_child;
					return *this;
				}
				// The parent node has no more children, move up tree
				cur_node = parent.get( );
			}
		}

		[[nodiscard]] constexpr gumbo_node_iterator_t operator++( int ) & {
			gumbo_node_iterator_t result = *this;
			operator++( );
			return result;
		}

		[[nodiscard]] friend constexpr bool
		operator==( gumbo_node_iterator_t const &lhs,
		            gumbo_node_iterator_t const &rhs ) {
			return lhs.m_node == rhs.m_node;
		}

		[[nodiscard]] friend constexpr bool
		operator!=( gumbo_node_iterator_t const &lhs,
		            gumbo_node_iterator_t const &rhs ) {
			return lhs.m_node != rhs.m_node;
		}
	};
} // namespace daw::gumbo
