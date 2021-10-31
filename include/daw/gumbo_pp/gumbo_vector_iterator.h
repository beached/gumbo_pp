// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/gumbo_pp
//

#pragma once

#include "details/gumbo_pp.h"

#include <daw/daw_not_null.h>

#include <cstddef>
#include <gumbo.h>
#include <iterator>

namespace daw::gumbo::details {
	template<typename ChildType = GumboNode *>
	struct GumboVectorIterator {
		using difference_type = std::ptrdiff_t;
		using size_type = unsigned int;
		using value_type = ChildType;
		using pointer = value_type *;
		using const_pointer = value_type const *;
		using iterator_category = std::random_access_iterator_tag;
		using reference = value_type &;
		using const_reference = value_type const &;
		using values_type = daw::not_null<GumboVector const *>;

	private:
		values_type m_vector;
		size_type m_index;

	public:
		constexpr GumboVectorIterator( GumboVector const &vect )
		  : m_vector( &vect )
		  , m_index( 0 ) {}

		[[nodiscard]] constexpr GumboVectorIterator begin( ) const {
			return *this;
		}

		[[nodiscard]] constexpr GumboVectorIterator end( ) const {
			auto result = *this;
			result.m_index = m_vector->length;
			return result;
		}

		[[nodiscard]] inline pointer data( ) const {
			return reinterpret_cast<ChildType *>( m_vector->data );
		}

		[[nodiscard]] inline reference operator*( ) const {
			return data( )[m_index];
		}

		[[nodiscard]] inline pointer operator->( ) const {
			return data( );
		}

		constexpr GumboVectorIterator &operator++( ) {
			++m_index;
			return *this;
		}

		constexpr GumboVectorIterator operator++( int ) & {
			GumboVectorIterator result = *this;
			++m_index;
			return result;
		}

		constexpr GumboVectorIterator &operator--( ) {
			--m_index;
			return *this;
		}

		constexpr GumboVectorIterator operator--( int ) & {
			GumboVectorIterator result = *this;
			--m_index;
			return result;
		}

		constexpr GumboVectorIterator &operator+=( difference_type n ) {
			m_index += n;
			return *this;
		}

		constexpr GumboVectorIterator &operator-=( difference_type n ) {
			m_index -= n;
			return *this;
		}

		[[nodiscard]] constexpr GumboVectorIterator
		operator+( difference_type n ) const noexcept {
			GumboVectorIterator result = *this;
			result.m_index += n;
			return result;
		}

		[[nodiscard]] constexpr GumboVectorIterator
		operator-( difference_type n ) const noexcept {
			GumboVectorIterator result = *this;
			result.m_index -= n;
			return result;
		}

		[[nodiscard]] constexpr difference_type
		operator-( GumboVectorIterator const &rhs ) const noexcept {
			return static_cast<difference_type>( m_index ) -
			       static_cast<difference_type>( rhs.m_index );
		}

		[[nodiscard]] inline daw::not_null<ChildType>
		operator[]( size_type n ) noexcept {
			return reinterpret_cast<ChildType>( m_vector->data[m_index + n] );
		}

		[[nodiscard]] inline const_reference
		operator[]( size_type n ) const noexcept {
			return reinterpret_cast<ChildType>( m_vector->data[m_index + n] );
		}

		[[nodiscard]] friend constexpr bool
		operator==( GumboVectorIterator const &lhs,
		            GumboVectorIterator const &rhs ) {
			return lhs.m_index == rhs.m_index;
		}

		[[nodiscard]] friend constexpr bool
		operator!=( GumboVectorIterator const &lhs,
		            GumboVectorIterator const &rhs ) {
			return lhs.m_index != rhs.m_index;
		}

		[[nodiscard]] friend constexpr bool
		operator<( GumboVectorIterator const &lhs,
		           GumboVectorIterator const &rhs ) {
			return lhs.m_index < rhs.m_index;
		}

		[[nodiscard]] friend constexpr bool
		operator<=( GumboVectorIterator const &lhs,
		            GumboVectorIterator const &rhs ) {
			return lhs.m_index <= rhs.m_index;
		}

		[[nodiscard]] friend constexpr bool
		operator>( GumboVectorIterator const &lhs,
		           GumboVectorIterator const &rhs ) {
			return lhs.m_index > rhs.m_index;
		}

		[[nodiscard]] friend constexpr bool
		operator>=( GumboVectorIterator const &lhs,
		            GumboVectorIterator const &rhs ) {
			return lhs.m_index >= rhs.m_index;
		}
	};

	GumboVectorIterator( GumboVector const & ) -> GumboVectorIterator<>;
} // namespace daw::gumbo::details
