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
	template<typename... Matchers>
	struct match_all {
		daw::tuple2<Matchers...> m_matchers;

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
	};
	template<typename... Matchers>
	match_all( Matchers... ) -> match_all<Matchers...>;

	template<typename... Matchers>
	class match_any {
		daw::tuple2<Matchers...> m_matchers;

	public:
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
	};
	template<typename... Matchers>
	match_any( Matchers... ) -> match_any<Matchers...>;

	template<typename... Matchers>
	class match_one {
		daw::tuple2<Matchers...> m_matchers;

	public:
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
			return where( [=]( daw::string_view text ) noexcept {
				auto first = std::begin( c );
				auto last = std::end( c );
				return std::find_if( first, last, [&]( daw::string_view cur_text ) {
					       return cur_text == text;
				       } ) != last;
			} );
		}

		template<typename... StringView>
		static constexpr auto contains( daw::string_view search_text,
		                                StringView &&...search_texts ) noexcept {
			return where( [=]( daw::string_view text ) noexcept -> bool {
				return ( text.find( search_text ) != daw::string_view::npos ) or
				       ( ( text.find( search_texts ) != daw::string_view::npos ) or
				         ... );
			} );
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

	template<typename Result,
	         typename MatchL,
	         typename MatchR,
	         std::size_t... LIdx,
	         std::size_t... RIdx>
	constexpr Result opCombine( MatchL const &lhs,
	                            MatchR const &rhs,
	                            std::index_sequence<LIdx...>,
	                            std::index_sequence<RIdx...> ) {
		return Result{ daw::get<LIdx>( lhs.m_matchers )...,
		               daw::get<RIdx>( rhs.m_matchers )... };
	}

	template<typename... MatchL, typename... MatchR>
	constexpr match_any<MatchL..., MatchR...>
	operator||( match_any<MatchL...> const &lhs,
	            match_any<MatchR...> const &rhs ) {
		// This will keep the stack depths smaller
		return opCombine<match_any<MatchL..., MatchR...>>(
		  lhs,
		  rhs,
		  std::make_index_sequence<sizeof...( MatchL )>{ },
		  std::make_index_sequence<sizeof...( MatchR )>{ } );
	}

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
	constexpr auto operator||( MatchL &&lhs, MatchR &&rhs ) noexcept {
		return match_any{ DAW_FWD2( MatchL, lhs ), DAW_FWD2( MatchR, rhs ) };
	};

	template<typename... MatchL, typename... MatchR>
	constexpr match_all<MatchL..., MatchR...>
	operator&&( match_all<MatchL...> const &lhs,
	            match_all<MatchR...> const &rhs ) {
		// This will keep the stack depths smaller
		return opCombine<match_all<MatchL..., MatchR...>>(
		  lhs,
		  rhs,
		  std::make_index_sequence<sizeof...( MatchL )>{ },
		  std::make_index_sequence<sizeof...( MatchR )>{ } );
	}

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
	constexpr auto operator&&( MatchL &&lhs, MatchR &&rhs ) noexcept {
		return match_all{ DAW_FWD2( MatchL, lhs ), DAW_FWD2( MatchR, rhs ) };
	};

	template<typename... MatchL, typename... MatchR>
	constexpr match_one<MatchL..., MatchR...>
	operator^( match_one<MatchL...> const &lhs,
	           match_one<MatchR...> const &rhs ) {
		// This will keep the stack depths smaller
		return opCombine<match_one<MatchL..., MatchR...>>(
		  lhs,
		  rhs,
		  std::make_index_sequence<sizeof...( MatchL )>{ },
		  std::make_index_sequence<sizeof...( MatchR )>{ } );
	}

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
	constexpr auto operator^( MatchL &&lhs, MatchR &&rhs ) noexcept {
		return match_one{ DAW_FWD2( MatchL, lhs ), DAW_FWD2( MatchR, rhs ) };
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
	struct match {
		using attribute = match_details::match_attribute;
		using class_type = match_details::match_class;
		using id = match_details::match_id;
		using inner_text = match_details::match_inner_text;
		using outer_text = match_details::match_outer_text;
		using content_text = match_details::match_content_text;
		using tag = match_details::match_tag;
	};
} // namespace daw::gumbo
