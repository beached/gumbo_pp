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
#include <daw/daw_string_view.h>

#include <cstddef>
#include <gumbo.h>
#include <iterator>

namespace daw::gumbo {
	struct gumbo_node_iterator_t {
		using difference_type = std::ptrdiff_t;
		using size_type = unsigned int;
		using value_type = std::remove_cv_t<GumboNode>;
		using pointer = GumboNode const *;
		using const_pointer = GumboNode const *;
		// Could be forward, but that allows calling std::distance when calling
		// insert or using iterator ctor's on Containers
		using iterator_category = std::input_iterator_tag;
		using reference = GumboNode const &;
		using const_reference = GumboNode const &;

	private:
		pointer m_node = nullptr;

	public:
		explicit constexpr gumbo_node_iterator_t( ) noexcept = default;

		explicit constexpr gumbo_node_iterator_t( GumboNode const *node ) noexcept
		  : m_node( node ) {}

		explicit constexpr gumbo_node_iterator_t( GumboNode const &node ) noexcept
		  : m_node( &node ) {}

		[[nodiscard]] constexpr gumbo_node_iterator_t begin( ) const {
			return *this;
		}

		[[nodiscard]] constexpr gumbo_node_iterator_t end( ) const {
			return gumbo_node_iterator_t{ };
		}

		[[nodiscard]] constexpr reference operator*( ) const noexcept {
			return *m_node;
		}

		[[nodiscard]] constexpr pointer get( ) const noexcept {
			return m_node;
		}

		[[nodiscard]] constexpr explicit operator bool( ) const noexcept {
			return static_cast<bool>( m_node );
		}

		[[nodiscard]] constexpr pointer operator->( ) const {
			return m_node;
		}

		constexpr gumbo_node_iterator_t &operator++( ) &noexcept {
			// iterate to lowest indexed child, then next children if any.  If no more
			// children move to parent's, next child.
			if( not m_node ) {
				return *this;
			}
			daw::not_null<pointer> cur_node = m_node;
			// Check if we have any children.  Will always be first because they will
			// iterate through their parents children
			if( get_children_count( *cur_node ) > 0 ) {
				if( pointer child_node = get_child_node_at( *cur_node, 0 );
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
				auto const max_idx = get_children_count( *parent );
				if( cur_idx + 1 < max_idx ) {
					// Parent node has more children left, choose next one
					pointer next_child = get_child_node_at( *parent, cur_idx + 1 );
					assert( next_child );
					m_node = next_child;
					return *this;
				}
				// The parent node has no more children, move up tree
				cur_node = parent.get( );
			}
		}

		[[nodiscard]] constexpr gumbo_node_iterator_t operator++( int ) &noexcept {
			gumbo_node_iterator_t result = *this;
			operator++( );
			return result;
		}

		[[nodiscard]] friend constexpr bool
		operator==( gumbo_node_iterator_t const &lhs,
		            gumbo_node_iterator_t const &rhs ) noexcept {
			return lhs.m_node == rhs.m_node;
		}

		[[nodiscard]] friend constexpr bool
		operator!=( gumbo_node_iterator_t const &lhs,
		            gumbo_node_iterator_t const &rhs ) noexcept {
			return lhs.m_node != rhs.m_node;
		}
	};

	class gumbo_range {
		GumboHandle m_handle;
		gumbo_node_iterator_t m_first{ };
		gumbo_node_iterator_t m_last{ };

	public:
		explicit gumbo_range( GumboHandle &&handle );
		explicit gumbo_range( daw::string_view html_document,
		                      GumboOptions options );
		explicit gumbo_range( daw::string_view html_document );

		[[nodiscard]] inline gumbo_node_iterator_t begin( ) const {
			return m_first;
		}

		[[nodiscard]] inline gumbo_node_iterator_t end( ) const {
			return m_last;
		}

		[[nodiscard]] inline GumboOutput *get( ) {
			return m_handle.get( );
		}

		[[nodiscard]] inline GumboNode *document( ) {
			return m_handle->document;
		}

		[[nodiscard]] inline GumboNode *root( ) {
			return m_handle->root;
		}

		[[nodiscard]] inline GumboVector errors( ) {
			return m_handle->errors;
		}
	};


	class gumbo_child_range {
		gumbo_node_iterator_t m_first{ };
		gumbo_node_iterator_t m_last{ };

	public:
		explicit gumbo_child_range( GumboNode const & parent_node );

		[[nodiscard]] inline gumbo_node_iterator_t begin( ) const {
			return m_first;
		}

		[[nodiscard]] inline gumbo_node_iterator_t end( ) const {
			return m_last;
		}
	};
} // namespace daw::gumbo
