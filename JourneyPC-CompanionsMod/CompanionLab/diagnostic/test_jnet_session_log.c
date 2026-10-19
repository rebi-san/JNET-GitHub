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
int main(void){
    pthread_t threads[4];char tail[128];
    for(uintptr_t i=0;i<4;++i)assert(!pthread_create(&threads[i],NULL,writer,(void *)i));
    for(unsigned i=0;i<4;++i)assert(!pthread_join(threads[i],NULL));
    assert(atomic_load(&logdata.count)==800);
    for(unsigned i=0;i<800;++i)assert(atomic_load(&logdata.rows[i].length)>0);
    assert(jnet_log_tail(&logdata,tail,sizeof(tail))==127 && tail[127]==0);
    for(unsigned i=800;i<JNET_LOG_ROWS+100;++i)assert(jnet_log_append(&logdata,"x",1));
    assert(atomic_load(&logdata.count)==JNET_LOG_ROWS && atomic_load(&logdata.dropped)==100);
    assert(jnet_log_tail(&logdata,tail,sizeof(tail))==127);
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
    puts("Session logs: concurrent publication, bounded capacity, interrupted writer and retention after writer exit passed.");
}
