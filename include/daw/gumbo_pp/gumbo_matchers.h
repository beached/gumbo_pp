// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/gumbo_pp
//

#pragma once

#include "details/find_attrib_if_impl.h"
#include "details/gumbo_pp.h"
#include "gumbo_node_iterator.h"
#include "gumbo_text.h"

#include <daw/daw_logic.h>
#include <daw/daw_move.h>
#include <daw/daw_string_view.h>
#include <daw/daw_tuple2.h>

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

namespace daw::gumbo {
	// A combined predicate that returns true when all of it's predicates return
	// true
	template<typename... Matchers>
	struct match_all {
		daw::tuple2<Matchers...> m_matchers;

		constexpr match_all( daw::tuple2<Matchers...> &&m )
		  : m_matchers( std::move( m ) ) {}

		explicit constexpr match_all( Matchers &&...matchers )
		  : m_matchers{ std::move( matchers )... } {}

		explicit constexpr match_all( Matchers const &...matchers )
		  : m_matchers{ matchers... } {}

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const {
			return daw::apply( m_matchers, [&]( auto &&...matchers ) -> bool {
				return ( DAW_FWD( matchers )( node ) and ... );
			} );
		}

		template<typename Matcher>
		constexpr match_all<Matchers..., Matcher>
		append( Matcher const &other ) const {
			return { daw::tuple2_cat( m_matchers, daw::tuple2{ other } ) };
		}

		template<typename... Ms>
		constexpr match_all<Matchers..., Ms...>
		append( match_all<Ms...> const &other ) const {
			return { daw::tuple2_cat( m_matchers, other.m_matchers ) };
		}
	};
	template<typename... Matchers>
	match_all( Matchers... ) -> match_all<Matchers...>;

	// A combined predicate that returns true if any of the predicates return true
	template<typename... Matchers>
	struct match_any {
		daw::tuple2<Matchers...> m_matchers;

		constexpr match_any( daw::tuple2<Matchers...> &&m )
		  : m_matchers( std::move( m ) ) {}

		explicit constexpr match_any( Matchers &&...matchers )
		  : m_matchers{ std::move( matchers )... } {}

		explicit constexpr match_any( Matchers const &...matchers )
		  : m_matchers{ matchers... } {}

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const {
			return daw::apply( m_matchers, [&]( auto &&...matchers ) -> bool {
				return ( DAW_FWD( matchers )( node ) or ... );
			} );
		}

		template<typename Matcher>
		constexpr match_any<Matchers..., Matcher>
		append( Matcher const &other ) const {
			return { daw::tuple2_cat( m_matchers, daw::tuple2{ other } ) };
		}

		template<typename... Ms>
		constexpr match_any<Matchers..., Ms...>
		append( match_any<Ms...> const &other ) const {
			return { daw::tuple2_cat( m_matchers, other.m_matchers ) };
		}
	};

	template<typename... Matchers>
	match_any( Matchers... ) -> match_any<Matchers...>;

	template<typename... Matchers>
	struct match_one {
		daw::tuple2<Matchers...> m_matchers;

		constexpr match_one( daw::tuple2<Matchers...> &&m )
		  : m_matchers( std::move( m ) ) {}

		explicit constexpr match_one( Matchers &&...matchers )
		  : m_matchers{ std::move( matchers )... } {}

		explicit constexpr match_one( Matchers const &...matchers )
		  : m_matchers{ matchers... } {}

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const {
			return daw::apply( m_matchers, [&]( auto &&...matchers ) -> bool {
				return ( static_cast<bool>( DAW_FWD( matchers )( node ) ) ^ ... );
			} );
		}

		template<typename Matcher>
		constexpr match_one<Matchers..., Matcher>
		append( Matcher const &other ) const {
			return { daw::tuple2_cat( m_matchers, daw::tuple2{ other } ) };
		}

		template<typename... Ms>
		constexpr match_one<Matchers..., Ms...>
		append( match_one<Ms...> const &other ) const {
			return { daw::tuple2_cat( m_matchers, other.m_matchers ) };
		}
	};
	template<typename... Matchers>
	match_one( Matchers... ) -> match_one<Matchers...>;

	template<typename Matcher>
	class match_not : private Matcher {

	public:
		explicit constexpr match_not( Matcher &&matcher )
		  : Matcher{ std::move( matcher ) } {}

		explicit constexpr match_not( Matcher const &matcher )
		  : Matcher{ matcher } {}

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const {
			return not Matcher::operator( )( node );
		}
	};
	template<typename Matcher>
	match_not( Matcher ) -> match_not<Matcher>;

} // namespace daw::gumbo
namespace daw::gumbo::match_details {
	namespace match_attribute {
		/// Match any node that has an attribute where all the predicates returns
		/// true
		template<typename Predicate, typename... Predicates>
		constexpr auto where( Predicate &&pred, Predicates &&...preds ) noexcept {
			return [=]( GumboNode const &node ) -> bool {
				return details::find_attribute_if_impl(
				         gumbo_node_iterator_t( &node ),
				         [&]( GumboAttribute const &attr ) -> bool {
					         auto name = daw::string_view( attr.name );
					         auto value = daw::string_view( attr.value );
					         return pred( name, value ) and
					                ( preds( name, value ) and ... );
				         } )
				  .found;
			};
		}

		/// Match any node that does not have any attributes
		constexpr auto has_none = []( auto const &node ) {
			return get_attribute_count( node ) == 0;
		};

		/// Match any node that has attributes
		constexpr auto has = []( auto const &node ) {
			return get_attribute_count( node ) != 0;
		};

		/// Match any node that has any of these attributes
		template<typename Container,
		         std::enable_if_t<
		           daw::traits::is_container_like_v<daw::remove_cvref_t<Container>>,
		           std::nullptr_t> = nullptr>
		constexpr auto exists( Container &&c ) noexcept {
			return [=]( auto const &node ) {
				auto first = std::begin( c );
				auto last = std::end( c );
				if( first == last ) {
					return false;
				}
				return std::find_if( first, last, [&]( daw::string_view name ) {
					       return attribute_exists( node, name );
				       } ) != last;
			};
		}

		/// Match any node that has any of these attributes
		template<typename... StringView>
		constexpr auto exists( daw::string_view name,
		                       StringView &&...names ) noexcept {
			return [=]( auto const &node ) {
				return attribute_exists( node, name ) or
				       ( attribute_exists( node, names ) or ... );
			};
		}

		namespace name {
			// Match any node that has a matching attribute name
			template<typename Container,
			         std::enable_if_t<daw::traits::is_container_like_v<
			                            daw::remove_cvref_t<Container>>,
			                          std::nullptr_t> = nullptr>
			constexpr auto is( Container &&c ) noexcept {
				return where( [=]( daw::string_view name, daw::string_view ) noexcept {
					auto first = std::begin( c );
					auto last = std::end( c );
					if( first == last ) {
						return false;
					}
					return std::find_if( first, last, [&]( daw::string_view n ) {
						       return n == name;
					       } ) != last;
				} );
			}
			// Match any node that has a matching attribute name
			template<typename... StringView>
			constexpr auto is( daw::string_view attribute_name,
			                   StringView &&...attribute_names ) noexcept {
				return where( [=]( daw::string_view name, daw::string_view ) noexcept {
					return ( name == attribute_name ) or
					       ( ( name == attribute_names ) or ... );
				} );
			}
		} // namespace name

		namespace value {
			/// Match any node with named attribute who's value is either value or
			/// prefixed by value and a hyphen `-`
			constexpr auto contains_prefix( daw::string_view attribute_name,
			                                daw::string_view value_prefix ) noexcept {
				return where(
				  [=]( daw::string_view name, daw::string_view value ) noexcept {
					  if( name != attribute_name ) {
						  return false;
					  }
					  if( not value.starts_with( value_prefix ) ) {
						  return false;
					  }
					  if( value_prefix.size( ) == value.size( ) ) {
						  return true;
					  }
					  return value.substr( value_prefix.size( ) ).starts_with( '-' );
				  } );
			}

			/// Match any node with named attribute who's value contains the
			/// specified value
			template<typename Container,
			         std::enable_if_t<daw::traits::is_container_like_v<
			                            daw::remove_cvref_t<Container>>,
			                          std::nullptr_t> = nullptr>
			constexpr auto contains( daw::string_view attribute_name,
			                         Container &&value_substrs ) noexcept {
				return where(
				  [=]( daw::string_view name, daw::string_view value ) noexcept {
					  auto first = std::begin( value_substrs );
					  auto last = std::end( value_substrs );
					  if( first == last ) {
						  return false;
					  }
					  return name == attribute_name and
					         std::find_if(
					           first, last, [&]( daw::string_view value_substr ) {
						           return value.find( value_substr ) !=
						                  daw::string_view::npos;
					           } ) != last;
				  } );
			}

			/// Match any node with named attribute who's value contains the
			/// specified value
			template<typename... StringView>
			constexpr auto contains( daw::string_view attribute_name,
			                         daw::string_view value_substr,
			                         StringView &&...value_substrs ) noexcept {
				return where(
				  [=]( daw::string_view name, daw::string_view value ) noexcept {
					  return name == attribute_name and
					         ( ( value.find( value_substr ) != daw::string_view::npos ) or
					           ( ( value.find( value_substrs ) !=
					               daw::string_view::npos ) or
					             ... ) );
				  } );
			}

			/// Match any node with named attribute who's value starts with one of the
			/// specified values
			template<typename Container,
			         std::enable_if_t<daw::traits::is_container_like_v<
			                            daw::remove_cvref_t<Container>>,
			                          std::nullptr_t> = nullptr>
			constexpr auto starts_with( daw::string_view attribute_name,
			                            Container &&value_prefixes ) noexcept {
				return where(
				  [=]( daw::string_view name, daw::string_view value ) noexcept {
					  auto first = std::begin( value_prefixes );
					  auto last = std::end( value_prefixes );
					  if( first == last ) {
						  return false;
					  }
					  return name == attribute_name and
					         std::find_if(
					           first, last, [&]( daw::string_view value_prefix ) {
						           return value.starts_with( value_prefix );
					           } ) != last;
				  } );
			}

			/// Match any node with named attribute who's value starts with one of the
			/// specified values
			template<typename... StringView>
			constexpr auto starts_with( daw::string_view attribute_name,
			                            daw::string_view value_prefix,
			                            StringView &&...value_prefixes ) noexcept {
				return where(
				  [=]( daw::string_view name, daw::string_view value ) noexcept {
					  return name == attribute_name and
					         ( value.starts_with( value_prefix ) or
					           ( value.starts_with( value_prefixes ) or ... ) );
				  } );
			}

			/// Match any node with named attribute who's value ends with one of the
			/// specified values
			template<typename Container,
			         std::enable_if_t<daw::traits::is_container_like_v<
			                            daw::remove_cvref_t<Container>>,
			                          std::nullptr_t> = nullptr>
			constexpr auto ends_with( daw::string_view attribute_name,
			                          Container &&value_prefixes ) noexcept {
				return where(
				  [=]( daw::string_view name, daw::string_view value ) noexcept {
					  auto first = std::begin( value_prefixes );
					  auto last = std::end( value_prefixes );
					  if( first == last ) {
						  return false;
					  }
					  return name == attribute_name and
					         std::find_if(
					           first, last, [&]( daw::string_view value_prefix ) {
						           return value.ends_with( value_prefix );
					           } ) != last;
				  } );
			}

			/// Match any node with named attribute who's value end with the
			/// specified value
			template<typename... StringView>
			constexpr auto ends_with( daw::string_view attribute_name,
			                          daw::string_view value_prefix,
			                          StringView &&...value_prefixes ) noexcept {
				return where(
				  [=]( daw::string_view name, daw::string_view value ) noexcept {
					  return name == attribute_name and
					         ( value.ends_with( value_prefix ) or
					           ( value.ends_with( value_prefixes ) or ... ) );
				  } );
			}

			/// Match any node with named attribute who's value equals to one of the
			/// specified values
			template<typename Container,
			         std::enable_if_t<daw::traits::is_container_like_v<
			                            daw::remove_cvref_t<Container>>,
			                          std::nullptr_t> = nullptr>
			constexpr auto is( daw::string_view attribute_name,
			                   Container &&attribute_values ) noexcept {
				return where(
				  [=]( daw::string_view name, daw::string_view value ) noexcept {
					  auto first = std::begin( attribute_values );
					  auto last = std::end( attribute_values );
					  if( first == last ) {
						  return false;
					  }
					  return name == attribute_name and
					         std::find( first, last, value ) != last;
				  } );
			}

			/// Match any node with named attribute who's value equals to one of the
			/// specified values
			template<typename... StringView>
			constexpr auto is( daw::string_view attribute_name,
			                   daw::string_view attribute_value,
			                   StringView &&...attribute_values ) noexcept {
				return where(
				  [=]( daw::string_view name, daw::string_view value ) noexcept {
					  return name == attribute_name and
					         ( value == attribute_value or
					           ( ( value == attribute_values ) or ... ) );
				  } );
			}

			/// Match any node with named attribute who's value is empty
			constexpr auto is_empty( daw::string_view attribute_name ) noexcept {
				return where( [attribute_name]( daw::string_view name,
				                                daw::string_view value ) noexcept {
					return name == attribute_name and value.empty( ) and value.data( );
				} );
			}

			/// Match any node with named attribute who's value is null
			constexpr auto is_null( daw::string_view attribute_name ) noexcept {
				return where( [attribute_name]( daw::string_view name,
				                                daw::string_view value ) noexcept {
					return name == attribute_name and not value.data( );
				} );
			}

			/// Match any node with named attribute who's value is not empty
			constexpr auto has_value( daw::string_view attribute_name ) noexcept {
				return where( [attribute_name]( daw::string_view name,
				                                daw::string_view value ) noexcept {
					return name == attribute_name and not value.empty( );
				} );
			}
		} // namespace value
	}   // namespace match_attribute

	namespace match_class {
		/// Match any node with a class that returns true for all the predicates
		template<typename Predicate, typename... Predicates>
		constexpr auto where( Predicate &&pred, Predicates &&...preds ) noexcept {
			return match_attribute::where(
			  [=]( daw::string_view attribute_name,
			       daw::string_view attribute_value ) noexcept -> bool {
				  return attribute_name == "class" and pred( attribute_value ) and
				         ( preds( attribute_value ) and ... );
			  } );
		}

		template<typename Container,
		         std::enable_if_t<
		           daw::traits::is_container_like_v<daw::remove_cvref_t<Container>>,
		           std::nullptr_t> = nullptr>
		constexpr auto is( daw::string_view attribute_name,
		                   Container &&attribute_values ) noexcept {
			return where(
			  [=]( daw::string_view name, daw::string_view value ) noexcept {
				  auto first = std::begin( attribute_values );
				  auto last = std::end( attribute_values );
				  if( first == last ) {
					  return false;
				  }
				  return name == attribute_name and
				         std::find( first, last, value ) != last;
			  } );
		}

		/// Match any node with a class named that is one of the following
		template<typename... StringView>
		constexpr auto is( daw::string_view class_name,
		                   StringView &&...class_names ) noexcept {
			return match_attribute::where(
			  [=]( daw::string_view attribute_name,
			       daw::string_view attribute_value ) noexcept -> bool {
				  return attribute_name == "class" and
				         ( attribute_value == class_name or
				           ( ( attribute_value == class_names ) or ... ) );
			  } );
		}
	} // namespace match_class

	namespace match_id {
		/// Match any node with a id that returns true for the predicate
		template<typename Predicate, typename... Predicates>
		constexpr auto where( Predicate &&pred, Predicates &&...preds ) noexcept {
			return match_attribute::where(
			  [=]( daw::string_view attribute_name,
			       daw::string_view attribute_value ) noexcept -> bool {
				  return attribute_name == "id" and pred( attribute_value ) and
				         ( preds( attribute_value ) and ... );
			  } );
		}

		/// Match any node with a id that is any of the following names
		template<typename... StringView>
		constexpr auto is( daw::string_view id_name,
		                   StringView &&...id_names ) noexcept {
			return match_attribute::where(
			  [=]( daw::string_view attribute_name,
			       daw::string_view attribute_value ) noexcept -> bool {
				  return attribute_name == "id" and
				         ( attribute_value == id_name or
				           ( ( attribute_value == id_names ) or ... ) );
			  } );
		}
	} // namespace match_id

	// For matching the content of tags like A
	namespace match_content_text {
		/// Match any node with a id that returns true for the predicates
		template<typename Predicate, typename... Predicates>
		constexpr auto where( Predicate &&pred, Predicates &&...preds ) noexcept {
			return [=]( auto const &node ) -> bool {
				auto txt = node_content_text( node );
				daw::string_view sv = txt;
				return pred( sv ) and ( preds( sv ) and ... );
			};
		}

		template<typename Map, typename Predicate>
		constexpr auto map( Map &&map, Predicate &&pred ) noexcept {
			return [=]( auto const &node ) -> bool {
				auto txt = node_content_text( node );
				daw::string_view sv = txt;
				return pred( map( sv ) );
			};
		}

		template<typename Container,
		         std::enable_if_t<
		           daw::traits::is_container_like_v<daw::remove_cvref_t<Container>>,
		           std::nullptr_t> = nullptr>
		constexpr auto contains( Container &&c ) noexcept {
			return [=]( auto const &node ) noexcept {
				auto text = node_content_text( node );
				if( text.empty( ) ) {
					return false;
				}
				auto first = std::begin( c );
				auto last = std::end( c );
				auto const fpos =
				  std::find_if( first, last, [&]( daw::string_view cur_text ) {
					  return text.find( static_cast<std::string_view>( cur_text ) ) !=
					         std::string::npos;
				  } );
				return fpos != last;
			};
		}

		template<typename... StringView>
		constexpr auto contains( daw::string_view search_text,
		                         StringView &&...search_texts ) noexcept {
			return [=]( auto const &node ) noexcept -> bool {
				auto text = node_content_text( node );
				return ( text.find( search_text ) != std::string::npos ) or
				       ( ( text.find( search_texts ) != std::string::npos ) or ... );
			};
		}

		inline constexpr auto is_empty = []( auto const &node ) noexcept -> bool {
			return node_content_text( node ).empty( );
		};

		/// Match any node with outer text who's value starts with and of the
		/// specified values
		template<typename Container,
		         std::enable_if_t<
		           daw::traits::is_container_like_v<daw::remove_cvref_t<Container>>,
		           std::nullptr_t> = nullptr>
		constexpr auto starts_with( Container &&c ) noexcept {
			return where( [=]( daw::string_view text ) noexcept {
				auto first = std::begin( c );
				auto last = std::end( c );
				return std::find_if( first, last, [&]( daw::string_view prefix_text ) {
					       return text.starts_with( prefix_text );
				       } ) != last;
			} );
		}

		/// Match any node with outer text who's value starts with and of the
		/// specified values
		template<typename... StringView>
		constexpr auto starts_with( daw::string_view prefix_text,
		                            StringView &&...prefix_texts ) noexcept {
			return where( [=]( daw::string_view text ) noexcept -> bool {
				return text.starts_with( prefix_text ) or
				       ( text.starts_with( prefix_texts ) or ... );
			} );
		}

		/// Match any node with outer text who's value ends with and of the
		/// specified values
		template<typename Container,
		         std::enable_if_t<
		           daw::traits::is_container_like_v<daw::remove_cvref_t<Container>>,
		           std::nullptr_t> = nullptr>
		constexpr auto ends_with( Container &&c ) noexcept {
			return where( [=]( daw::string_view text ) noexcept {
				auto first = std::begin( c );
				auto last = std::end( c );
				return std::find_if( first, last, [&]( daw::string_view suffix_text ) {
					       return text.ends_with( suffix_text );
				       } ) != last;
			} );
		}

		/// Match any node with outer text who's value ends with the specified
		/// value
		template<typename... StringView>
		constexpr auto ends_with( daw::string_view prefix_text,
		                          StringView &&...prefix_texts ) noexcept {
			return where( [=]( daw::string_view text ) noexcept -> bool {
				return text.starts_with( prefix_text ) or
				       ( text.ends_with( prefix_texts ) or ... );
			} );
		}
		/// Match any node with outer text who's value is equal to on of the
		/// specified values
		template<typename Container,
		         std::enable_if_t<
		           daw::traits::is_container_like_v<daw::remove_cvref_t<Container>>,
		           std::nullptr_t> = nullptr>
		constexpr auto is( Container &&c ) noexcept {
			return where( [=]( daw::string_view text ) noexcept {
				auto first = std::begin( c );
				auto last = std::end( c );
				return std::find_if( first, last, [&]( daw::string_view cur_text ) {
					       return text = cur_text;
				       } ) != last;
			} );
		}

		/// Match any node with outer text who's value is equal to on of the
		/// specified values
		template<typename... StringView>
		constexpr auto is( daw::string_view match_text,
		                   StringView &&...match_texts ) noexcept {
			return where( [=]( daw::string_view text ) noexcept -> bool {
				return ( text == match_text ) or ( ( text == match_texts ) or ... );
			} );
		}
	} // namespace match_content_text

	namespace match_inner_text {
		/// Match any node with inner_text that returns true for the predicate
		template<typename Predicate, typename... Predicates>
		constexpr auto where( daw::string_view html_doc,
		                      Predicate &&pred,
		                      Predicates &&...preds ) noexcept {
			return [=]( auto const &node ) -> bool {
				daw::string_view text = node_inner_text( node, html_doc );
				return pred( text ) and ( preds( text ) and ... );
			};
		}

		/// Match any node with inner_text that is empty
		constexpr auto is_empty( daw::string_view html_doc ) noexcept {
			return where( html_doc, []( daw::string_view text ) noexcept -> bool {
				return text.empty( );
			} );
		}

		/// Match any node with inner_text that contains search_text
		template<typename Container,
		         std::enable_if_t<
		           daw::traits::is_container_like_v<daw::remove_cvref_t<Container>>,
		           std::nullptr_t> = nullptr>
		constexpr auto contains( daw::string_view html_doc,
		                         Container &&c ) noexcept {
			return where( html_doc, [=]( daw::string_view text ) noexcept {
				auto first = std::begin( c );
				auto last = std::end( c );
				return std::find_if( first, last, [&]( daw::string_view search_text ) {
					       return text.find( search_text ) != daw::string_view::npos;
				       } ) != last;
			} );
		}

		/// Match any node with inner_text that contains search_text
		template<typename... StringView>
		constexpr auto contains( daw::string_view html_doc,
		                         daw::string_view search_text,
		                         StringView &&...search_texts ) noexcept {
			return where( html_doc, [=]( daw::string_view text ) noexcept {
				return ( text.find( search_text ) != daw::string_view::npos ) or
				       ( ( text.find( search_texts ) != daw::string_view::npos ) or
				         ... );
			} );
		}

		/// Match any node with inner text who's value starts with the specified
		/// values
		template<typename Container,
		         std::enable_if_t<
		           daw::traits::is_container_like_v<daw::remove_cvref_t<Container>>,
		           std::nullptr_t> = nullptr>
		constexpr auto starts_with( daw::string_view html_doc,
		                            Container &&c ) noexcept {
			return where( html_doc, [=]( daw::string_view text ) noexcept {
				auto first = std::begin( c );
				auto last = std::end( c );
				return std::find_if( first, last, [&]( daw::string_view prefix_text ) {
					return text.starts_with( prefix_text );
				} );
			} );
		}

		/// Match any node with inner text who's value starts with the specified
		/// values
		constexpr auto starts_with( daw::string_view html_doc,
		                            daw::string_view prefix_text ) noexcept {
			return where( html_doc, [prefix_text]( daw::string_view text ) noexcept {
				return text.starts_with( prefix_text );
			} );
		}

		/// Match any node with inner text who's value ends with the specified
		/// values
		template<typename Container,
		         std::enable_if_t<
		           daw::traits::is_container_like_v<daw::remove_cvref_t<Container>>,
		           std::nullptr_t> = nullptr>
		constexpr auto ends_with( daw::string_view html_doc,
		                          Container &&c ) noexcept {
			return where( html_doc, [=]( daw::string_view text ) noexcept {
				auto first = std::begin( c );
				auto last = std::end( c );
				return std::find_if( first, last, [&]( daw::string_view suffix_text ) {
					return text.ends_with( suffix_text );
				} );
			} );
		}

		/// Match any node with inner text who's value ends with the specified
		/// values
		template<typename... StringView>
		constexpr auto ends_with( daw::string_view html_doc,
		                          daw::string_view suffix_text,
		                          StringView &&...suffix_texts ) noexcept {
			return where( html_doc, [=]( daw::string_view text ) noexcept {
				return text.ends_with( suffix_text ) or
				       ( text.ends_with( suffix_texts ) or ... );
			} );
		}

		/// Match any node with inner text who's value is equal to the specified
		/// values
		template<typename Container,
		         std::enable_if_t<
		           daw::traits::is_container_like_v<daw::remove_cvref_t<Container>>,
		           std::nullptr_t> = nullptr>
		constexpr auto is( daw::string_view html_doc, Container &&c ) noexcept {
			return where( html_doc, [=]( daw::string_view text ) noexcept {
				auto first = std::begin( c );
				auto last = std::end( c );
				return std::find_if( first, last, [&]( daw::string_view suffix_text ) {
					return text == suffix_text;
				} );
			} );
		}

		/// Match any node with inner text who's value is equal to the specified
		/// values
		template<typename... StringView>
		constexpr auto is( daw::string_view html_doc,
		                   daw::string_view suffix_text,
		                   StringView &&...suffix_texts ) noexcept {
			return where( html_doc, [=]( daw::string_view text ) noexcept {
				return text == suffix_text or ( ( text == suffix_texts ) or ... );
			} );
		}
	} // namespace match_inner_text

	namespace match_outer_text {
		/// Match any node with outer_text that returns true for the predicates
		template<typename Predicate, typename... Predicates>
		constexpr auto where( daw::string_view html_doc,
		                      Predicate &&pred,
		                      Predicates &&...preds ) noexcept {
			return [=]( auto const &node ) -> bool {
				auto text = node_outer_text( node, html_doc );
				return pred( text ) and ( preds( text ) and ... );
			};
		}

		/// Match any node with outer_text that is empty
		constexpr auto is_empty( daw::string_view html_doc ) noexcept {
			return where( html_doc, []( daw::string_view text ) noexcept -> bool {
				return text.empty( );
			} );
		}

		/// Match any node with outer_text that contains match_text
		template<typename Container,
		         std::enable_if_t<
		           daw::traits::is_container_like_v<daw::remove_cvref_t<Container>>,
		           std::nullptr_t> = nullptr>
		constexpr auto contains( Container &&c ) noexcept {
			return where( [=]( daw::string_view text ) noexcept {
				auto first = std::begin( c );
				auto last = std::end( c );
				return std::find_if( first, last, [&]( daw::string_view match_text ) {
					return text.find( match_text ) != daw::string_view::npos;
				} );
			} );
		}

		/// Match any node with outer_text that contains match_text
		template<typename... StringView>
		constexpr auto contains( daw::string_view html_doc,
		                         daw::string_view match_text,
		                         StringView &&...match_texts ) noexcept {
			return where( html_doc, [=]( daw::string_view text ) noexcept {
				return ( text.find( match_text ) != daw::string_view::npos ) or
				       ( ( text.find( match_texts ) != daw::string_view::npos ) or
				         ... );
			} );
		}

		/// Match any node with outer text who's value starts with the specified
		/// values
		template<typename Container,
		         std::enable_if_t<
		           daw::traits::is_container_like_v<daw::remove_cvref_t<Container>>,
		           std::nullptr_t> = nullptr>
		constexpr auto starts_with( Container &&c ) noexcept {
			return where( [=]( daw::string_view text ) noexcept {
				auto first = std::begin( c );
				auto last = std::end( c );
				return std::find_if( first, last, [&]( daw::string_view match_text ) {
					return text.template starts_with( match_text );
				} );
			} );
		}

		/// Match any node with outer text who's value starts with the specified
		/// values
		template<typename... StringView>
		constexpr auto starts_with( daw::string_view html_doc,
		                            daw::string_view match_text,
		                            StringView &&...match_texts ) noexcept {
			return where( html_doc, [=]( daw::string_view text ) noexcept {
				return text.starts_with( match_text ) or
				       ( text.starts_with( match_texts ) or ... );
			} );
		}

		/// Match any node with outer text who's value ends with the specified
		/// values
		template<typename Container,
		         std::enable_if_t<
		           daw::traits::is_container_like_v<daw::remove_cvref_t<Container>>,
		           std::nullptr_t> = nullptr>
		constexpr auto ends_with( Container &&c ) noexcept {
			return where( [=]( daw::string_view text ) noexcept {
				auto first = std::begin( c );
				auto last = std::end( c );
				return std::find_if( first, last, [&]( daw::string_view match_text ) {
					return text.template ends_with( match_text );
				} );
			} );
		}

		/// Match any node with outer text who's value ends with the specified
		/// values
		template<typename... StringView>
		constexpr auto ends_with( daw::string_view html_doc,
		                          daw::string_view match_text,
		                          StringView &&...match_texts ) noexcept {
			return where( html_doc, [=]( daw::string_view text ) noexcept {
				return text.ends_with( match_text ) or
				       ( text.ends_with( match_texts ) or ... );
			} );
		}

		/// Match any node with outer text who's value is equal to the specified
		/// values
		template<typename Container,
		         std::enable_if_t<
		           daw::traits::is_container_like_v<daw::remove_cvref_t<Container>>,
		           std::nullptr_t> = nullptr>
		constexpr auto is( Container &&c ) noexcept {
			return where( [=]( daw::string_view text ) noexcept {
				auto first = std::begin( c );
				auto last = std::end( c );
				return std::find_if( first, last, [&]( daw::string_view match_text ) {
					return text == match_text;
				} );
			} );
		}

		/// Match any node with outer text who's value is equal to the specified
		/// values
		template<typename... StringView>
		constexpr auto is( daw::string_view html_doc,
		                   daw::string_view match_text,
		                   StringView &&...match_texts ) noexcept {
			return where( html_doc, [=]( daw::string_view text ) noexcept {
				return text == match_text or ( ( text == match_texts ) or ... );
			} );
		}
	} // namespace match_outer_text

	namespace match_tag {
		/// Match any node with where the tag type where all the Predicates return
		/// true
		template<typename Predicate, typename... Predicates>
		constexpr auto where( Predicate &&pred, Predicates &&...preds ) noexcept {
			return [=]( auto const &node ) noexcept -> bool {
				if( node.type != GUMBO_NODE_ELEMENT ) {
					return false;
				}
				auto tag_value = node.v.element.tag;
				return pred( tag_value ) and ( preds( tag_value ) and ... );
			};
		}

		/// Match any node with where the tag type where the tag type matches on
		/// of the specified types
		template<GumboTag... tags>
		inline constexpr auto types = []( GumboNode const &node ) -> bool {
			if( node.type != GUMBO_NODE_ELEMENT ) {
				return false;
			}
			GumboTag const tag_value = node.v.element.tag;
			return ( ( tag_value == tags ) | ... );
		};

		inline constexpr auto HTML = types<GumboTag::GUMBO_TAG_HTML>;
		inline constexpr auto HEAD = types<GumboTag::GUMBO_TAG_HEAD>;
		inline constexpr auto TITLE = types<GumboTag::GUMBO_TAG_TITLE>;
		inline constexpr auto BASE = types<GumboTag::GUMBO_TAG_BASE>;
		inline constexpr auto LINK = types<GumboTag::GUMBO_TAG_LINK>;
		inline constexpr auto META = types<GumboTag::GUMBO_TAG_META>;
		inline constexpr auto STYLE = types<GumboTag::GUMBO_TAG_STYLE>;
		inline constexpr auto SCRIPT = types<GumboTag::GUMBO_TAG_SCRIPT>;
		inline constexpr auto NOSCRIPT = types<GumboTag::GUMBO_TAG_NOSCRIPT>;
		inline constexpr auto TEMPLATE = types<GumboTag::GUMBO_TAG_TEMPLATE>;
		inline constexpr auto BODY = types<GumboTag::GUMBO_TAG_BODY>;
		inline constexpr auto ARTICLE = types<GumboTag::GUMBO_TAG_ARTICLE>;
		inline constexpr auto SECTION = types<GumboTag::GUMBO_TAG_SECTION>;
		inline constexpr auto NAV = types<GumboTag::GUMBO_TAG_NAV>;
		inline constexpr auto ASIDE = types<GumboTag::GUMBO_TAG_ASIDE>;
		inline constexpr auto H1 = types<GumboTag::GUMBO_TAG_H1>;
		inline constexpr auto H2 = types<GumboTag::GUMBO_TAG_H2>;
		inline constexpr auto H3 = types<GumboTag::GUMBO_TAG_H3>;
		inline constexpr auto H4 = types<GumboTag::GUMBO_TAG_H4>;
		inline constexpr auto H5 = types<GumboTag::GUMBO_TAG_H5>;
		inline constexpr auto H6 = types<GumboTag::GUMBO_TAG_H6>;
		inline constexpr auto HGROUP = types<GumboTag::GUMBO_TAG_HGROUP>;
		inline constexpr auto HEADER = types<GumboTag::GUMBO_TAG_HEADER>;
		inline constexpr auto FOOTER = types<GumboTag::GUMBO_TAG_FOOTER>;
		inline constexpr auto ADDRESS = types<GumboTag::GUMBO_TAG_ADDRESS>;
		inline constexpr auto P = types<GumboTag::GUMBO_TAG_P>;
		inline constexpr auto HR = types<GumboTag::GUMBO_TAG_HR>;
		inline constexpr auto PRE = types<GumboTag::GUMBO_TAG_PRE>;
		inline constexpr auto BLOCKQUOTE = types<GumboTag::GUMBO_TAG_BLOCKQUOTE>;
		inline constexpr auto OL = types<GumboTag::GUMBO_TAG_OL>;
		inline constexpr auto UL = types<GumboTag::GUMBO_TAG_UL>;
		inline constexpr auto LI = types<GumboTag::GUMBO_TAG_LI>;
		inline constexpr auto DL = types<GumboTag::GUMBO_TAG_DL>;
		inline constexpr auto DT = types<GumboTag::GUMBO_TAG_DT>;
		inline constexpr auto DD = types<GumboTag::GUMBO_TAG_DD>;
		inline constexpr auto FIGURE = types<GumboTag::GUMBO_TAG_FIGURE>;
		inline constexpr auto FIGCAPTION = types<GumboTag::GUMBO_TAG_FIGCAPTION>;
		inline constexpr auto MAIN = types<GumboTag::GUMBO_TAG_MAIN>;
		inline constexpr auto DIV = types<GumboTag::GUMBO_TAG_DIV>;
		inline constexpr auto A = types<GumboTag::GUMBO_TAG_A>;
		inline constexpr auto EM = types<GumboTag::GUMBO_TAG_EM>;
		inline constexpr auto STRONG = types<GumboTag::GUMBO_TAG_STRONG>;
		inline constexpr auto SMALL = types<GumboTag::GUMBO_TAG_SMALL>;
		inline constexpr auto S = types<GumboTag::GUMBO_TAG_S>;
		inline constexpr auto CITE = types<GumboTag::GUMBO_TAG_CITE>;
		inline constexpr auto Q = types<GumboTag::GUMBO_TAG_Q>;
		inline constexpr auto DFN = types<GumboTag::GUMBO_TAG_DFN>;
		inline constexpr auto ABBR = types<GumboTag::GUMBO_TAG_ABBR>;
		inline constexpr auto DATA = types<GumboTag::GUMBO_TAG_DATA>;
		inline constexpr auto TIME = types<GumboTag::GUMBO_TAG_TIME>;
		inline constexpr auto CODE = types<GumboTag::GUMBO_TAG_CODE>;
		inline constexpr auto VAR = types<GumboTag::GUMBO_TAG_VAR>;
		inline constexpr auto SAMP = types<GumboTag::GUMBO_TAG_SAMP>;
		inline constexpr auto KBD = types<GumboTag::GUMBO_TAG_KBD>;
		inline constexpr auto SUB = types<GumboTag::GUMBO_TAG_SUB>;
		inline constexpr auto SUP = types<GumboTag::GUMBO_TAG_SUP>;
		inline constexpr auto I = types<GumboTag::GUMBO_TAG_I>;
		inline constexpr auto B = types<GumboTag::GUMBO_TAG_B>;
		inline constexpr auto U = types<GumboTag::GUMBO_TAG_U>;
		inline constexpr auto MARK = types<GumboTag::GUMBO_TAG_MARK>;
		inline constexpr auto RUBY = types<GumboTag::GUMBO_TAG_RUBY>;
		inline constexpr auto RT = types<GumboTag::GUMBO_TAG_RT>;
		inline constexpr auto RP = types<GumboTag::GUMBO_TAG_RP>;
		inline constexpr auto BDI = types<GumboTag::GUMBO_TAG_BDI>;
		inline constexpr auto BDO = types<GumboTag::GUMBO_TAG_BDO>;
		inline constexpr auto SPAN = types<GumboTag::GUMBO_TAG_SPAN>;
		inline constexpr auto BR = types<GumboTag::GUMBO_TAG_BR>;
		inline constexpr auto WBR = types<GumboTag::GUMBO_TAG_WBR>;
		inline constexpr auto INS = types<GumboTag::GUMBO_TAG_INS>;
		inline constexpr auto DEL = types<GumboTag::GUMBO_TAG_DEL>;
		inline constexpr auto IMAGE = types<GumboTag::GUMBO_TAG_IMAGE>;
		inline constexpr auto IMG = types<GumboTag::GUMBO_TAG_IMG>;
		inline constexpr auto IFRAME = types<GumboTag::GUMBO_TAG_IFRAME>;
		inline constexpr auto EMBED = types<GumboTag::GUMBO_TAG_EMBED>;
		inline constexpr auto OBJECT = types<GumboTag::GUMBO_TAG_OBJECT>;
		inline constexpr auto PARAM = types<GumboTag::GUMBO_TAG_PARAM>;
		inline constexpr auto VIDEO = types<GumboTag::GUMBO_TAG_VIDEO>;
		inline constexpr auto AUDIO = types<GumboTag::GUMBO_TAG_AUDIO>;
		inline constexpr auto SOURCE = types<GumboTag::GUMBO_TAG_SOURCE>;
		inline constexpr auto TRACK = types<GumboTag::GUMBO_TAG_TRACK>;
		inline constexpr auto CANVAS = types<GumboTag::GUMBO_TAG_CANVAS>;
		inline constexpr auto MAP = types<GumboTag::GUMBO_TAG_MAP>;
		inline constexpr auto AREA = types<GumboTag::GUMBO_TAG_AREA>;
		inline constexpr auto MATH = types<GumboTag::GUMBO_TAG_MATH>;
		inline constexpr auto MI = types<GumboTag::GUMBO_TAG_MI>;
		inline constexpr auto MO = types<GumboTag::GUMBO_TAG_MO>;
		inline constexpr auto MN = types<GumboTag::GUMBO_TAG_MN>;
		inline constexpr auto MS = types<GumboTag::GUMBO_TAG_MS>;
		inline constexpr auto MTEXT = types<GumboTag::GUMBO_TAG_MTEXT>;
		inline constexpr auto MGLYPH = types<GumboTag::GUMBO_TAG_MGLYPH>;
		inline constexpr auto MALIGNMARK = types<GumboTag::GUMBO_TAG_MALIGNMARK>;
		inline constexpr auto ANNOTATION_XML =
		  types<GumboTag::GUMBO_TAG_ANNOTATION_XML>;
		inline constexpr auto SVG = types<GumboTag::GUMBO_TAG_SVG>;
		inline constexpr auto FOREIGNOBJECT =
		  types<GumboTag::GUMBO_TAG_FOREIGNOBJECT>;
		inline constexpr auto DESC = types<GumboTag::GUMBO_TAG_DESC>;
		inline constexpr auto TABLE = types<GumboTag::GUMBO_TAG_TABLE>;
		inline constexpr auto CAPTION = types<GumboTag::GUMBO_TAG_CAPTION>;
		inline constexpr auto COLGROUP = types<GumboTag::GUMBO_TAG_COLGROUP>;
		inline constexpr auto COL = types<GumboTag::GUMBO_TAG_COL>;
		inline constexpr auto TBODY = types<GumboTag::GUMBO_TAG_TBODY>;
		inline constexpr auto THEAD = types<GumboTag::GUMBO_TAG_THEAD>;
		inline constexpr auto TFOOT = types<GumboTag::GUMBO_TAG_TFOOT>;
		inline constexpr auto TR = types<GumboTag::GUMBO_TAG_TR>;
		inline constexpr auto TD = types<GumboTag::GUMBO_TAG_TD>;
		inline constexpr auto TH = types<GumboTag::GUMBO_TAG_TH>;
		inline constexpr auto FORM = types<GumboTag::GUMBO_TAG_FORM>;
		inline constexpr auto FIELDSET = types<GumboTag::GUMBO_TAG_FIELDSET>;
		inline constexpr auto LEGEND = types<GumboTag::GUMBO_TAG_LEGEND>;
		inline constexpr auto LABEL = types<GumboTag::GUMBO_TAG_LABEL>;
		inline constexpr auto INPUT = types<GumboTag::GUMBO_TAG_INPUT>;
		inline constexpr auto BUTTON = types<GumboTag::GUMBO_TAG_BUTTON>;
		inline constexpr auto SELECT = types<GumboTag::GUMBO_TAG_SELECT>;
		inline constexpr auto DATALIST = types<GumboTag::GUMBO_TAG_DATALIST>;
		inline constexpr auto OPTGROUP = types<GumboTag::GUMBO_TAG_OPTGROUP>;
		inline constexpr auto OPTION = types<GumboTag::GUMBO_TAG_OPTION>;
		inline constexpr auto TEXTAREA = types<GumboTag::GUMBO_TAG_TEXTAREA>;
		inline constexpr auto KEYGEN = types<GumboTag::GUMBO_TAG_KEYGEN>;
		inline constexpr auto OUTPUT = types<GumboTag::GUMBO_TAG_OUTPUT>;
		inline constexpr auto PROGRESS = types<GumboTag::GUMBO_TAG_PROGRESS>;
		inline constexpr auto METER = types<GumboTag::GUMBO_TAG_METER>;
		inline constexpr auto DETAILS = types<GumboTag::GUMBO_TAG_DETAILS>;
		inline constexpr auto SUMMARY = types<GumboTag::GUMBO_TAG_SUMMARY>;
		inline constexpr auto MENU = types<GumboTag::GUMBO_TAG_MENU>;
		inline constexpr auto MENUITEM = types<GumboTag::GUMBO_TAG_MENUITEM>;
		inline constexpr auto APPLET = types<GumboTag::GUMBO_TAG_APPLET>;
		inline constexpr auto ACRONYM = types<GumboTag::GUMBO_TAG_ACRONYM>;
		inline constexpr auto BGSOUND = types<GumboTag::GUMBO_TAG_BGSOUND>;
		inline constexpr auto DIR = types<GumboTag::GUMBO_TAG_DIR>;
		inline constexpr auto FRAME = types<GumboTag::GUMBO_TAG_FRAME>;
		inline constexpr auto FRAMESET = types<GumboTag::GUMBO_TAG_FRAMESET>;
		inline constexpr auto NOFRAMES = types<GumboTag::GUMBO_TAG_NOFRAMES>;
		inline constexpr auto ISINDEX = types<GumboTag::GUMBO_TAG_ISINDEX>;
		inline constexpr auto LISTING = types<GumboTag::GUMBO_TAG_LISTING>;
		inline constexpr auto XMP = types<GumboTag::GUMBO_TAG_XMP>;
		inline constexpr auto NEXTID = types<GumboTag::GUMBO_TAG_NEXTID>;
		inline constexpr auto NOEMBED = types<GumboTag::GUMBO_TAG_NOEMBED>;
		inline constexpr auto PLAINTEXT = types<GumboTag::GUMBO_TAG_PLAINTEXT>;
		inline constexpr auto RB = types<GumboTag::GUMBO_TAG_RB>;
		inline constexpr auto STRIKE = types<GumboTag::GUMBO_TAG_STRIKE>;
		inline constexpr auto BASEFONT = types<GumboTag::GUMBO_TAG_BASEFONT>;
		inline constexpr auto BIG = types<GumboTag::GUMBO_TAG_BIG>;
		inline constexpr auto BLINK = types<GumboTag::GUMBO_TAG_BLINK>;
		inline constexpr auto CENTER = types<GumboTag::GUMBO_TAG_CENTER>;
		inline constexpr auto FONT = types<GumboTag::GUMBO_TAG_FONT>;
		inline constexpr auto MARQUEE = types<GumboTag::GUMBO_TAG_MARQUEE>;
		inline constexpr auto MULTICOL = types<GumboTag::GUMBO_TAG_MULTICOL>;
		inline constexpr auto NOBR = types<GumboTag::GUMBO_TAG_NOBR>;
		inline constexpr auto SPACER = types<GumboTag::GUMBO_TAG_SPACER>;
		inline constexpr auto TT = types<GumboTag::GUMBO_TAG_TT>;
		inline constexpr auto RTC = types<GumboTag::GUMBO_TAG_RTC>;
	} // namespace match_tag
} // namespace daw::gumbo::match_details

template<
  typename MatchL,
  typename MatchR,
  std::enable_if_t<
    std::conjunction_v<
      std::is_invocable_r<bool, daw::remove_cvref_t<MatchL>, GumboNode const &>,
      std::
        is_invocable_r<bool, daw::remove_cvref_t<MatchR>, GumboNode const &>>,
    std::nullptr_t> = nullptr>
constexpr auto operator||( MatchL const &lhs, MatchR const &rhs ) noexcept {
	return daw::gumbo::match_any{ lhs }.append( daw::gumbo::match_any{ rhs } );
};

template<
  typename MatchL,
  typename MatchR,
  std::enable_if_t<
    std::conjunction_v<
      std::is_invocable_r<bool, daw::remove_cvref_t<MatchL>, GumboNode const &>,
      std::
        is_invocable_r<bool, daw::remove_cvref_t<MatchR>, GumboNode const &>>,
    std::nullptr_t> = nullptr>
constexpr auto operator&&( MatchL const &lhs, MatchR const &rhs ) noexcept {
	return daw::gumbo::match_all{ lhs }.append( daw::gumbo::match_all{ rhs } );
};

template<
  typename MatchL,
  typename MatchR,
  std::enable_if_t<
    std::conjunction_v<
      std::is_invocable_r<bool, daw::remove_cvref_t<MatchL>, GumboNode const &>,
      std::
        is_invocable_r<bool, daw::remove_cvref_t<MatchR>, GumboNode const &>>,
    std::nullptr_t> = nullptr>
constexpr auto operator^( MatchL const &lhs, MatchR const &rhs ) noexcept {
	return daw::gumbo::match_one{ lhs }.append( daw::gumbo::match_one{ rhs } );
};

template<typename Matcher,
         std::enable_if_t<std::is_invocable_r_v<bool,
                                                daw::remove_cvref_t<Matcher>,
                                                GumboNode const &>,
                          std::nullptr_t> = nullptr>
constexpr auto operator!( Matcher &&matcher ) noexcept {
	return daw::gumbo::match_not{ DAW_FWD2( Matcher, matcher ) };
};

namespace daw::gumbo::match {
	namespace attribute {
		using namespace match_details::match_attribute;
	}

	namespace class_type {
		using namespace match_details::match_class;
	}

	namespace id {
		using namespace match_details::match_id;
	}

	namespace inner_text {
		using namespace match_details::match_inner_text;
	}

	namespace outer_text {
		using namespace match_details::match_outer_text;
	}

	namespace content_text {
		using namespace match_details::match_content_text;
	}

	namespace tag {
		using namespace match_details::match_tag;
	}
} // namespace daw::gumbo::match
