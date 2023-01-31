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
		class children_t {
			pointer node;
			friend class gumbo_node_iterator_t;

			explicit children_t( ) = default;
			explicit constexpr children_t( pointer n ) noexcept
			  : node( n ) {}
			~children_t() = default;
			children_t( children_t const & ) noexcept = default;
			children_t & operator=( children_t const & ) noexcept = default;
			children_t( children_t && ) noexcept = default;
			children_t & operator=( children_t && ) noexcept = default;


		public:
			constexpr std::size_t size( ) const {
				if( not node ) {
					return 0;
				}
				return get_children_count( *node );
			}

			inline reference operator[]( std::size_t idx ) const {
				assert( node );
				assert( idx < size( ) );
				return *get_child_node_at( *node, idx );
			}

			constexpr gumbo_node_iterator_t begin( ) const noexcept {
				if( not node ) {
					return gumbo_node_iterator_t( );
				}
				auto const child_count = get_children_count( *node );
				if( child_count == 0 ) {
					return gumbo_node_iterator_t{ };
				}
				return gumbo_node_iterator_t( get_child_node_at( *node, 0 ) );
			}

			constexpr gumbo_node_iterator_t end( ) const noexcept {
				if( not node ) {
					return gumbo_node_iterator_t( );
				}
				auto const child_count = get_children_count( *node );
				if( child_count == 0 ) {
					return gumbo_node_iterator_t{ };
				}
				auto *child = get_child_node_at( *node, child_count - 1 );
				assert( child );
				return std::next( gumbo_node_iterator_t( *child ) );
			}
		};

	public:
		children_t children{ };

		explicit gumbo_node_iterator_t( ) noexcept = default;

		explicit constexpr gumbo_node_iterator_t( GumboNode const *node ) noexcept
		  : children( node ) {}

		constexpr gumbo_node_iterator_t( GumboNode const &node ) noexcept
		  : children( &node ) {}

		[[nodiscard]] constexpr gumbo_node_iterator_t begin( ) const {
			return *this;
		}

		[[nodiscard]] constexpr gumbo_node_iterator_t end( ) const {
			return gumbo_node_iterator_t{ };
		}

		[[nodiscard]] constexpr reference operator*( ) const noexcept {
			assert( children.node );
			return *children.node;
		}

		[[nodiscard]] constexpr pointer get( ) const noexcept {
			return children.node;
		}

		[[nodiscard]] constexpr explicit operator bool( ) const noexcept {
			return static_cast<bool>( children.node );
		}

		[[nodiscard]] constexpr pointer operator->( ) const {
			assert( children.node );
			return children.node;
		}

		constexpr gumbo_node_iterator_t parent( ) const noexcept {
			if( not children.node or not children.node->parent ) {
				return gumbo_node_iterator_t( );
			}
			return gumbo_node_iterator_t( *children.node->parent );
		}

		constexpr gumbo_node_iterator_t next_sibling( ) const noexcept {
			auto cur_idx = children.node->parent->index_within_parent;
			auto *parent = children.node->parent;
			if( not parent ) {
				return gumbo_node_iterator_t( );
			}
			auto const max_idx = get_children_count( *parent );
			if( cur_idx + 1 < max_idx ) {
				// Parent node has more children left, choose next one
				pointer next_child = get_child_node_at( *parent, cur_idx + 1 );
				assert( next_child );
				return gumbo_node_iterator_t( next_child );
			}
			return gumbo_node_iterator_t{ };
		}

		inline gumbo_node_iterator_t last_sibling( ) const noexcept {
			auto *parent = children.node->parent;
			if( not parent ) {
				return gumbo_node_iterator_t( );
			}
			auto const max_idx = get_children_count( *parent );
			pointer next_child = get_child_node_at( *parent, max_idx - 1 );
			assert( next_child );
			return gumbo_node_iterator_t( next_child );
		}

		constexpr gumbo_node_iterator_t &operator++( ) &noexcept {
			// iterate to lowest indexed child, then next children if any.  If no
			// more children move to parent's, next child.
			if( not children.node ) {
				return *this;
			}
			daw::not_null<pointer> cur_node = children.node;
			// Check if we have any children.  Will always be first because they
			// will iterate through their parents children
			if( get_children_count( *cur_node ) > 0 ) {
				if( pointer child_node = get_child_node_at( *cur_node, 0 );
				    child_node ) {
					children.node = child_node;
					return *this;
				}
			}

			// No child nodes, go to parent and look for it's next child
			while( true ) {
				if( not cur_node->parent ) {
					// We have no parent
					children.node = nullptr;
					return *this;
				}
				daw::not_null<pointer> parent = cur_node->parent;
				auto cur_idx = cur_node->index_within_parent;
				auto const max_idx = get_children_count( *parent );
				if( cur_idx + 1 < max_idx ) {
					// Parent node has more children left, choose next one
					pointer next_child = get_child_node_at( *parent, cur_idx + 1 );
					assert( next_child );
					children.node = next_child;
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
			return lhs.get( ) == rhs.get( );
		}

		[[nodiscard]] friend constexpr bool
		operator!=( gumbo_node_iterator_t const &lhs,
		            gumbo_node_iterator_t const &rhs ) noexcept {
			return lhs.get( ) != rhs.get( );
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
		explicit gumbo_child_range( GumboNode const &parent_node );

		[[nodiscard]] inline gumbo_node_iterator_t begin( ) const {
			return m_first;
		}

		[[nodiscard]] inline gumbo_node_iterator_t end( ) const {
			return m_last;
		}
	};

	template<typename Predicate>
	constexpr void advance_until( gumbo_node_iterator_t &first,
	                              gumbo_node_iterator_t const &last,
	                              Predicate pred ) {
		while( first != last and not pred( *first ) ) {
			++first;
		}
	}
} // namespace daw::gumbo
