#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include "jnet_session_log.h"
typedef void *HANDLE;
typedef void *HWND;
typedef void *HGLOBAL;
typedef uint32_t DWORD;
typedef size_t SIZE_T;
#define JC_PATH_CAP 32768
#define MAXDWORD UINT32_MAX
#define INVALID_HANDLE_VALUE ((void *)(intptr_t)-1)
#define GENERIC_WRITE 1
#define FILE_SHARE_READ 2
#define CREATE_ALWAYS 3
#define FILE_ATTRIBUTE_NORMAL 4
#define OFN_OVERWRITEPROMPT 1
#define OFN_PATHMUSTEXIST 2
#define OFN_NOCHANGEDIR 4
#define GWLP_USERDATA 1
#define CP_UTF8 65001
#define GMEM_MOVEABLE 2
#define CF_UNICODETEXT 13
struct picker {size_t lStructSize;HWND hwndOwner;const wchar_t *lpstrFilter;wchar_t *lpstrFile;DWORD nMaxFile;const wchar_t *lpstrDefExt;DWORD Flags;};
typedef struct picker OPENFILENAMEW;
static struct jnet_memory_log helper_log,exit_log,hang_log;
static struct jnet_session_logs logs;
static struct {struct jnet_session_logs *logs;}exit_watch={&logs};
static char output[16384];static size_t used;static int selected,opens,closes,short_write;
static wchar_t status_text[128];
static HANDLE preview_process;
static wchar_t preview_text[128];
static wchar_t *displayed=L"Displayed session status",*clipboard;
static unsigned refreshed;
static unsigned picker_error;
static int GetSaveFileNameW(OPENFILENAMEW *p){assert(p->Flags&OFN_OVERWRITEPROMPT);return selected;}
static DWORD CommDlgExtendedError(void){return picker_error;}
static intptr_t GetWindowLongPtrW(HWND w,int i){(void)w;(void)i;return (intptr_t)displayed;}
typedef intptr_t LONG_PTR;
static void SetWindowLongPtrW(HWND w,int i,LONG_PTR p){(void)w;(void)i;displayed=(wchar_t *)p;}
static int WideCharToMultiByte(unsigned cp,int flags,const wchar_t *text,int length,char *out,int cap,void *a,void *b){
    (void)cp;(void)flags;(void)length;(void)a;(void)b;int n=(int)wcslen(text)+1;
    if(out){assert(cap>=n);for(int i=0;i<n;++i)out[i]=(char)text[i];}return n;
}
static HANDLE GetProcessHeap(void){return (void *)1;}
static void *HeapAlloc(HANDLE h,int flags,size_t size){(void)h;(void)flags;return malloc(size);}
static void HeapFree(HANDLE h,int flags,void *p){(void)h;(void)flags;free(p);}
static HANDLE CreateFileW(const wchar_t *path,int access,int share,void *security,int mode,int flags,void *template){
    (void)path;(void)access;(void)share;(void)security;(void)flags;(void)template;
    assert(mode==CREATE_ALWAYS);++opens;used=0;output[0]=0;return (void *)1;
}
static int WriteFile(HANDLE file,const char *text,DWORD bytes,DWORD *written,void *overlap){
    (void)file;(void)overlap;assert(used+bytes<sizeof(output));
    *written=short_write && bytes?bytes-1:bytes;memcpy(output+used,text,*written);used+=*written;output[used]=0;return 1;
}
static int FlushFileBuffers(HANDLE h){(void)h;return 1;}
static void CloseHandle(HANDLE h){(void)h;++closes;}
static void SetDlgItemTextW(HWND w,int id,const wchar_t *text){(void)w;assert(id==117);wcscpy(status_text,text);}
#include "jnet_report_under_test.h"
static int refresh_preview(void){++refreshed;wcscpy(preview_text,L"Refreshed session status");return 1;}
struct jc_connection_recorder {
    unsigned bytes;int lifecycle,error,active;
    struct jnet_memory_log log;wchar_t summary[3000];
};
static void connection_summary(struct jc_connection_recorder *r){wcscpy(r->summary,L"Recent connection log retained in memory\n");}
static int MultiByteToWideChar(unsigned cp,int flags,const char *text,int length,wchar_t *out,int cap){
    (void)cp;(void)flags;(void)length;int n=(int)strlen(text)+1;
    if(out){assert(cap>=n);for(int i=0;i<n;++i)out[i]=(wchar_t)(unsigned char)text[i];}return n;
}
static HGLOBAL GlobalAlloc(int flags,size_t n){(void)flags;return malloc(n);}
static void *GlobalLock(HGLOBAL h){return h;}
static void GlobalUnlock(HGLOBAL h){(void)h;}
static void GlobalFree(HGLOBAL h){free(h);}
static int OpenClipboard(HWND w){(void)w;return 1;}
static int EmptyClipboard(void){free(clipboard);clipboard=NULL;return 1;}
static int SetClipboardData(int format,HGLOBAL h){assert(format==CF_UNICODETEXT);clipboard=h;return 1;}
static void CloseClipboard(void){}
#include "jnet_connection_export_under_test.h"
#include "jnet_exit_report_under_test.h"
typedef struct {int left,top,right,bottom;} RECT;
static struct {unsigned dpi;} result_theme;
static RECT client_area,control_area[210];
#define TRUE 1
#define IDOK 1
#define EM_SETRECT 1
typedef intptr_t LPARAM;
static int MulDiv(int value,int scale,int divisor){return (int)(((int64_t)value*scale+divisor/2)/divisor);}
static HWND GetDlgItem(HWND window,int id){(void)window;return (HWND)(intptr_t)id;}
static void MoveWindow(HWND window,int x,int y,int width,int height,int repaint){
    (void)repaint;unsigned id=(unsigned)(uintptr_t)window;assert(id<210 && width>0 && height>0);
    control_area[id]=(RECT){x,y,x+width,y+height};
}
static void GetClientRect(HWND window,RECT *out){
    if(!window){*out=client_area;return;}
    RECT r=control_area[(unsigned)(uintptr_t)window];*out=(RECT){0,0,r.right-r.left,r.bottom-r.top};
}
static void SendMessageW(HWND window,unsigned message,uintptr_t wp,LPARAM lp){
    (void)window;(void)message;(void)wp;RECT *r=(RECT *)lp;assert(r->right>r->left && r->bottom>r->top);
}
#include "jnet_client_layout_under_test.h"
int main(void){
    atomic_store(&logs.ready,1);
    jnet_log_append(&helper_log,"helper details\n",15);
    jnet_log_append(&logs.runtime,"runtime details\n",16);
    jnet_log_append(&logs.fault,"crash details\n",14);
    save_session_report(NULL);assert(!opens && !closes);
    picker_error=1;save_session_report(NULL);assert(!opens && !wcscmp(status_text,L"Save dialog unavailable"));picker_error=0;
    selected=1;save_session_report(NULL);assert(opens==1 && closes==1);
    assert(strstr(output,"Displayed session status") && strstr(output,"helper details") && strstr(output,"runtime details") && strstr(output,"crash details"));
    assert(!wcscmp(status_text,L"Session report saved"));
    short_write=1;save_session_report(NULL);assert(opens==2 && closes==2);
    assert(!wcscmp(status_text,L"Session report save failed"));
    short_write=0;preview_process=(HANDLE)1;save_session_report(NULL);
    assert(refreshed==1 && strstr(output,"Refreshed session status"));
    static struct jc_connection_recorder r;unsigned before=(unsigned)opens;
    assert(connection_log(&r,"connection data\n",16));
    assert(connection_log(&r,"latest event\n",13));
    assert((unsigned)opens==before);
    selected=0;connection_export(NULL,&r,1);assert((unsigned)opens==before);
    picker_error=1;connection_export(NULL,&r,1);
    assert((unsigned)opens==before && !wcscmp(status_text,L"Save dialog unavailable"));picker_error=0;
    connection_export(NULL,&r,0);assert((unsigned)opens==before && clipboard);
    assert(wcsstr(clipboard,L"connection data") && wcsstr(clipboard,L"latest event"));
    assert(!wcscmp(status_text,L"Connection report copied"));
    selected=1;connection_export(NULL,&r,1);assert((unsigned)opens==before+1);
    assert(strstr(output,"connection data") && strstr(output,"latest event"));
    assert(!wcscmp(status_text,L"Connection report saved"));
    short_write=1;connection_export(NULL,&r,1);assert((unsigned)opens==before+2);
    assert(!wcscmp(status_text,L"Connection report save failed"));
    assert(opens==closes);free(clipboard);
    wchar_t exit_report[2048];
    format_exit_report(exit_report,2048,1,0xe04a4302u,L"Last live status",0);
    assert(wcsstr(exit_report,L"Collecting exit report") && wcsstr(exit_report,L"E04A4302") && wcsstr(exit_report,L"Last live status"));
    format_exit_report(exit_report,2048,1,0,L"Last live status",1);
    assert(wcsstr(exit_report,L"Exit report ready") && !wcsstr(exit_report,L"Collecting"));
    format_exit_report(exit_report,2048,0,0,L"",1);
    assert(wcsstr(exit_report,L"Exit code unavailable") && wcsstr(exit_report,L"No live status"));
    for(unsigned dpi=96;dpi<=192;dpi+=48)for(unsigned minimum=0;minimum<2;++minimum){
        result_theme.dpi=dpi;
        client_area=(RECT){0,0,MulDiv(minimum?204:228,(int)dpi,96),MulDiv(minimum?467:587,(int)dpi,96)};
        result_layout(NULL);
        assert(control_area[IDOK].bottom<control_area[100].top);
        assert(control_area[100].bottom<control_area[117].top);
        assert(control_area[117].bottom<control_area[119].top);
        assert(control_area[119].bottom<=client_area.bottom);
        assert(control_area[117].left>=0 && control_area[117].right<=client_area.right);
    }
    puts("Report export: recording, Copy and picker cancellation create no files; explicit Save includes retained records, refreshes live status and reports short writes.");
    puts("Client status: pending/completed/unknown exit reports and non-overlapping minimum/default layouts at 96/144/192 DPI passed.");
}
