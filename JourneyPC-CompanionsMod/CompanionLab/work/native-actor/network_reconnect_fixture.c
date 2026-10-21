#include <assert.h>
#include <stdio.h>
typedef int LONG;
#define JC_ACTOR_RUNTIME_BUSY 43
static LONG JCProbeNetwork[24],JCProbeTraveler[44];
static unsigned traveler_local_guard,traveler_local_state,traveler_network_active,traveler_stream_enabled,starts,logs;
static void *traveler_actor;
static struct {int paused;} traveler_network;
static int offline=1;
static int traveler_local_offline(void){traveler_local_guard=offline?0:5;return offline;}
static int traveler_network_start(void){++starts;traveler_network.paused=0;return 0;}
static void message(const char *s){assert(s && *s);++logs;}
static void traveler_local_network_guard(void) {
    if(!JCProbeNetwork[23]) {
        char line[160];
        JCProbeNetwork[23]=(LONG)traveler_local_guard;
        snprintf(line,sizeof(line),"[JC] network ownership guard reason=%u native-state=%u; restart required",
            traveler_local_guard,traveler_local_state);message(line);
    }
    traveler_network.paused=1;JCProbeNetwork[0]=-1;JCProbeNetwork[2]=123;
}
static int traveler_local_network_connect(void) {
    if(JCProbeNetwork[23])return 123;
    if(traveler_local_state>=5)return 124;
    if(JCProbeTraveler[0]!=3)return JC_ACTOR_RUNTIME_BUSY;
    if(!traveler_local_offline()) {traveler_local_network_guard();return 123;}
    if(!traveler_network_active && (traveler_local_state || traveler_actor || traveler_stream_enabled))
        return JC_ACTOR_RUNTIME_BUSY;
    return traveler_network_start();
}

int main(void){
 JCProbeTraveler[0]=3;
 assert(!traveler_local_network_connect() && starts==1);
 traveler_network_active=1;
 for(unsigned i=0;i<5;++i){traveler_local_state=i;assert(!traveler_local_network_connect());}
 traveler_local_state=5;assert(traveler_local_network_connect()==124);
 traveler_local_state=6;assert(traveler_local_network_connect()==124);
 traveler_local_state=2;offline=0;assert(traveler_local_network_connect()==123);
 assert(JCProbeNetwork[23]==5 && traveler_network.paused && logs==1);
 unsigned before=starts;offline=1;
 for(unsigned i=0;i<100;++i){assert(traveler_local_network_connect()==123);traveler_local_network_guard();}
 assert(starts==before && logs==1 && JCProbeNetwork[23]==5 && traveler_network.paused);
 puts("Production reconnect control: valid resumes, stopped receiver refusal, first failure retention and 100 repeated Connect attempts passed.");
}
