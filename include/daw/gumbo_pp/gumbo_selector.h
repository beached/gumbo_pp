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

namespace daw::gumbo {
	struct match_attribute {
		/// Match any node that has an attribute where the predicate returns true
		template<typename Predicate>
		static constexpr auto where( Predicate p ) noexcept {
			return [p]( auto const &node ) {
				return details::find_attribute_if_impl(
				         gumbo_node_iterator_t( &node ),
				         [&]( GumboAttribute const &attr ) -> bool {
					         return p( daw::string_view( attr.name ),
					                   daw::string_view( attr.value ) );
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

		static constexpr auto exists( daw::string_view name ) noexcept {
			return
			  [name]( auto const &node ) { return attribute_exists( node, name ); };
		}

		struct name {
			// Match any node that has a matching attribute name
			static constexpr auto is( daw::string_view attribute_name ) noexcept {
				return where(
				  [attribute_name]( daw::string_view name, daw::string_view ) noexcept {
					  return name == attribute_name;
				  } );
			}

			// Match any node that does not have a matching attribute name
			static constexpr auto is_not( daw::string_view attribute_name ) noexcept {
				return [attribute_name]( auto const &node ) noexcept {
					where( [attribute_name]( daw::string_view name,
					                         daw::string_view ) noexcept {
						return name != attribute_name;
					} )( node ) or
					  get_attribute_count( node ) == 0;
				};
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
		/// Match any node with a class that returns true for the predicate
		template<typename Predicate>
		static constexpr auto where( Predicate p ) noexcept {
			return match_attribute::where(
			  [p]( daw::string_view attribute_name,
			       daw::string_view attribute_value ) noexcept -> bool {
				  return attribute_name == "class" and p( attribute_value );
			  } );
		}

		/// Match any node with a class that returns true for the predicate
		static constexpr auto is( daw::string_view class_name ) noexcept {
			return match_attribute::where(
			  [class_name]( daw::string_view attribute_name,
			                daw::string_view attribute_value ) noexcept -> bool {
				  return attribute_name == "class" and class_name == attribute_value;
			  } );
		}

		static constexpr auto is_not( daw::string_view class_name ) noexcept {
			return [class_name]( auto const &node ) {
				return match_attribute::where(
				         [class_name](
				           daw::string_view attribute_name,
				           daw::string_view attribute_value ) noexcept -> bool {
					         return attribute_name == "class" and
					                class_name != attribute_value;
				         } )( node ) or
				       match_attribute::has_none( node );
			};
		}
	};

	struct match_id {
		/// Match any node with a id that returns true for the predicate
		template<typename Predicate>
		static constexpr auto where( Predicate p ) noexcept {
			return match_attribute::where(
			  [p]( daw::string_view attribute_name,
			       daw::string_view attribute_value ) noexcept -> bool {
				  return attribute_name == "id" and p( attribute_value );
			  } );
		}

		/// Match any node with a id that returns true for the predicate
		static constexpr auto is( daw::string_view id_name ) noexcept {
			return match_attribute::where(
			  [id_name]( daw::string_view attribute_name,
			             daw::string_view attribute_value ) noexcept {
				  return attribute_name == "id" and id_name == attribute_value;
			  } );
		}

		/// Match any node with a id that returns true for the predicate
		static constexpr auto is_not( daw::string_view id_name ) noexcept {
			return match_attribute::where(
			  [id_name]( daw::string_view attribute_name,
			             daw::string_view attribute_value ) noexcept {
				  return attribute_name == "id" and id_name != attribute_value;
			  } );
		}
	};

	struct match_inner_text {
		/// Match any node with a id that returns true for the predicate
		template<typename Predicate>
		static constexpr auto where( daw::string_view html_doc,
		                             Predicate p ) noexcept {
			return [html_doc, p]( auto const &node ) -> bool {
				return p( node_inner_text( node, html_doc ) );
			};
		}

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
	};

	// For matching the content of tags like A
	struct match_content_text {
		/// Match any node with a id that returns true for the predicate
		template<typename Predicate>
		static constexpr auto where( Predicate p ) noexcept {
			return [p]( auto const &node ) -> bool {
				return p( node_content_text( node ) );
			};
		}

		static constexpr auto contains( daw::string_view search_text ) noexcept {
			return where( [search_text]( daw::string_view text ) noexcept -> bool {
				return text.find( search_text ) != daw::string_view::npos;
			} );
		}

		/// Match any node with outer text who's value starts with the specified
		/// value
		static constexpr auto starts_with( daw::string_view prefix_text ) noexcept {
			return where( [prefix_text]( daw::string_view text ) noexcept -> bool {
				return text.starts_with( prefix_text );
			} );
		}

		/// Match any node with outer text who's value ends with the specified
		/// value
		static constexpr auto ends_with( daw::string_view suffix_text ) noexcept {
			return where( [suffix_text]( daw::string_view text ) noexcept -> bool {
				return text.ends_with( suffix_text );
			} );
		}

		/// Match any node with outer text who's value is equal to the specified
		/// value
		static constexpr auto is( daw::string_view match_text ) noexcept {
			return where( [match_text]( daw::string_view text ) noexcept -> bool {
				return text == match_text;
			} );
		}
	};

	struct match_outer_text {
		/// Match any node with a id that returns true for the predicate
		template<typename Predicate>
		static constexpr auto where( daw::string_view html_doc,
		                             Predicate p ) noexcept {
			return [html_doc, p]( auto const &node ) -> bool {
				return p( node_outer_text( node, html_doc ) );
			};
		}

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
	};

	struct match_tag {
		/// Match any node with where the tag type where the Predicate returns
		/// true
		template<typename Predicate>
		static constexpr auto where( Predicate p ) noexcept {
			return [p]( auto const &node ) noexcept -> bool {
				return node.type == GUMBO_NODE_ELEMENT and p( node.v.element.tag );
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

	struct match {
		using attribute = match_attribute;
		using class_type = match_class;
		using id = match_id;
		using inner_text = match_inner_text;
		using outer_text = match_outer_text;
		using content_text = match_content_text;
		using tag = match_tag;
	};
} // namespace daw::gumbo
