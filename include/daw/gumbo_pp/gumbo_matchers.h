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
#include <daw/daw_string_view.h>
#include <daw/daw_tuple2.h>

#include <cstddef>
#include <functional>
#include <type_traits>

namespace daw::gumbo {
	// A combined predicate that returns true when all of it's predicates return
	// true
	template<typename... Matchers>
	struct match_all {
		daw::tuple2<Matchers...> m_matchers;

		constexpr match_all( daw::tuple2<Matchers...> &&m )
		  : m_matchers( DAW_MOVE( m ) ) {}

		explicit constexpr match_all( Matchers &&...matchers )
		  : m_matchers{ DAW_MOVE( matchers )... } {}

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
		  : m_matchers( DAW_MOVE( m ) ) {}

		explicit constexpr match_any( Matchers &&...matchers )
		  : m_matchers{ DAW_MOVE( matchers )... } {}

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
		  : m_matchers( DAW_MOVE( m ) ) {}

		explicit constexpr match_one( Matchers &&...matchers )
		  : m_matchers{ DAW_MOVE( matchers )... } {}

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
		  : Matcher{ DAW_MOVE( matcher ) } {}

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
	struct match_attribute {
		/// Match any node that has an attribute where all the predicates returns
		/// true
		template<typename Predicate, typename... Predicates>
		static constexpr auto where( Predicate &&pred,
		                             Predicates &&...preds ) noexcept {
			return [=]( auto const &node ) {
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
		static constexpr auto has_none = []( auto const &node ) {
			return get_attribute_count( node ) == 0;
		};

		/// Match any node that has attributes
		static constexpr auto has = []( auto const &node ) {
			return get_attribute_count( node ) != 0;
		};

		/// Match any node that has any of these attributes
		template<typename Container,
		         std::enable_if_t<
		           daw::traits::is_container_like_v<daw::remove_cvref_t<Container>>,
		           std::nullptr_t> = nullptr>
		static constexpr auto exists( Container &&c ) noexcept {
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
		static constexpr auto exists( daw::string_view name,
		                              StringView &&...names ) noexcept {
			return [=]( auto const &node ) {
				return attribute_exists( node, name ) or
				       ( attribute_exists( node, names ) or ... );
			};
		}

		struct name {
			// Match any node that has a matching attribute name
			template<typename Container,
			         std::enable_if_t<daw::traits::is_container_like_v<
			                            daw::remove_cvref_t<Container>>,
			                          std::nullptr_t> = nullptr>
			static constexpr auto is( Container &&c ) noexcept {
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
			static constexpr auto is( daw::string_view attribute_name,
			                          StringView &&...attribute_names ) noexcept {
				return where( [=]( daw::string_view name, daw::string_view ) noexcept {
					return ( name == attribute_name ) or
					       ( ( name == attribute_names ) or ... );
				} );
			}
		};

		struct value {
			/// Match any node with named attribute who's value is either value or
			/// prefixed by value and a hyphen `-`
			static constexpr auto
			contains_prefix( daw::string_view attribute_name,
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
			static constexpr auto contains( daw::string_view attribute_name,
			                                Container &&value_substrs ) noexcept {
				return where(
				  [=]( daw::string_view name, daw::string_view value ) noexcept {
					  auto first = std::begin( value_substrs );
					  auto last = std::end( value_substrs );
					  if( first == last ) {
						  return false;
					  }
					  return name == attribute_name and
					         std::find_if( first,
					                       last,
					                       [&]( daw::string_view value_substr ) {
						                       return value.find( value_substr ) !=
						                              daw::string_view::npos;
					                       } ) != last;
				  } );
			}

			/// Match any node with named attribute who's value contains the
			/// specified value
			template<typename... StringView>
			static constexpr auto contains( daw::string_view attribute_name,
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
			static constexpr auto starts_with( daw::string_view attribute_name,
			                                   Container &&value_prefixes ) noexcept {
				return where(
				  [=]( daw::string_view name, daw::string_view value ) noexcept {
					  auto first = std::begin( value_prefixes );
					  auto last = std::end( value_prefixes );
					  if( first == last ) {
						  return false;
					  }
					  return name == attribute_name and
					         std::find_if( first,
					                       last,
					                       [&]( daw::string_view value_prefix ) {
						                       return value.starts_with( value_prefix );
					                       } ) != last;
				  } );
			}

			/// Match any node with named attribute who's value starts with one of the
			/// specified values
			template<typename... StringView>
			static constexpr auto
			starts_with( daw::string_view attribute_name,
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
			static constexpr auto ends_with( daw::string_view attribute_name,
			                                 Container &&value_prefixes ) noexcept {
				return where(
				  [=]( daw::string_view name, daw::string_view value ) noexcept {
					  auto first = std::begin( value_prefixes );
					  auto last = std::end( value_prefixes );
					  if( first == last ) {
						  return false;
					  }
					  return name == attribute_name and
					         std::find_if( first,
					                       last,
					                       [&]( daw::string_view value_prefix ) {
						                       return value.ends_with( value_prefix );
					                       } ) != last;
				  } );
			}

			/// Match any node with named attribute who's value end with the
			/// specified value
			template<typename... StringView>
			static constexpr auto
			ends_with( daw::string_view attribute_name,
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
			static constexpr auto is( daw::string_view attribute_name,
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
			static constexpr auto is( daw::string_view attribute_name,
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
			static constexpr auto
			is_empty( daw::string_view attribute_name ) noexcept {
				return where( [attribute_name]( daw::string_view name,
				                                daw::string_view value ) noexcept {
					return name == attribute_name and value.empty( ) and value.data( );
				} );
			}

			/// Match any node with named attribute who's value is null
			static constexpr auto
			is_null( daw::string_view attribute_name ) noexcept {
				return where( [attribute_name]( daw::string_view name,
				                                daw::string_view value ) noexcept {
					return name == attribute_name and not value.data( );
				} );
			}

			/// Match any node with named attribute who's value is not empty
			static constexpr auto
			has_value( daw::string_view attribute_name ) noexcept {
				return where( [attribute_name]( daw::string_view name,
				                                daw::string_view value ) noexcept {
					return name == attribute_name and not value.empty( );
				} );
			}
		};
	};

	struct match_class {
		/// Match any node with a class that returns true for all the predicates
		template<typename Predicate, typename... Predicates>
		static constexpr auto where( Predicate &&pred,
		                             Predicates &&...preds ) noexcept {
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
		static constexpr auto is( daw::string_view attribute_name,
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
		static constexpr auto is( daw::string_view class_name,
		                          StringView &&...class_names ) noexcept {
			return match_attribute::where(
			  [=]( daw::string_view attribute_name,
			       daw::string_view attribute_value ) noexcept -> bool {
				  return attribute_name == "class" and
				         ( attribute_value == class_name or
				           ( ( attribute_value == class_names ) or ... ) );
			  } );
		}
	};

	struct match_id {
		/// Match any node with a id that returns true for the predicate
		template<typename Predicate, typename... Predicates>
		static constexpr auto where( Predicate &&pred,
		                             Predicates &&...preds ) noexcept {
			return match_attribute::where(
			  [=]( daw::string_view attribute_name,
			       daw::string_view attribute_value ) noexcept -> bool {
				  return attribute_name == "id" and pred( attribute_value ) and
				         ( preds( attribute_value ) and ... );
			  } );
		}

		/// Match any node with a id that is any of the following names
		template<typename... StringView>
		static constexpr auto is( daw::string_view id_name,
		                          StringView &&...id_names ) noexcept {
			return match_attribute::where(
			  [=]( daw::string_view attribute_name,
			       daw::string_view attribute_value ) noexcept -> bool {
				  return attribute_name == "id" and
				         ( attribute_value == id_name or
				           ( ( attribute_value == id_names ) or ... ) );
			  } );
		}
	};

	// For matching the content of tags like A
	struct match_content_text {
		/// Match any node with a id that returns true for the predicates
		template<typename Predicate, typename... Predicates>
		static constexpr auto where( Predicate &&pred,
		                             Predicates &&...preds ) noexcept {
			return [=]( auto const &node ) -> bool {
				auto txt = node_content_text( node );
				daw::string_view sv = txt;
				return pred( sv ) and ( preds( sv ) and ... );
			};
		}

		template<typename Map, typename Predicate>
		static constexpr auto map( Map &&map, Predicate &&pred ) noexcept {
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
		static constexpr auto contains( Container &&c ) noexcept {
			return [=]( auto const &node ) noexcept {
				auto text = node_content_text( node );
				if( text.empty( ) ) {
					return false;
				}
				auto first = std::begin( c );
				auto last = std::end( c );
				auto const fpos =
				  std::find_if( first, last, [&]( daw::string_view cur_text ) {
					  return text.find( cur_text ) != std::string::npos;
				  } );
				return fpos != last;
			};
		}

		template<typename... StringView>
		static constexpr auto contains( daw::string_view search_text,
		                                StringView &&...search_texts ) noexcept {
			return [=]( auto const &node ) noexcept -> bool {
				auto text = node_content_text( node );
				return ( text.find( search_text ) != std::string::npos ) or
				       ( ( text.find( search_texts ) != std::string::npos ) or ... );
			};
		}

		static constexpr auto is_empty = []( auto const &node ) noexcept -> bool {
			return node_content_text( node ).empty( );
		};

		/// Match any node with outer text who's value starts with and of the
		/// specified values
		template<typename Container,
		         std::enable_if_t<
		           daw::traits::is_container_like_v<daw::remove_cvref_t<Container>>,
		           std::nullptr_t> = nullptr>
		static constexpr auto starts_with( Container &&c ) noexcept {
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
		static constexpr auto starts_with( daw::string_view prefix_text,
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
		static constexpr auto ends_with( Container &&c ) noexcept {
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
		static constexpr auto ends_with( daw::string_view prefix_text,
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
		static constexpr auto is( Container &&c ) noexcept {
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
		static constexpr auto is( daw::string_view match_text,
		                          StringView &&...match_texts ) noexcept {
			return where( [=]( daw::string_view text ) noexcept -> bool {
				return ( text == match_text ) or ( ( text == match_texts ) or ... );
			} );
		}
	};

	struct match_inner_text {
		/// Match any node with inner_text that returns true for the predicate
		template<typename Predicate, typename... Predicates>
		static constexpr auto where( daw::string_view html_doc,
		                             Predicate &&pred,
		                             Predicates &&...preds ) noexcept {
			return [=]( auto const &node ) -> bool {
				daw::string_view text = node_inner_text( node, html_doc );
				return pred( text ) and ( preds( text ) and ... );
			};
		}

		/// Match any node with inner_text that is empty
		static constexpr auto is_empty( daw::string_view html_doc ) noexcept {
			return where( html_doc, []( daw::string_view text ) noexcept -> bool {
				return text.empty( );
			} );
		}

		/// Match any node with inner_text that contains search_text
		template<typename Container,
		         std::enable_if_t<
		           daw::traits::is_container_like_v<daw::remove_cvref_t<Container>>,
		           std::nullptr_t> = nullptr>
		static constexpr auto contains( daw::string_view html_doc,
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
		static constexpr auto contains( daw::string_view html_doc,
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
		static constexpr auto starts_with( daw::string_view html_doc,
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
		static constexpr auto starts_with( daw::string_view html_doc,
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
		static constexpr auto ends_with( daw::string_view html_doc,
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
		static constexpr auto ends_with( daw::string_view html_doc,
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
		static constexpr auto is( daw::string_view html_doc,
		                          Container &&c ) noexcept {
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
		static constexpr auto is( daw::string_view html_doc,
		                          daw::string_view suffix_text,
		                          StringView &&...suffix_texts ) noexcept {
			return where( html_doc, [=]( daw::string_view text ) noexcept {
				return text == suffix_text or ( ( text == suffix_texts ) or ... );
			} );
		}
	};

	struct match_outer_text {
		/// Match any node with outer_text that returns true for the predicates
		template<typename Predicate, typename... Predicates>
		static constexpr auto where( daw::string_view html_doc,
		                             Predicate &&pred,
		                             Predicates &&...preds ) noexcept {
			return [=]( auto const &node ) -> bool {
				auto text = node_outer_text( node, html_doc );
				return pred( text ) and ( preds( text ) and ... );
			};
		}

		/// Match any node with outer_text that is empty
		static constexpr auto is_empty( daw::string_view html_doc ) noexcept {
			return where( html_doc, []( daw::string_view text ) noexcept -> bool {
				return text.empty( );
			} );
		}

		/// Match any node with outer_text that contains match_text
		template<typename Container,
		         std::enable_if_t<
		           daw::traits::is_container_like_v<daw::remove_cvref_t<Container>>,
		           std::nullptr_t> = nullptr>
		static constexpr auto contains( Container &&c ) noexcept {
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
		static constexpr auto contains( daw::string_view html_doc,
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
		static constexpr auto starts_with( Container &&c ) noexcept {
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
		static constexpr auto starts_with( daw::string_view html_doc,
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
		static constexpr auto ends_with( Container &&c ) noexcept {
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
		static constexpr auto ends_with( daw::string_view html_doc,
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
		static constexpr auto is( Container &&c ) noexcept {
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
		static constexpr auto is( daw::string_view html_doc,
		                          daw::string_view match_text,
		                          StringView &&...match_texts ) noexcept {
			return where( html_doc, [=]( daw::string_view text ) noexcept {
				return text == match_text or ( ( text == match_texts ) or ... );
			} );
		}
	};

	struct match_tag {
		/// Match any node with where the tag type where all the Predicates return
		/// true
		template<typename Predicate, typename... Predicates>
		static constexpr auto where( Predicate &&pred,
		                             Predicates &&...preds ) noexcept {
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
		static constexpr auto types = []( auto const &node ) {
			if( node.type != GUMBO_NODE_ELEMENT ) {
				return false;
			}
			auto tag_value = node.v.element.tag;
			return ( ( tag_value == tags ) | ... );
		};
	};

	template<
	  typename MatchL,
	  typename MatchR,
	  std::enable_if_t<
	    std::conjunction_v<
	      std::
	        is_invocable_r<bool, daw::remove_cvref_t<MatchL>, GumboNode const &>,
	      std::
	        is_invocable_r<bool, daw::remove_cvref_t<MatchR>, GumboNode const &>>,
	    std::nullptr_t> = nullptr>
	constexpr auto operator||( MatchL const &lhs, MatchR const &rhs ) noexcept {
		return match_any{ lhs }.append( match_any{ rhs } );
	};

	template<
	  typename MatchL,
	  typename MatchR,
	  std::enable_if_t<
	    std::conjunction_v<
	      std::
	        is_invocable_r<bool, daw::remove_cvref_t<MatchL>, GumboNode const &>,
	      std::
	        is_invocable_r<bool, daw::remove_cvref_t<MatchR>, GumboNode const &>>,
	    std::nullptr_t> = nullptr>
	constexpr auto operator&&( MatchL const &lhs, MatchR const &rhs ) noexcept {
		return match_all{ lhs }.append( match_all{ rhs } );
	};

	template<
	  typename MatchL,
	  typename MatchR,
	  std::enable_if_t<
	    std::conjunction_v<
	      std::
	        is_invocable_r<bool, daw::remove_cvref_t<MatchL>, GumboNode const &>,
	      std::
	        is_invocable_r<bool, daw::remove_cvref_t<MatchR>, GumboNode const &>>,
	    std::nullptr_t> = nullptr>
	constexpr auto operator^( MatchL const &lhs, MatchR const &rhs ) noexcept {
		return match_one{ lhs }.append( match_one{ rhs } );
	};

	template<typename Matcher,
	         std::enable_if_t<std::is_invocable_r_v<bool,
	                                                daw::remove_cvref_t<Matcher>,
	                                                GumboNode const &>,
	                          std::nullptr_t> = nullptr>
	constexpr auto operator!( Matcher &&matcher ) noexcept {
		return match_not{ DAW_FWD2( Matcher, matcher ) };
	};
} // namespace daw::gumbo::match_details

namespace daw::gumbo {
	namespace match {
		using attribute = match_details::match_attribute;
		using class_type = match_details::match_class;
		using id = match_details::match_id;
		using inner_text = match_details::match_inner_text;
		using outer_text = match_details::match_outer_text;
		using content_text = match_details::match_content_text;
		using tag = match_details::match_tag;

		namespace tags {
			inline constexpr auto HTML =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_HTML>;
			inline constexpr auto HEAD =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_HEAD>;
			inline constexpr auto TITLE =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_TITLE>;
			inline constexpr auto BASE =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_BASE>;
			inline constexpr auto LINK =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_LINK>;
			inline constexpr auto META =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_META>;
			inline constexpr auto STYLE =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_STYLE>;
			inline constexpr auto SCRIPT =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_SCRIPT>;
			inline constexpr auto NOSCRIPT =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_NOSCRIPT>;
			inline constexpr auto TEMPLATE =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_TEMPLATE>;
			inline constexpr auto BODY =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_BODY>;
			inline constexpr auto ARTICLE =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_ARTICLE>;
			inline constexpr auto SECTION =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_SECTION>;
			inline constexpr auto NAV =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_NAV>;
			inline constexpr auto ASIDE =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_ASIDE>;
			inline constexpr auto H1 =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_H1>;
			inline constexpr auto H2 =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_H2>;
			inline constexpr auto H3 =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_H3>;
			inline constexpr auto H4 =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_H4>;
			inline constexpr auto H5 =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_H5>;
			inline constexpr auto H6 =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_H6>;
			inline constexpr auto HGROUP =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_HGROUP>;
			inline constexpr auto HEADER =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_HEADER>;
			inline constexpr auto FOOTER =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_FOOTER>;
			inline constexpr auto ADDRESS =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_ADDRESS>;
			inline constexpr auto P =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_P>;
			inline constexpr auto HR =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_HR>;
			inline constexpr auto PRE =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_PRE>;
			inline constexpr auto BLOCKQUOTE =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_BLOCKQUOTE>;
			inline constexpr auto OL =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_OL>;
			inline constexpr auto UL =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_UL>;
			inline constexpr auto LI =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_LI>;
			inline constexpr auto DL =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_DL>;
			inline constexpr auto DT =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_DT>;
			inline constexpr auto DD =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_DD>;
			inline constexpr auto FIGURE =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_FIGURE>;
			inline constexpr auto FIGCAPTION =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_FIGCAPTION>;
			inline constexpr auto MAIN =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_MAIN>;
			inline constexpr auto DIV =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_DIV>;
			inline constexpr auto A =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_A>;
			inline constexpr auto EM =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_EM>;
			inline constexpr auto STRONG =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_STRONG>;
			inline constexpr auto SMALL =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_SMALL>;
			inline constexpr auto S =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_S>;
			inline constexpr auto CITE =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_CITE>;
			inline constexpr auto Q =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_Q>;
			inline constexpr auto DFN =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_DFN>;
			inline constexpr auto ABBR =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_ABBR>;
			inline constexpr auto DATA =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_DATA>;
			inline constexpr auto TIME =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_TIME>;
			inline constexpr auto CODE =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_CODE>;
			inline constexpr auto VAR =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_VAR>;
			inline constexpr auto SAMP =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_SAMP>;
			inline constexpr auto KBD =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_KBD>;
			inline constexpr auto SUB =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_SUB>;
			inline constexpr auto SUP =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_SUP>;
			inline constexpr auto I =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_I>;
			inline constexpr auto B =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_B>;
			inline constexpr auto U =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_U>;
			inline constexpr auto MARK =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_MARK>;
			inline constexpr auto RUBY =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_RUBY>;
			inline constexpr auto RT =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_RT>;
			inline constexpr auto RP =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_RP>;
			inline constexpr auto BDI =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_BDI>;
			inline constexpr auto BDO =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_BDO>;
			inline constexpr auto SPAN =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_SPAN>;
			inline constexpr auto BR =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_BR>;
			inline constexpr auto WBR =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_WBR>;
			inline constexpr auto INS =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_INS>;
			inline constexpr auto DEL =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_DEL>;
			inline constexpr auto IMAGE =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_IMAGE>;
			inline constexpr auto IMG =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_IMG>;
			inline constexpr auto IFRAME =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_IFRAME>;
			inline constexpr auto EMBED =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_EMBED>;
			inline constexpr auto OBJECT =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_OBJECT>;
			inline constexpr auto PARAM =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_PARAM>;
			inline constexpr auto VIDEO =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_VIDEO>;
			inline constexpr auto AUDIO =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_AUDIO>;
			inline constexpr auto SOURCE =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_SOURCE>;
			inline constexpr auto TRACK =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_TRACK>;
			inline constexpr auto CANVAS =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_CANVAS>;
			inline constexpr auto MAP =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_MAP>;
			inline constexpr auto AREA =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_AREA>;
			inline constexpr auto MATH =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_MATH>;
			inline constexpr auto MI =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_MI>;
			inline constexpr auto MO =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_MO>;
			inline constexpr auto MN =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_MN>;
			inline constexpr auto MS =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_MS>;
			inline constexpr auto MTEXT =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_MTEXT>;
			inline constexpr auto MGLYPH =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_MGLYPH>;
			inline constexpr auto MALIGNMARK =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_MALIGNMARK>;
			inline constexpr auto ANNOTATION_XML =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_ANNOTATION_XML>;
			inline constexpr auto SVG =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_SVG>;
			inline constexpr auto FOREIGNOBJECT =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_FOREIGNOBJECT>;
			inline constexpr auto DESC =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_DESC>;
			inline constexpr auto TABLE =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_TABLE>;
			inline constexpr auto CAPTION =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_CAPTION>;
			inline constexpr auto COLGROUP =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_COLGROUP>;
			inline constexpr auto COL =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_COL>;
			inline constexpr auto TBODY =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_TBODY>;
			inline constexpr auto THEAD =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_THEAD>;
			inline constexpr auto TFOOT =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_TFOOT>;
			inline constexpr auto TR =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_TR>;
			inline constexpr auto TD =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_TD>;
			inline constexpr auto TH =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_TH>;
			inline constexpr auto FORM =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_FORM>;
			inline constexpr auto FIELDSET =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_FIELDSET>;
			inline constexpr auto LEGEND =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_LEGEND>;
			inline constexpr auto LABEL =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_LABEL>;
			inline constexpr auto INPUT =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_INPUT>;
			inline constexpr auto BUTTON =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_BUTTON>;
			inline constexpr auto SELECT =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_SELECT>;
			inline constexpr auto DATALIST =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_DATALIST>;
			inline constexpr auto OPTGROUP =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_OPTGROUP>;
			inline constexpr auto OPTION =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_OPTION>;
			inline constexpr auto TEXTAREA =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_TEXTAREA>;
			inline constexpr auto KEYGEN =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_KEYGEN>;
			inline constexpr auto OUTPUT =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_OUTPUT>;
			inline constexpr auto PROGRESS =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_PROGRESS>;
			inline constexpr auto METER =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_METER>;
			inline constexpr auto DETAILS =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_DETAILS>;
			inline constexpr auto SUMMARY =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_SUMMARY>;
			inline constexpr auto MENU =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_MENU>;
			inline constexpr auto MENUITEM =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_MENUITEM>;
			inline constexpr auto APPLET =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_APPLET>;
			inline constexpr auto ACRONYM =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_ACRONYM>;
			inline constexpr auto BGSOUND =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_BGSOUND>;
			inline constexpr auto DIR =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_DIR>;
			inline constexpr auto FRAME =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_FRAME>;
			inline constexpr auto FRAMESET =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_FRAMESET>;
			inline constexpr auto NOFRAMES =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_NOFRAMES>;
			inline constexpr auto ISINDEX =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_ISINDEX>;
			inline constexpr auto LISTING =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_LISTING>;
			inline constexpr auto XMP =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_XMP>;
			inline constexpr auto NEXTID =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_NEXTID>;
			inline constexpr auto NOEMBED =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_NOEMBED>;
			inline constexpr auto PLAINTEXT =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_PLAINTEXT>;
			inline constexpr auto RB =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_RB>;
			inline constexpr auto STRIKE =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_STRIKE>;
			inline constexpr auto BASEFONT =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_BASEFONT>;
			inline constexpr auto BIG =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_BIG>;
			inline constexpr auto BLINK =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_BLINK>;
			inline constexpr auto CENTER =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_CENTER>;
			inline constexpr auto FONT =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_FONT>;
			inline constexpr auto MARQUEE =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_MARQUEE>;
			inline constexpr auto MULTICOL =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_MULTICOL>;
			inline constexpr auto NOBR =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_NOBR>;
			inline constexpr auto SPACER =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_SPACER>;
			inline constexpr auto TT =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_TT>;
			inline constexpr auto RTC =
			  daw::gumbo::match::tag::types<GumboTag::GUMBO_TAG_RTC>;
		} // namespace tags
	}   // namespace match
} // namespace daw::gumbo
