#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include "jnet_session_log.h"
typedef int32_t LONG;
typedef struct {const char *text;size_t length;} lua_State;
static const char *tolstring(lua_State *state,int index,size_t *length){
    assert(index==1);*length=state->length;return state->text;
}
static LONG InterlockedIncrement(volatile LONG *target){return ++*target;}
static LONG InterlockedExchange(volatile LONG *target,LONG value){LONG old=*target;*target=value;return old;}
static unsigned jnet_diagnostic_logs;
static volatile LONG jnet_diagnostic_suppressed,JNETObservation,JNETPhase;
static struct jnet_session_logs dll_logs;
static struct jnet_session_logs *session_logs=&dll_logs;
#include "jnet_dll_logging_under_test.h"
static void lua_line(const char *text,size_t length){lua_State state={text,length};assert(lua_log(&state)==0);}
static void dll_logging(void){
    char tail[512],line[80];
    lua_line(NULL,0);assert(!atomic_load(&dll_logs.runtime.count));
    for(unsigned value=0;value<=8;++value){
        int n=snprintf(line,sizeof(line),"[JNET] observation_state=%u",value);
        JNETObservation=-1;lua_line(line,(size_t)n);assert(JNETObservation==(LONG)value);
    }
    for(unsigned value=10;value<=18;++value){
        int n=snprintf(line,sizeof(line),"[JNET] sample_stage=%u",value);
        JNETPhase=-1;lua_line(line,(size_t)n);assert(JNETPhase==(LONG)value);
    }
    lua_line("[JNET] sample_stage=15",sizeof("[JNET] sample_stage=15")-1);
    assert(JNETObservation==8 && JNETPhase==15);
    assert(jnet_log_tail(&dll_logs.runtime,tail,sizeof(tail)) && strstr(tail,"[JNET] observation_state=8\r\n") && strstr(tail,"[JNET] sample_stage=15\r\n"));
    const char *invalid_states[]={"[JNET] observation_state=","[JNET] observation_state=9","[JNET] observation_state=-1",
        "[JNET] observation_state=08","[JNET] observation_state=8x","[JNET] sample_stage=9",
        "[JNET] sample_stage=19","[JNET] sample_stage=015","[JNET] sample_stage=15x"};
    for(unsigned i=0;i<sizeof(invalid_states)/sizeof(invalid_states[0]);++i){
        lua_line(invalid_states[i],strlen(invalid_states[i]));assert(JNETObservation==8 && JNETPhase==15);
    }
    uint32_t before=atomic_load(&dll_logs.runtime.count);
    for(size_t length=0;length<6;++length){
        char *short_line=malloc(length?length:1);assert(short_line);
        memcpy(short_line,"[JNET]",length);lua_line(short_line,length);free(short_line);
    }
    const char *invalid_prefixes[]={"[JC] prior","[JNETX] invalid","JNET wrong"};
    for(unsigned i=0;i<sizeof(invalid_prefixes)/sizeof(invalid_prefixes[0]);++i)
        lua_line(invalid_prefixes[i],strlen(invalid_prefixes[i]));
    char control[]="[JNET] payload";
    for(unsigned value=0;value<=255;++value)if(value<32 || value>126){
        control[sizeof(control)-2]=(char)value;lua_line(control,sizeof(control)-1);
    }
    assert(atomic_load(&dll_logs.runtime.count)==before && JNETObservation==8 && JNETPhase==15);
    const char *observations[]={"[JNET] visibility ","[JNET] appearance actor=","[JNET] glyph ","[JNET] floating glyph "};
    for(unsigned i=0;i<sizeof(observations)/sizeof(observations[0]);++i){
        size_t length=strlen(observations[i]);
        assert(jnet_observation_line(observations[i],length));
        assert(!jnet_observation_line(observations[i],length-1));
        lua_line(observations[i],length);
    }
    assert(jnet_diagnostic_suppressed==4 && atomic_load(&dll_logs.runtime.count)==before);
    jnet_diagnostic_logs=1;
    for(unsigned i=0;i<sizeof(observations)/sizeof(observations[0]);++i)
        lua_line(observations[i],strlen(observations[i]));
    assert(jnet_diagnostic_suppressed==4 && atomic_load(&dll_logs.runtime.count)==before+4);
    jnet_diagnostic_logs=0;
    lua_line("[JNET] normal status",sizeof("[JNET] normal status")-1);
    assert(atomic_load(&dll_logs.runtime.count)==before+5);
    char *maximum=malloc(481);assert(maximum);memset(maximum,'x',481);memcpy(maximum,"[JNET]",6);
    lua_line(maximum,480);
    assert(jnet_log_tail(&dll_logs.runtime,tail,483)==482 && !memcmp(tail,maximum,480) && !memcmp(tail+480,"\r\n",3));
    before=atomic_load(&dll_logs.runtime.count);lua_line(maximum,481);
    assert(atomic_load(&dll_logs.runtime.count)==before);free(maximum);
    puts("DLL logging: JNET state/phase parsing, disabled observation filtering, malformed/ASCII rejection and 480-byte input bounds passed.");
}
static struct jnet_memory_log logdata;
static void *writer(void *arg){
    char text[40];
    for(unsigned i=0;i<200;++i){
        int n=snprintf(text,sizeof(text),"writer %lu entry %u\n",(unsigned long)(uintptr_t)arg,i);
        assert(jnet_log_append(&logdata,text,(size_t)n));
    }
    return NULL;
}
static _Atomic unsigned finished;
static void *ring_writer(void *arg){
    char text[64];memset(text,(int)(uintptr_t)arg,sizeof(text));
    for(unsigned i=0;i<10000;++i)assert(jnet_log_append(&logdata,text,sizeof(text)));
    atomic_fetch_add(&finished,1);return NULL;
}
static void *ring_reader(void *arg){
    (void)arg;char text[JNET_LOG_BYTES];
    while(atomic_load(&finished)<4){
        uint32_t count=atomic_load(&logdata.count),start=count>JNET_LOG_ROWS?count-JNET_LOG_ROWS:0;
        for(uint32_t i=start;i<count;++i){
            size_t n=jnet_log_read_row(&logdata,i,text);
            if(n){assert(n==64);for(size_t j=1;j<n;++j)assert(text[j]==text[0]);}
        }
    }
    return NULL;
}
int main(void){
    dll_logging();
    pthread_t threads[4];char tail[128];
    assert(jnet_log_tail(&logdata,tail,sizeof(tail))==0 && tail[0]==0);
    assert(jnet_log_append(&logdata,"older\n",6));
    assert(jnet_log_append(&logdata,"newer\n",6));
    assert(jnet_log_tail(&logdata,tail,sizeof(tail))==12 && !strcmp(tail,"older\nnewer\n"));
    assert(jnet_log_tail(&logdata,tail,10)==9 && !strcmp(tail,"er\nnewer\n"));
    assert(jnet_log_tail(&logdata,tail,1)==0 && tail[0]==0);
    assert(jnet_log_tail(&logdata,NULL,0)==0);
    memset(&logdata,0,sizeof(logdata));
    for(uintptr_t i=0;i<4;++i)assert(!pthread_create(&threads[i],NULL,writer,(void *)i));
    for(unsigned i=0;i<4;++i)assert(!pthread_join(threads[i],NULL));
    assert(atomic_load(&logdata.count)==800);
    for(unsigned i=0;i<800;++i)assert(atomic_load(&logdata.rows[i].length)>0);
    assert(jnet_log_tail(&logdata,tail,sizeof(tail))==127 && tail[127]==0);
    for(unsigned i=800;i<JNET_LOG_ROWS+100;++i)assert(jnet_log_append(&logdata,"x",1));
    assert(atomic_load(&logdata.count)==JNET_LOG_ROWS+100 && atomic_load(&logdata.dropped)==100);
    assert(jnet_log_tail(&logdata,tail,sizeof(tail))==127);
    assert(jnet_log_append(&logdata,"LATEST",6));
    assert(jnet_log_tail(&logdata,tail,7)==6 && !strcmp(tail,"LATEST"));
    uint32_t blocked=atomic_load(&logdata.count);
    atomic_store(&logdata.rows[blocked%JNET_LOG_ROWS].busy,1);
    assert(jnet_log_append(&logdata,"interrupted",11));
    assert(jnet_log_append(&logdata,"AFTER",5));
    assert(jnet_log_tail(&logdata,tail,6)==5 && !strcmp(tail,"AFTER"));
    atomic_store(&logdata.rows[blocked%JNET_LOG_ROWS].busy,0);
    struct jnet_session_logs *shared=mmap(NULL,sizeof(*shared),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANON,-1,0);
    assert(shared!=MAP_FAILED);
    pid_t child=fork();assert(child>=0);
    if(!child){
        assert(jnet_log_append(&shared->runtime,"last runtime\n",13));
        assert(jnet_log_append(&shared->fault,"fault detail\n",13));
        _exit(0);
    }
    int status;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==0);
    assert(jnet_log_tail(&shared->runtime,tail,sizeof(tail))==13 && !strcmp(tail,"last runtime\n"));
    assert(jnet_log_tail(&shared->fault,tail,sizeof(tail))==13 && !strcmp(tail,"fault detail\n"));
    atomic_store(&shared->runtime.count,2); /* An interrupted writer never publishes this row. */
    assert(jnet_log_tail(&shared->runtime,tail,sizeof(tail))==13);
    assert(!munmap(shared,sizeof(*shared)));
    memset(&logdata,0,sizeof(logdata));pthread_t reader;
    assert(!pthread_create(&reader,NULL,ring_reader,NULL));
    for(uintptr_t i=0;i<4;++i)assert(!pthread_create(&threads[i],NULL,ring_writer,(void *)(i+65)));
    for(unsigned i=0;i<4;++i)assert(!pthread_join(threads[i],NULL));
    assert(!pthread_join(reader,NULL));
    assert(atomic_load(&logdata.count)==40000);
    puts("Session logs: concurrent publication, ring rollover, interrupted writer and retention after writer exit passed.");
}
