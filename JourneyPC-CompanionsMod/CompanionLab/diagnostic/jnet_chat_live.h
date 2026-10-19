#ifndef JNET_CHAT_LIVE_H
#define JNET_CHAT_LIVE_H
static struct {
    uint64_t room,generation,sequence,last_send;
    uint64_t ids[3],received[3],last_message[3];char names[3][33];unsigned count,chat;
} jnet_chat_session;
static void jnet_chat_show(uint64_t source,const char *body) {
    char name[33],line[160];jnet_sl_name(name,source);
    snprintf(line,sizeof(line),"%s: %s",name,body);jnet_chat_line(line);
}
static int jnet_chat_message(unsigned slot,const unsigned char *p,size_t bytes) {
    if(bytes<4 || memcmp(p,"JNC1",4))return 0;
    if(slot>=7 || !jnet_sl.room || !jnet_sl.chat || !jnet_sn.peers[slot].admitted)return 1;
    uint64_t source,sequence;char text[121];
    if(!jnet_chat_unpack(p,bytes,jnet_sl.room,jnet_sl.nonce,&source,&sequence,text))return 1;
    int host=jnet_sl.host==jnet_sl.identity;
    if((host && source!=jnet_sn.peers[slot].identity) || (!host && jnet_sn.peers[slot].identity!=jnet_sl.host) || source==jnet_sl.identity)return 1;
    unsigned at=0;for(;at<jnet_chat_session.count;++at)if(jnet_chat_session.ids[at]==source)break;
    if(at==jnet_chat_session.count || sequence<=jnet_chat_session.received[at])return 1;
    uint64_t now=GetTickCount64();jnet_chat_session.received[at]=sequence;
    if(jnet_chat_session.last_message[at] && now-jnet_chat_session.last_message[at]<500)return 1;
    jnet_chat_session.last_message[at]=now;jnet_chat_show(source,text);
    if(host)for(unsigned i=0;i<7;++i)if(i!=slot && jnet_sn.peers[i].admitted)
        jnet_sn_transmit(jnet_sn.peers[i].handle,p,(unsigned)bytes,9);
    return 1;
}
static void jnet_chat_lobby(struct jnet_lobby_view *view) {
    int changed=jnet_chat_session.room!=view->room || jnet_chat_session.generation!=jnet_sl.generation;
    if(changed){
        char departed[3][33]={{0}};unsigned departure_count=0;
        if(!view->room && jnet_chat_session.chat)for(unsigned i=0;i<jnet_chat_session.count;++i)
            if(jnet_chat_session.ids[i]!=view->self)memcpy(departed[departure_count++],jnet_chat_session.names[i],33);
        memset(&jnet_chat_session,0,sizeof(jnet_chat_session));jnet_chat_session.room=view->room;jnet_chat_session.generation=jnet_sl.generation;
        AcquireSRWLockExclusive(&jnet_chat_lock);memset(&jnet_chat,0,sizeof(jnet_chat));jnet_chat.room=view->room;jnet_chat_focus=0;ReleaseSRWLockExclusive(&jnet_chat_lock);
        jnet_chat_focus=0;
        for(unsigned i=0;i<departure_count;++i){char line[80];snprintf(line,sizeof(line),"%s disconnected",departed[i]);jnet_chat_line(line);}
    }
    jnet_chat_session.chat=view->chat;
    jnet_chat_allowed=!!(view->room && view->chat);
    if(!view->room)return;
    if(!view->member_count)return; /* Do not treat a missing cached roster as departures. */
    char names[3][33]={{0}};uint64_t received[3]={0},last[3]={0};
    for(unsigned i=0;i<view->member_count && i<3;++i){
        unsigned old=0;for(;old<jnet_chat_session.count;++old)if(jnet_chat_session.ids[old]==view->members[i])break;
        if(old<jnet_chat_session.count){memcpy(names[i],jnet_chat_session.names[old],33);received[i]=jnet_chat_session.received[old];last[i]=jnet_chat_session.last_message[old];}
        else {
            jnet_sl_name(names[i],view->members[i]);
            if(view->chat && view->members[i]!=view->self){char line[80];snprintf(line,sizeof(line),"%s connected",names[i]);jnet_chat_line(line);}
        }
        memcpy(view->member_names[i],names[i],33);
        if(view->members[i]==view->host)memcpy(view->host_name,names[i],33);
    }
    if(view->chat)for(unsigned i=0;i<jnet_chat_session.count;++i){
        unsigned found=0;for(unsigned j=0;j<view->member_count;++j)found|=jnet_chat_session.ids[i]==view->members[j];
        if(!found && jnet_chat_session.ids[i]!=view->self){char line[80];snprintf(line,sizeof(line),"%s disconnected",jnet_chat_session.names[i]);jnet_chat_line(line);}
    }
    jnet_chat_session.count=view->member_count;
    memcpy(jnet_chat_session.ids,view->members,sizeof(view->members));memcpy(jnet_chat_session.names,names,sizeof(names));
    memcpy(jnet_chat_session.received,received,sizeof(received));memcpy(jnet_chat_session.last_message,last,sizeof(last));
    char pending[121]={0};
    AcquireSRWLockExclusive(&jnet_chat_lock);
    if(GetTickCount64()-jnet_chat_session.last_send>=500){memcpy(pending,jnet_chat.pending,121);jnet_chat.pending[0]=0;}
    ReleaseSRWLockExclusive(&jnet_chat_lock);
    if(!pending[0] || !view->chat)return;
    unsigned char packet[JNET_CHAT_PACKET]={0};memcpy(packet,"JNC1",4);memcpy(packet+8,&view->room,8);memcpy(packet+16,&view->self,8);
    uint64_t sequence=++jnet_chat_session.sequence;memcpy(packet+24,&sequence,8);memcpy(packet+32,jnet_sl.nonce,32);memcpy(packet+64,pending,strlen(pending));
    unsigned sent=0,failed=0;
    for(unsigned i=0;i<7;++i)if(jnet_sn.peers[i].admitted){int result=jnet_sn_transmit(jnet_sn.peers[i].handle,packet,sizeof(packet),9);if(result==1)++sent;else ++failed;}
    jnet_chat_session.last_send=GetTickCount64();
    if(sent || view->member_count==1)jnet_chat_show(view->self,pending);
    if(failed || (!sent && view->member_count>1))jnet_chat_line("Message could not reach all players");
}
#endif
