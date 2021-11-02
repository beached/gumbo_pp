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
#include "gumbo_algorithms.h"
#include "gumbo_node_iterator.h"

#include <daw/daw_logic.h>
#include <daw/daw_string_view.h>

#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>

namespace daw::gumbo {
	template<typename... Matchers>
	class match_all {
		std::tuple<Matchers...> m_matchers;

	public:
		constexpr match_all( Matchers &&...matchers )
		  : m_matchers{ DAW_MOVE( matchers )... } {}

		constexpr match_all( Matchers const &...matchers )
		  : m_matchers{ matchers... } {}

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const {
			return std::apply(
			  [&]( auto &&...matchers ) -> bool {
				  return ( DAW_FWD( matchers )( node ) and ... );
			  },
			  m_matchers );
		}
	};
	template<typename... Matchers>
	match_all( Matchers... ) -> match_all<Matchers...>;

	template<typename... Matchers>
	class match_any {
		std::tuple<Matchers...> m_matchers;

	public:
		constexpr match_any( Matchers &&...matchers )
		  : m_matchers{ DAW_MOVE( matchers )... } {}

		constexpr match_any( Matchers const &...matchers )
		  : m_matchers{ matchers... } {}

		template<typename Node>
		constexpr bool operator( )( Node const &node ) const {
			return std::apply(
			  [&]( auto &&...matchers ) -> bool {
				  return ( DAW_FWD( matchers )( node ) or ... );
			  },
			  m_matchers );
		}
	};
	template<typename... Matchers>
	match_any( Matchers... ) -> match_any<Matchers...>;
} // namespace daw::gumbo
namespace daw::gumbo::match_details {
	struct match_attribute {
		/// Match any node that has an attribute where all the predicates returns
		/// true
		template<typename... Predicates>
		static constexpr auto where( Predicates &&...preds ) noexcept {
			return [=]( auto const &node ) {
				return details::find_attribute_if_impl(
				         gumbo_node_iterator_t( &node ),
				         [&]( GumboAttribute const &attr ) -> bool {
					         return ( preds( daw::string_view( attr.name ),
					                         daw::string_view( attr.value ) ) and
					                  ... );
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

			// Match any node that has a matching attribute name
			template<typename Container,
			         std::enable_if_t<daw::traits::is_container_like_v<
			                            daw::remove_cvref_t<Container>>,
			                          std::nullptr_t> = nullptr>
			static constexpr auto is_not( Container &&c ) noexcept {
				return match_all{
				  where( [=]( daw::string_view name, daw::string_view ) noexcept {
					  auto first = std::begin( c );
					  auto last = std::end( c );
					  return std::all_of( first, last, [&]( daw::string_view n ) {
						         return n == name;
					         } ) != last;
				  } ),
				  has_none };
			}

			// Match any node that does not have a matching attribute name
			template<typename... StringView>
			static constexpr auto is_not( daw::string_view attribute_name,
			                              StringView &&...attribute_names ) noexcept {
				return match_all{
				  where( [=]( daw::string_view name, daw::string_view ) noexcept {
					  return ( name == attribute_name ) and
					         ( ( name != attribute_names ) and ... );
				  } ),
				  has_none };
			}
		};

		struct value {
			/// Match any node with named attribute who's value is either value or
			/// prefixed by value and a hyphen `-`
			static constexpr auto
			contains_prefix( daw::string_view attribute_name,
			                 daw::string_view value_prefix ) noexcept {
				return where(
				  [attribute_name, value_prefix]( daw::string_view name,
				                                  daw::string_view value ) noexcept {
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
			static constexpr auto contains( daw::string_view attribute_name,
			                                daw::string_view value_substr ) noexcept {
				return where(
				  [attribute_name, value_substr]( daw::string_view name,
				                                  daw::string_view value ) noexcept {
					  return name == attribute_name and
					         value.find( value_substr ) != daw::string_view::npos;
				  } );
			}

			/// Match any node with named attribute who's value starts with the
			/// specified value
			static constexpr auto
			starts_with( daw::string_view attribute_name,
			             daw::string_view value_prefix ) noexcept {
				return where(
				  [attribute_name, value_prefix]( daw::string_view name,
				                                  daw::string_view value ) noexcept {
					  return name == attribute_name and value.starts_with( value_prefix );
				  } );
			}

			/// Match any node with named attribute who's value end with the
			/// specified value
			static constexpr auto
			ends_with( daw::string_view attribute_name,
			           daw::string_view value_prefix ) noexcept {
				return where(
				  [attribute_name, value_prefix]( daw::string_view name,
				                                  daw::string_view value ) noexcept {
					  return name == attribute_name and value.ends_with( value_prefix );
				  } );
			}

			/// Match any node with named attribute who's value equals with the
			/// specified value
			static constexpr auto is( daw::string_view attribute_name,
			                          daw::string_view value_prefix ) noexcept {
				return where(
				  [attribute_name, value_prefix]( daw::string_view name,
				                                  daw::string_view value ) noexcept {
					  return name == attribute_name and value == value_prefix;
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
		template<typename... Predicates>
		static constexpr auto where( Predicates &&...preds ) noexcept {
			return match_attribute::where(
			  [=]( daw::string_view attribute_name,
			       daw::string_view attribute_value ) noexcept -> bool {
				  return attribute_name == "class" and
				         ( preds( attribute_value ) and ... );
			  } );
		}

		/// Match any node with a class named that is one of the following
		template<typename... StringView>
		static constexpr auto is( StringView &&...class_names ) noexcept {
			return match_attribute::where(
			  [=]( daw::string_view attribute_name,
			       daw::string_view attribute_value ) noexcept -> bool {
				  return attribute_name == "class" and
				         ( ( attribute_value == class_names ) or ... );
			  } );
		}

		/// Match any node with a class named that is not any of the following
		template<typename... StringView>
		static constexpr auto is_not( StringView &&...class_names ) noexcept {
			return match_all(
			  match_attribute::where(
			    [=]( daw::string_view attribute_name,
			         daw::string_view attribute_value ) noexcept -> bool {
				    return attribute_name == "class" and
				           ( ( attribute_value != class_names ) and ... );
			    } ),
			  match_attribute::has_none );
		}
	};

	struct match_id {
		/// Match any node with a id that returns true for the predicate
		template<typename... Predicates>
		static constexpr auto where( Predicates &&...preds ) noexcept {
			return match_attribute::where(
			  [=]( daw::string_view attribute_name,
			       daw::string_view attribute_value ) noexcept -> bool {
				  return attribute_name == "id" and
				         ( preds( attribute_value ) and ... );
			  } );
		}

		/// Match any node with a id that is any of the following names
		template<typename... StringView>
		static constexpr auto is( StringView &&...id_names ) noexcept {
			return match_attribute::where(
			  [=]( daw::string_view attribute_name,
			       daw::string_view attribute_value ) noexcept -> bool {
				  return attribute_name == "id" and
				         ( ( attribute_value == id_names ) or ... );
			  } );
		}

		/// Match any node with a id that is not any of the following names
		template<typename... StringView>
		static constexpr auto is_not( StringView &&...id_names ) noexcept {
			return match_all{
			  match_attribute::where(
			    [=]( daw::string_view attribute_name,
			         daw::string_view attribute_value ) noexcept -> bool {
				    return attribute_name == "id" and
				           ( ( attribute_value != id_names ) and ... );
			    } ),
			  match_attribute::has_none };
		}
	};

	// For matching the content of tags like A
	struct match_content_text {
		/// Match any node with a id that returns true for the predicates
		template<typename... Predicates>
		static constexpr auto where( Predicates &&...preds ) noexcept {
			return [=]( auto const &node ) -> bool {
				return ( preds( node_content_text( node ) ) and ... );
			};
		}

		template<typename Map, typename Predicate>
		static constexpr auto map( Map &&map, Predicate &&pred ) noexcept {
			return [=]( auto const &node ) -> bool {
				return pred( map( node_content_text( node ) ) );
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

		static constexpr auto is_not_empty =
		  []( auto const &node ) noexcept -> bool {
			return not node_content_text( node ).empty( );
		};

		/// Match any node with outer text who's value starts with and of the
		/// specified value
		template<typename... StringView>
		static constexpr auto starts_with( StringView &&...prefix_text ) noexcept {
			return where( [=]( daw::string_view text ) noexcept -> bool {
				return ( text.starts_with( prefix_text ) or ... );
			} );
		}

		/// Match any node with outer text who's value ends with the specified
		/// value
		template<typename... StringView>
		static constexpr auto ends_with( StringView &&...suffix_text ) noexcept {
			return where( [=]( daw::string_view text ) noexcept -> bool {
				return ( text.ends_with( suffix_text ) or ... );
			} );
		}

		/// Match any node with outer text who's value is equal to on of the
		/// specified values
		template<typename... StringView>
		static constexpr auto is( StringView &&...match_text ) noexcept {
			return where( [=]( daw::string_view text ) noexcept -> bool {
				return ( ( text == match_text ) or ... );
			} );
		}
	};

	struct match_inner_text {
		/// Match any node with inner_text that returns true for the predicate
		template<typename... Predicates>
		static constexpr auto where( daw::string_view html_doc,
		                             Predicates &&...preds ) noexcept {
			return [=]( auto const &node ) -> bool {
				return ( preds( node_inner_text( node, html_doc ) ) and ... );
			};
		}

		/// Match any node with inner_text that is empty
		static constexpr auto is_empty( daw::string_view html_doc ) noexcept {
			return where( html_doc, []( daw::string_view text ) noexcept -> bool {
				return text.empty( );
			} );
		}

		/// Match any node with inner_text that is not empty
		static constexpr auto is_not_empty( daw::string_view html_doc ) noexcept {
			return where( html_doc, []( daw::string_view text ) noexcept -> bool {
				return not text.empty( );
			} );
		}

		/// Match any node with inner_text that contains search_text
		static constexpr auto contains( daw::string_view html_doc,
		                                daw::string_view search_text ) noexcept {
			return where( html_doc, [search_text]( daw::string_view text ) noexcept {
				return text.find( search_text ) != daw::string_view::npos;
			} );
		}

		/// Match any node with inner text who's value starts with the specified
		/// value
		static constexpr auto starts_with( daw::string_view html_doc,
		                                   daw::string_view prefix_text ) noexcept {
			return where( html_doc, [prefix_text]( daw::string_view text ) noexcept {
				return text.starts_with( prefix_text );
			} );
		}

		/// Match any node with inner text who's value ends with the specified
		/// value
		static constexpr auto ends_with( daw::string_view html_doc,
		                                 daw::string_view suffix_text ) noexcept {
			return where( html_doc, [suffix_text]( daw::string_view text ) noexcept {
				return text.ends_with( suffix_text );
			} );
		}

		/// Match any node with inner text who's value is equal to the specified
		/// value
		static constexpr auto is( daw::string_view html_doc,
		                          daw::string_view match_text ) noexcept {
			return where( html_doc, [match_text]( daw::string_view text ) noexcept {
				return text == match_text;
			} );
		}

		/// Match any node with inner text who's value is not equal to the
		/// specified value
		static constexpr auto is_not( daw::string_view html_doc,
		                              daw::string_view match_text ) noexcept {
			return where( html_doc, [match_text]( daw::string_view text ) noexcept {
				return text != match_text;
			} );
		}
	};

	struct match_outer_text {
		/// Match any node with outer_text that returns true for the predicate
		template<typename... Predicates>
		static constexpr auto where( daw::string_view html_doc,
		                             Predicates &&...preds ) noexcept {
			return [=]( auto const &node ) -> bool {
				return ( preds( node_outer_text( node, html_doc ) ) and ... );
			};
		}

		/// Match any node with outer_text that is empty
		static constexpr auto is_empty( daw::string_view html_doc ) noexcept {
			return where( html_doc, []( daw::string_view text ) noexcept -> bool {
				return text.empty( );
			} );
		}

		/// Match any node with outer_text that is not empty
		static constexpr auto is_not_empty( daw::string_view html_doc ) noexcept {
			return where( html_doc, []( daw::string_view text ) noexcept -> bool {
				return not text.empty( );
			} );
		}

		/// Match any node with outer_text that contains search_text
		static constexpr auto contains( daw::string_view html_doc,
		                                daw::string_view search_text ) noexcept {
			return where( html_doc, [search_text]( daw::string_view text ) noexcept {
				return text.find( search_text ) != daw::string_view::npos;
			} );
		}

		/// Match any node with outer text who's value starts with the specified
		/// value
		static constexpr auto starts_with( daw::string_view html_doc,
		                                   daw::string_view prefix_text ) noexcept {
			return where( html_doc, [prefix_text]( daw::string_view text ) noexcept {
				return text.starts_with( prefix_text );
			} );
		}

		/// Match any node with outer text who's value ends with the specified
		/// value
		static constexpr auto ends_with( daw::string_view html_doc,
		                                 daw::string_view suffix_text ) noexcept {
			return where( html_doc, [suffix_text]( daw::string_view text ) noexcept {
				return text.ends_with( suffix_text );
			} );
		}

		/// Match any node with outer text who's value is equal to the specified
		/// value
		static constexpr auto is( daw::string_view html_doc,
		                          daw::string_view match_text ) noexcept {
			return where( html_doc, [match_text]( daw::string_view text ) noexcept {
				return text == match_text;
			} );
		}

		/// Match any node with outer text who's value is not equal to the
		/// specified value
		static constexpr auto is_not( daw::string_view html_doc,
		                              daw::string_view match_text ) noexcept {
			return where( html_doc, [match_text]( daw::string_view text ) noexcept {
				return text != match_text;
			} );
		}
	};

	struct match_tag {
		/// Match any node with where the tag type where all the Predicates return
		/// true
		template<typename... Predicates>
		static constexpr auto where( Predicates &&...preds ) noexcept {
			return [=]( auto const &node ) noexcept -> bool {
				return node.type == GUMBO_NODE_ELEMENT and
				       ( preds( node.v.element.tag ) and ... );
			};
		}

		/// Match any node with where the tag type where the tag type matches on
		/// of the specified types
		template<GumboTag... tags>
		static constexpr auto types = where( []( GumboTag tag_value ) {
			static_assert( sizeof...( tags ) > 0, "Must supply at least one tag" );
			return ( ( tag_value == tags ) | ... );
		} );
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
	constexpr auto operator||( MatchL &&lhs, MatchR &&rhs ) noexcept {
		return match_any{ DAW_FWD2( MatchL, lhs ), DAW_FWD2( MatchR, rhs ) };
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
	constexpr auto operator&&( MatchL &&lhs, MatchR &&rhs ) noexcept {
		return match_all{ DAW_FWD2( MatchL, lhs ), DAW_FWD2( MatchR, rhs ) };
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
