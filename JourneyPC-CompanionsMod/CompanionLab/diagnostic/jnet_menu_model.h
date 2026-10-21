#ifndef JNET_MENU_MODEL_H
#define JNET_MENU_MODEL_H
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "jnet_character.h"

#define JNET_SCARF_MAX 30u

enum jnet_room_access { JNET_PUBLIC, JNET_PRIVATE };
enum jnet_room_mode { JNET_JOURNEY, JNET_ROCKET_DEATH_MATCH };
enum jnet_join_step {
    JNET_JOIN_IDLE, JNET_JOIN_CONFIRM, JNET_JOIN_DEPART,
    JNET_JOIN_LOAD, JNET_JOIN_CONNECT
};
enum jnet_menu_result {
    JNET_MENU_OK, JNET_MENU_INVALID, JNET_MENU_BUSY,
    JNET_MENU_FULL, JNET_MENU_UNAVAILABLE, JNET_MENU_STALE
};
struct jnet_room_listing {
    uint64_t id, revision, host;
    unsigned mode, level, players, capacity, access, password;
    char name[33], level_name[33];
};
struct jnet_menu_session {
    uint64_t room;
    unsigned level, mode, step;
    struct jnet_room_listing pending, current;
};
struct jnet_appearance_choice {
    unsigned outfit, scarf_segments, character, symbol, infinite_scarf, locked_scarf, variant;
};
struct jnet_appearance_edit {
    struct jnet_appearance_choice applied, draft, pending;
    unsigned awaiting;
};
static inline unsigned jnet_appearance_row_count(unsigned character) {
    return 6u+(jnet_character_variant_limit(character)!=0 || jnet_character_is_flow(character));
}
static inline void jnet_appearance_observe(struct jnet_appearance_edit *edit,unsigned outfit,unsigned scarf,unsigned symbol,int initial) {
    if(!edit || outfit>6 || scarf>JNET_SCARF_MAX || symbol>20 || edit->awaiting)return;
    if(initial || edit->draft.outfit==edit->applied.outfit)edit->draft.outfit=outfit;
    if(!initial && edit->applied.locked_scarf)scarf=edit->applied.scarf_segments;
    if(initial || edit->draft.scarf_segments==edit->applied.scarf_segments)edit->draft.scarf_segments=scarf;
    if(initial || edit->draft.symbol==edit->applied.symbol)edit->draft.symbol=symbol;
    edit->applied.outfit=outfit;edit->applied.scarf_segments=scarf;edit->applied.symbol=symbol;
    if(initial){
        edit->applied.character=edit->draft.character=0;
        edit->applied.variant=edit->draft.variant=0;
        edit->applied.infinite_scarf=edit->draft.infinite_scarf=0;
        edit->applied.locked_scarf=edit->draft.locked_scarf=0;
    }
}
static inline int jnet_listing_valid(const struct jnet_room_listing *r) {
    return r && r->id && r->revision && r->host && r->mode<=JNET_ROCKET_DEATH_MATCH &&
        r->level<12 && r->capacity>=2 && r->players && r->players<=r->capacity &&
        r->access<=JNET_PRIVATE && r->password<=1;
}
static inline int jnet_listing_visible(const struct jnet_room_listing *r,uint64_t current_room) {
    return jnet_listing_valid(r) && (r->access==JNET_PUBLIC || r->id==current_room);
}
static inline int jnet_room_set_access(struct jnet_room_listing *r,uint64_t actor,unsigned access) {
    if(!jnet_listing_valid(r) || actor!=r->host || access>JNET_PRIVATE || r->revision==UINT64_MAX)
        return JNET_MENU_INVALID;
    if(r->access!=access) {r->access=access;++r->revision;}
    return JNET_MENU_OK;
}
static inline int jnet_join_select(struct jnet_menu_session *s,const struct jnet_room_listing *r,
                                  unsigned supported_modes,unsigned supported_capacity) {
    if(!s || !jnet_listing_valid(r))return JNET_MENU_INVALID;
    if(s->step!=JNET_JOIN_IDLE)return JNET_MENU_BUSY;
    if(!(supported_modes&(1u<<r->mode)) || r->capacity>supported_capacity)return JNET_MENU_UNAVAILABLE;
    if(s->room==r->id)return JNET_MENU_OK;
    if(r->players==r->capacity)return JNET_MENU_FULL;
    s->pending=*r;s->step=JNET_JOIN_CONFIRM;return JNET_MENU_OK;
}
static inline void jnet_join_after_departure(struct jnet_menu_session *s) {
    s->step=s->level==s->pending.level && s->mode==s->pending.mode ? JNET_JOIN_CONNECT : JNET_JOIN_LOAD;
}
static inline int jnet_join_confirm(struct jnet_menu_session *s,const struct jnet_room_listing *current,
                                   int admission_granted) {
    if(!s || s->step!=JNET_JOIN_CONFIRM)return JNET_MENU_INVALID;
    if(!jnet_listing_valid(current) || current->id!=s->pending.id || current->revision!=s->pending.revision)
        return JNET_MENU_STALE;
    if(!admission_granted)return JNET_MENU_UNAVAILABLE;
    if(current->players>=current->capacity)return JNET_MENU_FULL;
    if(s->room)s->step=JNET_JOIN_DEPART;else jnet_join_after_departure(s);
    return JNET_MENU_OK;
}
static inline int jnet_join_departed(struct jnet_menu_session *s) {
    if(!s || s->step!=JNET_JOIN_DEPART)return JNET_MENU_INVALID;
    s->room=0;memset(&s->current,0,sizeof(s->current));jnet_join_after_departure(s);return JNET_MENU_OK;
}
static inline int jnet_join_loaded(struct jnet_menu_session *s,unsigned level,unsigned mode) {
    if(!s || s->step!=JNET_JOIN_LOAD)return JNET_MENU_INVALID;
    if(level!=s->pending.level || mode!=s->pending.mode)return JNET_MENU_STALE;
    s->level=level;s->mode=mode;s->step=JNET_JOIN_CONNECT;return JNET_MENU_OK;
}
static inline int jnet_join_connected(struct jnet_menu_session *s,uint64_t room,uint64_t revision) {
    if(!s || s->step!=JNET_JOIN_CONNECT)return JNET_MENU_INVALID;
    if(room!=s->pending.id || revision!=s->pending.revision)return JNET_MENU_STALE;
    memset(&s->current,0,sizeof(s->current));s->room=room;s->step=JNET_JOIN_IDLE;memset(&s->pending,0,sizeof(s->pending));return JNET_MENU_OK;
}
static inline int jnet_join_cancel(struct jnet_menu_session *s) {
    if(!s)return JNET_MENU_INVALID;
    if(s->step!=JNET_JOIN_CONFIRM)return JNET_MENU_BUSY;
    s->step=JNET_JOIN_IDLE;memset(&s->pending,0,sizeof(s->pending));return JNET_MENU_OK;
}
static inline int jnet_lobby_refresh(struct jnet_menu_session *s,const struct jnet_room_listing *r) {
    if(!s || !jnet_listing_valid(r))return JNET_MENU_INVALID;
    if(!s->room || r->id!=s->room || r->revision<s->current.revision)return JNET_MENU_STALE;
    s->current=*r;return JNET_MENU_OK;
}
struct jnet_lobby_labels {
    const char *action;
    char name[48],level[48],population[48];
};
static inline struct jnet_lobby_labels jnet_lobby_labels(const struct jnet_menu_session *s) {
    struct jnet_lobby_labels out={0};
    out.action=s && s->room ? "Leave" : "Join";
    if(!s || !s->room) {
        snprintf(out.name,sizeof(out.name),"Select a lobby");return out;
    }
    const struct jnet_room_listing *r=&s->current;
    if(!jnet_listing_valid(r) || r->id!=s->room) {
        snprintf(out.name,sizeof(out.name),"Updating lobby");return out;
    }
    snprintf(out.name,sizeof(out.name),"%.32s",r->name[0]?r->name:"Unnamed lobby");
    if(r->level_name[0])snprintf(out.level,sizeof(out.level),"%.32s",r->level_name);
    else snprintf(out.level,sizeof(out.level),"Level %u",r->level);
    snprintf(out.population,sizeof(out.population),"Players %u/%u",r->players,r->capacity);
    return out;
}
enum jnet_selector { JNET_SELECT_ROBE, JNET_SELECT_SCARF, JNET_SELECT_SYMBOL, JNET_SELECT_CHARACTER, JNET_SELECT_SCARF_MODE, JNET_SELECT_SCARF_GROW, JNET_SELECT_VARIANT };
static inline int jnet_appearance_step(struct jnet_appearance_edit *edit,unsigned selector,int direction,unsigned max_scarf) {
    if(!edit || (direction!=1 && direction!=-1))return JNET_MENU_INVALID;
    if(edit->awaiting)return JNET_MENU_BUSY;
    unsigned *value,limit;
    switch(selector) {
        case JNET_SELECT_VARIANT:
            if(jnet_character_is_flow(edit->draft.character)){
                edit->draft.character=edit->draft.character==JNET_CHARACTER_FLOW_1?JNET_CHARACTER_FLOW_2:JNET_CHARACTER_FLOW_1;
                edit->draft.variant=0;return JNET_MENU_OK;
            }
            value=&edit->draft.variant;
            limit=jnet_character_variant_limit(edit->draft.character);
            if(!limit)return JNET_MENU_UNAVAILABLE;
            break;
        case JNET_SELECT_ROBE:value=&edit->draft.outfit;limit=6;break;
        case JNET_SELECT_SCARF:value=&edit->draft.scarf_segments;limit=max_scarf;break;
        case JNET_SELECT_SYMBOL:value=&edit->draft.symbol;limit=20;break;
        case JNET_SELECT_SCARF_GROW:value=&edit->draft.locked_scarf;limit=1;break;
        case JNET_SELECT_SCARF_MODE:value=&edit->draft.infinite_scarf;limit=1;break;
        case JNET_SELECT_CHARACTER:
            if(edit->draft.character>=JNET_CHARACTER_COUNT)return JNET_MENU_INVALID;
            {
                const unsigned order[]={JNET_CHARACTER_WAYFARER,JNET_CHARACTER_LONGTAIL,JNET_CHARACTER_WHALE,
                    JNET_CHARACTER_WAR_MACHINE,JNET_CHARACTER_FLOW_1,JNET_CHARACTER_ROCKET};
                unsigned count=sizeof(order)/sizeof(order[0]);
                unsigned current=jnet_character_is_flow(edit->draft.character)?JNET_CHARACTER_FLOW_1:edit->draft.character;
                for(unsigned i=0;i<count;++i)if(order[i]==current){
                    edit->draft.variant=0;
                    edit->draft.character=order[(i+count+direction)%count];
                    return JNET_MENU_OK;
                }
            }
            return JNET_MENU_INVALID;
        default:return JNET_MENU_INVALID;
    }
    if(*value>limit)return JNET_MENU_INVALID;
    if(direction>0)*value=*value==limit?0:*value+1;
    else *value=*value==0?limit:*value-1;
    return JNET_MENU_OK;
}
static inline int jnet_scarf_white(const struct jnet_appearance_edit *edit) {
    return edit && edit->draft.outfit>=4 && edit->draft.outfit<=6;
}
static inline int jnet_level_host(const struct jnet_menu_session *session,uint64_t identity) {
    return session && identity && session->room && session->current.id==session->room &&
        jnet_listing_valid(&session->current) && session->current.host==identity;
}
static inline int jnet_appearance_apply(struct jnet_appearance_edit *edit,unsigned max_scarf,unsigned supported_characters) {
    if(!edit || edit->draft.outfit>6 || edit->draft.character>=JNET_CHARACTER_COUNT || edit->draft.symbol>20 || edit->draft.infinite_scarf>1 || edit->draft.locked_scarf>1 || edit->draft.scarf_segments>max_scarf)
        return JNET_MENU_INVALID;
    if(edit->awaiting)return JNET_MENU_BUSY;
    if(edit->draft.variant>jnet_character_variant_limit(edit->draft.character))return JNET_MENU_INVALID;
    if(edit->draft.character && !(supported_characters&(1u<<edit->draft.character)))return JNET_MENU_UNAVAILABLE;
    edit->pending=edit->draft;edit->awaiting=1;return JNET_MENU_OK;
}
static inline int jnet_appearance_complete(struct jnet_appearance_edit *edit,int applied) {
    if(!edit || !edit->awaiting)return JNET_MENU_INVALID;
    if(applied)edit->applied=edit->pending;
    edit->awaiting=0;return JNET_MENU_OK;
}
#endif
