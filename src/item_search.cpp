#include "item_search.h"

#include <map>
#include <utility>

#include "ammo.h"
#include "cata_utility.h"
#include "item.h"
#include "item_category.h"
#include "itype.h"
#include "iuse_actor.h"
#include "material.h"
#include "mtype.h"
#include "requirements.h"
#include "string_id.h"
#include "type_id.h"

std::pair<std::string, std::string> get_both( const std::string &a );

std::function<bool( const item & )> basic_item_filter( std::string filter )
{
    size_t colon;
    char flag = '\0';
    if( ( colon = filter.find( ':' ) ) != std::string::npos ) {
        if( colon >= 1 ) {
            flag = filter[colon - 1];
            filter = filter.substr( colon + 1 );
        }
    }
    switch( flag ) {
        // category
        case 'c':
            return [filter]( const item & i ) {
                return lcmatch( i.get_category().name(), filter );
            };
        // material
        case 'm':
            return [filter]( const item & i ) {
                return std::any_of( i.made_of().begin(), i.made_of().end(),
                [&filter]( const material_id & mat ) {
                    return lcmatch( mat->name(), filter );
                } );
            };
        // qualities
        case 'q':
            return [filter]( const item & i ) {
                return std::any_of( i.quality_of().begin(), i.quality_of().end(),
                [&filter]( const std::pair<quality_id, int> &e ) {
                    return lcmatch( e.first->name, filter );
                } );
            };
        // flag
        case 'f':
            return [filter]( const item & i ) {
                try {
                    if( i.has_flag( filter ) ) {
                        return true;
                    } else if( i.is_container() && !i.is_container_empty() ) {
                        return i.get_contained().has_flag( filter );
                    } else if( i.is_corpse() ) {
                        const mtype *corpse_mtype = i.get_mtype();
                        m_flag mf = io::string_to_enum<m_flag>( filter );
                        return corpse_mtype->has_flag( mf );
                    }
                    return false;
                } catch ( io::InvalidEnumString & ) {
                    return false;
                }
            };
        // ammo type
        case 'a':
            return [filter]( const item & i ) {
                if( i.is_ammo() ) {
                    if( lcmatch( i.ammo_type()->name(), filter ) ) {
                        return true;
                    }
                } else if( ( i.is_gun() && i.ammo_required() ) || i.is_magazine() ) {
                    for( const ammotype &at : i.ammo_types() ) {
                        if( lcmatch( at->name(), filter ) ) {
                            return true;
                        }
                    }
                } else if( i.is_bandolier() ) {
                    const bandolier_actor *ba = dynamic_cast<const bandolier_actor *>( i.type->get_use( "bandolier" )->get_actor_ptr() );
                    for( const ammotype &at : ba->ammo ) {
                        if( lcmatch( at->name(), filter ) ) {
                            return true;
                        }
                    }
                }
                return false;
            };
        // both
        case 'b':
            return [filter]( const item & i ) {
                auto pair = get_both( filter );
                return item_filter_from_string( pair.first )( i )
                       && item_filter_from_string( pair.second )( i );
            };
        // disassembled components
        case 'd':
            return [filter]( const item & i ) {
                const auto &components = i.get_uncraft_components();
                for( auto &component : components ) {
                    if( lcmatch( component.to_string(), filter ) ) {
                        return true;
                    }
                }
                return false;
            };
        // item notes
        case 'n':
            return [filter]( const item & i ) {
                const std::string note = i.get_var( "item_note" );
                return !note.empty() && lcmatch( note, filter );
            };
        // by name
        default:
            return [filter]( const item & a ) {
                return lcmatch( remove_color_tags( a.tname() ), filter );
            };
    }
}

std::function<bool( const item & )> item_filter_from_string( const std::string &filter )
{
    return filter_from_string<item>( filter, basic_item_filter );
}

std::pair<std::string, std::string> get_both( const std::string &a )
{
    size_t split_mark = a.find( ';' );
    return std::make_pair( a.substr( 0, split_mark - 1 ),
                           a.substr( split_mark + 1 ) );
}
