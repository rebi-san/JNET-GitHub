#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <CommonCrypto/CommonHMAC.h>
#include "companion_actor_packet.h"
#include "jnet_menu_model.h"
#include "jnet_lobby_bridge.h"
typedef int SOCKET;typedef unsigned long ULONG;typedef unsigned char *PUCHAR;
typedef unsigned long u_long;typedef unsigned short u_short;typedef int BOOL;
typedef struct {uint64_t value;} FILETIME;
#define TRUE 1
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define WSAEMSGSIZE 90
#define SO_EXCLUSIVEADDRUSE 0
#define FIONBIO 0
#define BCRYPT_SHA256_ALGORITHM NULL
#define BCRYPT_ALG_HANDLE_HMAC_FLAG 1
#define BCRYPT_USE_SYSTEM_PREFERRED_RNG 2
#define AcquireSRWLockExclusive(x) ((void)0)
#define ReleaseSRWLockExclusive(x) ((void)0)
#define SecureZeroMemory(p,n) memset(p,0,n)
struct crypto {unsigned char key[32];};
struct jc_network_peer {struct {unsigned admitted;uint64_t peer,last_seen;} connection;struct crypto authentication;};
static struct {uint64_t identity,epoch;unsigned paused;struct jc_network_peer peers[2];} traveler_network;
static unsigned current,traveler_local_state,traveler_actor,traveler_actor_trial_join_pending,traveler_peer_restart_requested;
typedef int LONG;
static int JNETMenu[19],JNETNetwork[24],JNETTraveler[1]={3};
static uint64_t ticks=1000,serial=100;
static unsigned char jc_game_hash[16]={15,9};
static int fail_connect;
static uint64_t GetTickCount64(void){return ticks;}
static void GetSystemTimeAsFileTime(FILETIME *ft){ft->value=++serial;}
static uintptr_t GetModuleHandleW(void *p){(void)p;return 0;}
static int traveler_network_start(void){return 0;}
static int traveler_local_network_connect(void){traveler_network.paused=0;return fail_connect;}
static void traveler_local_stop(void){traveler_local_state=4;}
static void traveler_actor_trial_stop(int n){(void)n;}
static int traveler_peer_vacant(void){return 1;}
static unsigned chapter=2;
static int capture(void *read,void *ctx,uintptr_t image,uintptr_t game,struct jc_actor_sender *sample){
 (void)read;(void)ctx;(void)image;(void)game;memset(sample,0,sizeof(*sample));sample->chapter=chapter;return 0;
}
#define jc_actor_capture_sender capture
#define own_read NULL
static int traveler_udp_mac(void *ctx,const unsigned char *data,size_t size,unsigned char tag[32]){
 CCHmac(kCCHmacAlgSHA256,((struct crypto*)ctx)->key,32,data,size,tag);return 1;
}
typedef void *BCRYPT_ALG_HANDLE;
struct hash {CCHmacContext ctx;};typedef struct hash *BCRYPT_HASH_HANDLE;
static int BCryptOpenAlgorithmProvider(BCRYPT_ALG_HANDLE *h,void *a,void *b,unsigned flags){(void)a;(void)b;(void)flags;*h=(void*)1;return 0;}
static int BCryptCreateHash(BCRYPT_ALG_HANDLE a,BCRYPT_HASH_HANDLE *h,void *b,unsigned c,PUCHAR key,ULONG length,unsigned flags){
 (void)a;(void)b;(void)c;(void)flags;*h=malloc(sizeof(**h));assert(*h);CCHmacInit(&(*h)->ctx,kCCHmacAlgSHA256,key,length);return 0;
}
static int BCryptHashData(BCRYPT_HASH_HANDLE h,PUCHAR data,ULONG size,unsigned f){(void)f;CCHmacUpdate(&h->ctx,data,size);return 0;}
static int BCryptFinishHash(BCRYPT_HASH_HANDLE h,PUCHAR data,ULONG size,unsigned f){(void)f;assert(size==32);CCHmacFinal(&h->ctx,data);return 0;}
static void BCryptDestroyHash(BCRYPT_HASH_HANDLE h){free(h);}
static void BCryptCloseAlgorithmProvider(BCRYPT_ALG_HANDLE h,unsigned f){(void)h;(void)f;}
static int BCryptGenRandom(void *a,PUCHAR p,unsigned n,unsigned f){(void)a;(void)f;assert(n==8);uint64_t value=++serial;memcpy(p,&value,8);return 0;}
struct packet {unsigned char data[160];unsigned from;};
static struct {struct packet packets[256];unsigned head,tail;} queues[3];
static int fake_send(int s,const char *p,int bytes,int flags,const struct sockaddr *to,int size){
 (void)s;(void)flags;(void)size;unsigned target=ntohs(((const struct sockaddr_in*)to)->sin_port)-43941;
 assert(target<3 && bytes==160);unsigned index=queues[target].tail++%256;
 memcpy(queues[target].packets[index].data,p,160);queues[target].packets[index].from=current;return bytes;
}
static int fake_recv(int s,char *p,int cap,int flags,struct sockaddr *from,int *size){
 (void)s;(void)cap;(void)flags;(void)size;if(queues[current].head==queues[current].tail)return -1;
 struct packet *packet=&queues[current].packets[queues[current].head++%256];memcpy(p,packet->data,160);
 struct sockaddr_in *address=(void*)from;memset(address,0,sizeof(*address));address->sin_family=AF_INET;
 address->sin_addr.s_addr=htonl(INADDR_LOOPBACK);address->sin_port=htons(43941+packet->from);return 160;
}
static int WSAGetLastError(void){return 1;}
#define sendto fake_send
#define recvfrom fake_recv
#define socket(a,b,c) (1)
#define setsockopt(a,b,c,d,e) ((void)(d),0)
#define ioctlsocket(a,b,c) ((void)(c),0)
#define bind(a,b,c) ((void)(b),0)
#define closesocket(a) ((void)(a))
static unsigned traveler_network_external;
#include "jnet_lobby_runtime.h"
static __typeof__(jnet_room) rooms[3];
static __typeof__(traveler_network) networks[3];
static struct jnet_lobby_view views[3];
static void enter(unsigned id){current=id;jnet_room=rooms[id];traveler_network=networks[id];jnet_lobby_view=views[id];}
static void leave(void){rooms[current]=jnet_room;networks[current]=traveler_network;views[current]=jnet_lobby_view;}
static void rounds(unsigned n){for(unsigned t=0;t<n;++t){ticks+=100;for(unsigned i=0;i<3;++i){enter(i);jnet_lobby_tick(1);leave();}}}
static void action(unsigned id,unsigned kind,unsigned access,const char *password,uint64_t host,uint64_t room){
 enter(id);struct jnet_lobby_action a={.kind=kind,.access=access,.host=host,.room=room};
 snprintf(a.password,sizeof(a.password),"%s",password);assert(jnet_lobby_submit(&a));jnet_lobby_tick(1);leave();
}
int main(void){
 for(unsigned i=0;i<3;++i){rooms[i].socket=-1;networks[i].identity=i+1;networks[i].epoch=100+i;
  for(unsigned j=0,s=0;j<3;++j)if(i!=j){networks[i].peers[s].connection.peer=j+1;networks[i].peers[s].authentication.key[0]=(unsigned char)(i+j+1);++s;}}
 rounds(6);assert(views[0].ready && views[1].ready);
 enter(0);struct jnet_lobby_action rocket={.kind=1,.mode=JNET_ROCKET_DEATH_MATCH};
 assert(jnet_lobby_submit(&rocket));jnet_lobby_tick(1);assert(!jnet_room.room);leave();
 action(0,1,JNET_PUBLIC,"",0,0);rounds(6);assert(rooms[0].room && views[1].count==1);
 uint64_t room=rooms[0].room;action(1,2,0,"",1,room);action(2,2,0,"",1,room);rounds(12);
 assert(rooms[1].room==room && rooms[2].room==room && rooms[0].mask==14);
 enter(1);assert(jnet_lobby_allows(1)&&jnet_lobby_allows(3));leave();
 action(1,3,0,"",0,0);rounds(8);assert(!rooms[1].room && !(rooms[0].mask&4) && !(rooms[2].mask&4));
 action(1,2,0,"",1,room);rounds(8);assert(rooms[1].room==room);
 action(0,3,0,"",0,0);rounds(8);assert(!rooms[1].room&&!rooms[2].room);
 action(0,1,JNET_PRIVATE,"secret",0,0);rounds(6);assert(views[1].count==0&&views[2].count==0);
 action(1,2,0,"wrong",0,0);rounds(60);assert(!rooms[1].room && rooms[0].mask==2);
 action(1,2,0,"secret",0,0);rounds(10);assert(rooms[1].room==rooms[0].room);
 enter(0);unsigned before=jnet_room.mask;struct packet forged={.from=2};memcpy(forged.data,"JNL1",4);
 memcpy(forged.data+8,jc_game_hash,16);jc_packet_put(forged.data+24,3,8);jc_packet_put(forged.data+32,1,8);
 queues[0].packets[queues[0].tail++%256]=forged;jnet_lobby_receive(ticks);assert(jnet_room.mask==before);leave();
 enter(0);uint64_t deadline=ticks+6000;ticks=deadline;jnet_lobby_tick(1);assert(jnet_room.mask==2);leave();
 chapter=3;rounds(25);assert(!rooms[0].room&&!rooms[1].room);
 puts("Actual lobby runtime: public discovery, three admissions, leave/rejoin, host departure, unlisted private room, wrong/correct password, forged packet rejection, timeout and chapter exit passed (mock transport, real HMAC).");
}
