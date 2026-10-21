#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include "jnet_session_log.h"
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
    pthread_t threads[4];char tail[128];
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
