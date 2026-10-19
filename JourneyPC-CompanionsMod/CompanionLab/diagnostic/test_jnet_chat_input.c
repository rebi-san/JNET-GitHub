#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
#include <assert.h>
typedef unsigned SRWLOCK;
typedef unsigned UINT;
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;
#define SRWLOCK_INIT 0
#define WM_KEYDOWN 0x100
#define WM_KEYUP 0x101
#define WM_CHAR 0x102
#define WM_KILLFOCUS 8
#define VK_ESCAPE 27
static void AcquireSRWLockExclusive(SRWLOCK *lock){(void)lock;}
static void ReleaseSRWLockExclusive(SRWLOCK *lock){(void)lock;}
static uint64_t now=1000;
static uint64_t GetTickCount64(void){return now;}
#define JNET_CHAT_FIXTURE 1
#define JNET_CHAT_INPUT_FIXTURE 1
#include "jnet_chat.h"
int main(void) {
    assert(!jnet_chat_allowed && !jnet_chat_focus);
    assert(!jnet_chat_window(WM_KEYDOWN,'T',0));
    jnet_chat_allowed=1;assert(!jnet_chat_window(WM_KEYDOWN,'T',0));
    jnet_chat_gameplay=1;jnet_chat.room=42;
    assert(jnet_chat_window(WM_KEYDOWN,'T',0) && jnet_chat_focus);
    assert(jnet_chat_window(WM_CHAR,'t',0) && !jnet_chat.input[0]);
    assert(jnet_chat_window(WM_CHAR,'H',0));assert(jnet_chat_window(WM_CHAR,'i',0));
    assert(jnet_chat_window(WM_CHAR,8,0) && !strcmp(jnet_chat.input,"H"));
    assert(jnet_chat_window(WM_CHAR,'i',0));assert(jnet_chat_window(WM_CHAR,13,0));
    assert(!jnet_chat_focus && !strcmp(jnet_chat.pending,"Hi"));jnet_chat.pending[0]=0;
    jnet_chat_window(WM_KEYDOWN,'T',0);jnet_chat_window(WM_CHAR,'T',0);jnet_chat_window(WM_CHAR,'x',0);
    assert(jnet_chat_window(WM_KEYDOWN,VK_ESCAPE,0) && !jnet_chat_focus && !jnet_chat.pending[0]);
    jnet_chat_window(WM_KEYDOWN,'T',0);jnet_chat_window(WM_CHAR,'t',0);
    for(unsigned i=0;i<200;++i)jnet_chat_window(WM_CHAR,'a',0);
    assert(strlen(jnet_chat.input)==120);jnet_chat.room=43;jnet_chat_window(WM_CHAR,13,0);
    assert(!jnet_chat.pending[0]); /* Input cannot cross a lobby boundary. */
    jnet_chat_window(WM_KEYDOWN,'T',0);jnet_chat_window(WM_KILLFOCUS,0,0);assert(!jnet_chat_focus);
    jnet_chat_line("A connected");assert(strstr(jnet_chat.lines[4],"connected"));
    now+=10400;jnet_chat_expire(now);assert(!jnet_chat.shown && !jnet_chat.lines[4][0]);
    jnet_chat_line("new message");assert(!jnet_chat.lines[3][0]);
    now+=10400;jnet_chat_window(WM_KEYDOWN,'T',0);
    assert(jnet_chat_focus && !jnet_chat.shown && !jnet_chat.lines[4][0]);
    puts("Chat input: disabled/menu gates, opening T suppression, Enter, Backspace, Escape, focus loss, length bound and room-change refusal passed in isolation.");
}
