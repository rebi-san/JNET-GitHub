#ifndef JNET_GROUP_TRAVEL_H
#define JNET_GROUP_TRAVEL_H
#include <stdint.h>
#include <string.h>

#define JNET_SUPPORTED_PLAYERS 3u
#define JNET_PLAYER_STORAGE 8u
enum jnet_travel_phase { JNET_TRAVEL_IDLE, JNET_TRAVEL_VOTE, JNET_TRAVEL_DRAIN, JNET_TRAVEL_LOAD, JNET_TRAVEL_DONE, JNET_TRAVEL_CANCELLED };
enum jnet_travel_reply { JNET_TRAVEL_PENDING, JNET_TRAVEL_YES, JNET_TRAVEL_NO, JNET_TRAVEL_TIMEOUT };
struct jnet_group_travel {
    uint64_t session, generation, proposal, host, deadline;
    uint64_t members[JNET_PLAYER_STORAGE];
    unsigned count, level, phase, drained, loaded;
    unsigned replies[JNET_PLAYER_STORAGE];
};
static inline unsigned jnet_travel_slot(const struct jnet_group_travel *t,uint64_t identity) {
    for(unsigned i=0;i<t->count;++i)if(t->members[i]==identity)return i;
    return JNET_PLAYER_STORAGE;
}
static inline unsigned jnet_travel_participants(const struct jnet_group_travel *t) {
    unsigned mask=0;
    for(unsigned i=0;i<t->count;++i)if(t->replies[i]==JNET_TRAVEL_YES)mask|=1u<<i;
    return mask;
}
static inline int jnet_travel_begin(struct jnet_group_travel *t,uint64_t session,uint64_t generation,
    uint64_t proposal,uint64_t host,const uint64_t *members,unsigned count,unsigned level,uint64_t now,uint64_t timeout) {
    if(!t || !session || !generation || !proposal || !host || !members || !count ||
       count>JNET_SUPPORTED_PLAYERS || level>=12 || !timeout || timeout>UINT64_MAX-now)return 0;
    if(t->phase==JNET_TRAVEL_VOTE || t->phase==JNET_TRAVEL_DRAIN || t->phase==JNET_TRAVEL_LOAD)return 0;
    if(t->session==session && t->generation==generation && proposal<=t->proposal)return 0;
    unsigned host_slot=JNET_PLAYER_STORAGE;
    for(unsigned i=0;i<count;++i) {
        if(!members[i])return 0;
        if(members[i]==host)host_slot=i;
        for(unsigned j=0;j<i;++j)if(members[j]==members[i])return 0;
    }
    if(host_slot==JNET_PLAYER_STORAGE)return 0;
    memset(t,0,sizeof(*t));t->session=session;t->generation=generation;t->proposal=proposal;
    t->host=host;t->count=count;t->level=level;t->deadline=now+timeout;t->phase=JNET_TRAVEL_VOTE;
    memcpy(t->members,members,count*sizeof(*members));t->replies[host_slot]=JNET_TRAVEL_YES;
    return 1;
}
static inline int jnet_travel_matches(const struct jnet_group_travel *t,uint64_t session,uint64_t generation,uint64_t proposal) {
    return t && t->session==session && t->generation==generation && t->proposal==proposal;
}
static inline int jnet_travel_answer(struct jnet_group_travel *t,uint64_t session,uint64_t generation,
    uint64_t proposal,uint64_t authenticated_peer,unsigned reply,uint64_t now) {
    if(!jnet_travel_matches(t,session,generation,proposal) || t->phase!=JNET_TRAVEL_VOTE ||
       now>=t->deadline || (reply!=JNET_TRAVEL_YES && reply!=JNET_TRAVEL_NO))return 0;
    unsigned slot=jnet_travel_slot(t,authenticated_peer);
    if(slot==JNET_PLAYER_STORAGE || authenticated_peer==t->host)return 0;
    if(t->replies[slot]!=JNET_TRAVEL_PENDING)return t->replies[slot]==reply;
    t->replies[slot]=reply;return 1;
}
static inline int jnet_travel_close_vote(struct jnet_group_travel *t,uint64_t now,uint64_t drain_timeout) {
    if(!t || t->phase!=JNET_TRAVEL_VOTE || !drain_timeout || drain_timeout>UINT64_MAX-now)return 0;
    for(unsigned i=0;i<t->count;++i)if(t->replies[i]==JNET_TRAVEL_PENDING) {
        if(now<t->deadline)return 0;
        t->replies[i]=JNET_TRAVEL_TIMEOUT;
    }
    t->phase=JNET_TRAVEL_DRAIN;t->deadline=now+drain_timeout;return 1;
}
static inline int jnet_travel_drained(struct jnet_group_travel *t,uint64_t session,uint64_t generation,
    uint64_t proposal,uint64_t authenticated_peer,uint64_t now) {
    if(!jnet_travel_matches(t,session,generation,proposal) || t->phase!=JNET_TRAVEL_DRAIN || now>=t->deadline)return 0;
    unsigned slot=jnet_travel_slot(t,authenticated_peer);
    if(slot==JNET_PLAYER_STORAGE || t->replies[slot]!=JNET_TRAVEL_YES)return 0;
    t->drained|=1u<<slot;return 1;
}
static inline int jnet_travel_commit(struct jnet_group_travel *t,uint64_t authenticated_host,uint64_t now,uint64_t load_timeout) {
    if(!t || t->phase!=JNET_TRAVEL_DRAIN || authenticated_host!=t->host ||
       !load_timeout || load_timeout>UINT64_MAX-now)return 0;
    if(now>=t->deadline){t->phase=JNET_TRAVEL_CANCELLED;return 0;}
    if(t->drained!=jnet_travel_participants(t))return 0;
    t->phase=JNET_TRAVEL_LOAD;t->deadline=now+load_timeout;return 1;
}
static inline int jnet_travel_loaded(struct jnet_group_travel *t,uint64_t session,uint64_t generation,
    uint64_t proposal,uint64_t authenticated_peer,unsigned actual_level,uint64_t now) {
    if(!jnet_travel_matches(t,session,generation,proposal) || t->phase!=JNET_TRAVEL_LOAD ||
       actual_level!=t->level || now>=t->deadline)return 0;
    unsigned slot=jnet_travel_slot(t,authenticated_peer);
    if(slot==JNET_PLAYER_STORAGE || t->replies[slot]!=JNET_TRAVEL_YES)return 0;
    t->loaded|=1u<<slot;
    if(t->loaded==jnet_travel_participants(t))t->phase=JNET_TRAVEL_DONE;
    return 1;
}
static inline void jnet_travel_expire(struct jnet_group_travel *t,uint64_t now,int host_present) {
    if(!t || t->phase==JNET_TRAVEL_IDLE || t->phase>=JNET_TRAVEL_DONE)return;
    if(!host_present || (t->phase!=JNET_TRAVEL_VOTE && now>=t->deadline))t->phase=JNET_TRAVEL_CANCELLED;
}
#endif
