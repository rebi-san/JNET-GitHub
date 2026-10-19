#include <assert.h>
#include <stdio.h>
#include "jnet_group_travel.h"
int main(void) {
    struct jnet_group_travel t={0};uint64_t members[]={100001,200002,300003,400004};
    assert(!jnet_travel_begin(&t,1,2,3,members[0],members,4,1,10,100));
    assert(jnet_travel_begin(&t,1,2,3,members[0],members,3,1,10,100));
    assert(!jnet_travel_answer(&t,1,1,3,members[1],JNET_TRAVEL_YES,20));
    assert(!jnet_travel_answer(&t,1,2,3,400004,JNET_TRAVEL_YES,20));
    assert(jnet_travel_answer(&t,1,2,3,members[1],JNET_TRAVEL_YES,20));
    assert(jnet_travel_answer(&t,1,2,3,members[1],JNET_TRAVEL_YES,20));
    assert(!jnet_travel_answer(&t,1,2,3,members[1],JNET_TRAVEL_NO,20));
    assert(!jnet_travel_close_vote(&t,20,100));
    assert(jnet_travel_answer(&t,1,2,3,members[2],JNET_TRAVEL_NO,21));
    assert(jnet_travel_close_vote(&t,22,100));
    assert(jnet_travel_participants(&t)==3);
    assert(!jnet_travel_commit(&t,members[0],23,100));
    assert(!jnet_travel_drained(&t,1,2,3,members[2],23));
    assert(jnet_travel_drained(&t,1,2,3,members[0],23));
    assert(jnet_travel_drained(&t,1,2,3,members[1],23));
    assert(!jnet_travel_commit(&t,members[1],24,100));
    assert(jnet_travel_commit(&t,members[0],24,100));
    assert(!jnet_travel_loaded(&t,1,2,3,members[2],1,30));
    assert(!jnet_travel_loaded(&t,1,2,3,members[1],2,30));
    assert(jnet_travel_loaded(&t,1,2,3,members[0],1,30));
    assert(jnet_travel_loaded(&t,1,2,3,members[1],1,30));
    assert(t.phase==JNET_TRAVEL_DONE);
    assert(!jnet_travel_begin(&t,1,2,3,members[0],members,3,1,40,100));
    assert(jnet_travel_begin(&t,1,2,4,members[0],members,3,1,40,100));
    assert(!jnet_travel_answer(&t,1,2,3,members[1],JNET_TRAVEL_YES,50));
    assert(jnet_travel_close_vote(&t,140,100));
    assert(t.replies[1]==JNET_TRAVEL_TIMEOUT && t.replies[2]==JNET_TRAVEL_TIMEOUT);
    assert(jnet_travel_participants(&t)==1);
    jnet_travel_expire(&t,240,1);assert(t.phase==JNET_TRAVEL_CANCELLED);
    assert(jnet_travel_begin(&t,1,2,5,members[0],members,3,1,250,100));
    jnet_travel_expire(&t,251,0);assert(t.phase==JNET_TRAVEL_CANCELLED);
    puts("Group travel: consent, capacity, authenticated sender, stale generation, cleanup barrier, level acknowledgement, timeout and host departure passed; game adapter not exercised.");
}
