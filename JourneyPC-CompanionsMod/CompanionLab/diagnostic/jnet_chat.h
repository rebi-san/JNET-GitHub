#ifndef JNET_CHAT_H
#define JNET_CHAT_H
#define JNET_CHAT_LIMIT 120
#define JNET_CHAT_PACKET 184
static inline int jnet_chat_unpack(const unsigned char *p,size_t bytes,uint64_t room,const char nonce[33],uint64_t *source,uint64_t *sequence,char text[121]) {
    if(bytes!=JNET_CHAT_PACKET || memcmp(p,"JNC1",4))return 0;
    uint64_t packet_room;memcpy(&packet_room,p+8,8);
    if(p[4] || p[5] || p[6] || p[7] || packet_room!=room || memcmp(p+32,nonce,32))return 0;
    memcpy(source,p+16,8);memcpy(sequence,p+24,8);
    if(!*source || !*sequence)return 0;
    size_t n=0;while(n<JNET_CHAT_LIMIT && p[64+n]){if(p[64+n]<32 || p[64+n]>126)return 0;++n;}
    if(!n)return 0;
    memcpy(text,p+64,n);text[n]=0;return 1;
}
#if defined(_WIN32) || defined(JNET_CHAT_FIXTURE)
static SRWLOCK jnet_chat_lock=SRWLOCK_INIT;
static _Atomic unsigned jnet_chat_focus,jnet_chat_allowed,jnet_chat_gameplay;
static struct {
    uint64_t room,input_room,revision,last_activity,shown;
    char input[121],pending[121],lines[5][160];
    uint64_t expires[5];
} jnet_chat;
static void jnet_chat_expire(uint64_t now) {
    if(!jnet_chat_focus && jnet_chat.last_activity && now-jnet_chat.last_activity>=10400){
        memset(jnet_chat.lines,0,sizeof(jnet_chat.lines));
        memset(jnet_chat.expires,0,sizeof(jnet_chat.expires));
        jnet_chat.shown=jnet_chat.last_activity=0;
        ++jnet_chat.revision;
    }
}
static void jnet_chat_line(const char *text) {
    AcquireSRWLockExclusive(&jnet_chat_lock);
    jnet_chat_expire(GetTickCount64());
    memmove(jnet_chat.lines,jnet_chat.lines+1,sizeof(jnet_chat.lines[0])*4);
    memmove(jnet_chat.expires,jnet_chat.expires+1,sizeof(jnet_chat.expires[0])*4);
    uint64_t now=GetTickCount64();
    if(!jnet_chat.last_activity || now-jnet_chat.last_activity>10400)jnet_chat.shown=now;
    jnet_chat.last_activity=now;
    snprintf(jnet_chat.lines[4],160,"%s",text);jnet_chat.expires[4]=GetTickCount64()+10000;++jnet_chat.revision;
    ReleaseSRWLockExclusive(&jnet_chat_lock);
}
static inline void jnet_chat_cancel(void) {
    jnet_chat_focus=0;
    AcquireSRWLockExclusive(&jnet_chat_lock);jnet_chat.input[0]=0;++jnet_chat.revision;ReleaseSRWLockExclusive(&jnet_chat_lock);
}
#if defined(_WIN32) || defined(JNET_CHAT_INPUT_FIXTURE)
static int jnet_chat_window(UINT msg,WPARAM wp,LPARAM lp) {
    static unsigned opening;
    if(msg==WM_KILLFOCUS){jnet_chat_cancel();return 0;}
    if(msg==WM_KEYDOWN && wp=='T' && !(lp&(1LL<<30)) && !jnet_chat_focus && jnet_chat_allowed && jnet_chat_gameplay) {
        AcquireSRWLockExclusive(&jnet_chat_lock);jnet_chat_expire(GetTickCount64());jnet_chat.input_room=jnet_chat.room;jnet_chat_focus=1;opening=1;ReleaseSRWLockExclusive(&jnet_chat_lock);return 1;
    }
    if(!jnet_chat_focus)return 0;
    if(msg==WM_KEYDOWN && wp==VK_ESCAPE){jnet_chat_cancel();return 1;}
    if(msg==WM_CHAR) {
        AcquireSRWLockExclusive(&jnet_chat_lock);
        if(!jnet_chat_focus){ReleaseSRWLockExclusive(&jnet_chat_lock);return 1;}
        size_t n=strlen(jnet_chat.input);
        /* WM_CHAR for the opening T follows its keydown. */
        if(opening && (wp=='t' || wp=='T')){opening=0;ReleaseSRWLockExclusive(&jnet_chat_lock);return 1;}
        opening=0;
        if(wp==13){if(n && jnet_chat.input_room==jnet_chat.room && !jnet_chat.pending[0])memcpy(jnet_chat.pending,jnet_chat.input,n+1);jnet_chat.input[0]=0;jnet_chat_focus=0;}
        else if(wp==8){if(n)jnet_chat.input[n-1]=0;}
        else if(wp>=32 && wp<127 && n<JNET_CHAT_LIMIT){jnet_chat.input[n]=(char)wp;jnet_chat.input[n+1]=0;}
        jnet_chat.last_activity=GetTickCount64();
        ++jnet_chat.revision;ReleaseSRWLockExclusive(&jnet_chat_lock);return 1;
    }
    return msg==WM_KEYDOWN || msg==WM_KEYUP;
}
#endif
#endif
#endif
