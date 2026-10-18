#pragma once
// The Win32 declarations required by this application. Kept self-contained so
// the project also cross-compiles with Clang/LLD without an installed Windows SDK.
// Definitions mirror the Windows SDK ABI for x86, x64 and ARM64 Windows.

#if !defined(_M_IX86) && !defined(_M_X64) && !defined(_M_ARM64)
#error Unsupported Windows target architecture.
#endif

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int UINT;
typedef unsigned long DWORD;
typedef long LONG;
typedef int BOOL;
#if defined(_M_IX86)
typedef unsigned int ULONG_PTR;
typedef int LONG_PTR;
#else
typedef unsigned long long ULONG_PTR;
typedef long long LONG_PTR;
#endif
typedef LONG_PTR LPARAM;
typedef ULONG_PTR WPARAM;
typedef LONG_PTR LRESULT;
typedef WORD ATOM;
typedef void* HANDLE;
typedef HANDLE HWND;
typedef HANDLE HINSTANCE;
typedef HANDLE HMENU;
typedef HANDLE HICON;
typedef HANDLE HCURSOR;
typedef HANDLE HBRUSH;
typedef HANDLE HFONT;
typedef HANDLE HGDIOBJ;
typedef const wchar_t* LPCWSTR;
typedef wchar_t* LPWSTR;
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef BYTE* LPBYTE;
#define WINAPI __stdcall
#define CALLBACK __stdcall
#define API(dll,ret,name,params) extern "C" __declspec(dllimport) ret WINAPI name params

struct POINT { LONG x, y; };
struct RECT { LONG left, top, right, bottom; };
struct MSG { HWND hwnd; UINT message; WPARAM wParam; LPARAM lParam; DWORD time; POINT pt; DWORD lPrivate; };
struct MINMAXINFO { POINT ptReserved; POINT ptMaxSize; POINT ptMaxPosition; POINT ptMinTrackSize; POINT ptMaxTrackSize; };
typedef LRESULT (CALLBACK *WNDPROC)(HWND,UINT,WPARAM,LPARAM);
struct WNDCLASSEXW { UINT cbSize; UINT style; WNDPROC lpfnWndProc; int cbClsExtra; int cbWndExtra; HINSTANCE hInstance; HICON hIcon; HCURSOR hCursor; HBRUSH hbrBackground; LPCWSTR lpszMenuName; LPCWSTR lpszClassName; HICON hIconSm; };
struct CREDENTIALW { DWORD Flags; DWORD Type; LPWSTR TargetName; LPWSTR Comment; struct {DWORD dwLowDateTime; DWORD dwHighDateTime;} LastWritten; DWORD CredentialBlobSize; LPBYTE CredentialBlob; DWORD Persist; DWORD AttributeCount; void* Attributes; LPWSTR TargetAlias; LPWSTR UserName; };
static_assert(sizeof(wchar_t)==2, "Windows UTF-16 required");
#if defined(_M_IX86)
static_assert(sizeof(void*)==4 && sizeof(ULONG_PTR)==4 && sizeof(LONG_PTR)==4,
              "Windows x86 pointer ABI mismatch");
static_assert(sizeof(CREDENTIALW)==52 && sizeof(WNDCLASSEXW)==48 && sizeof(MSG)==32,
              "Windows x86 Win32 ABI mismatch");
#else
static_assert(sizeof(void*)==8 && sizeof(ULONG_PTR)==8 && sizeof(LONG_PTR)==8,
              "Windows 64-bit pointer ABI mismatch");
static_assert(sizeof(CREDENTIALW)==80 && sizeof(WNDCLASSEXW)==80 && sizeof(MSG)==48,
              "Windows x64/ARM64 Win32 ABI mismatch");
#endif

API("user32",ATOM,RegisterClassExW,(const WNDCLASSEXW*));
API("user32",HWND,CreateWindowExW,(DWORD,LPCWSTR,LPCWSTR,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,LPVOID));
API("user32",LRESULT,DefWindowProcW,(HWND,UINT,WPARAM,LPARAM));
API("user32",BOOL,ShowWindow,(HWND,int));
API("user32",BOOL,UpdateWindow,(HWND));
API("user32",BOOL,GetMessageW,(MSG*,HWND,UINT,UINT));
API("user32",BOOL,TranslateMessage,(const MSG*));
API("user32",BOOL,IsDialogMessageW,(HWND,MSG*));
API("user32",LRESULT,DispatchMessageW,(const MSG*));
API("user32",void,PostQuitMessage,(int));
API("user32",LRESULT,SendMessageW,(HWND,UINT,WPARAM,LPARAM));
API("user32",BOOL,MoveWindow,(HWND,int,int,int,int,BOOL));
API("user32",BOOL,GetClientRect,(HWND,RECT*));
API("user32",BOOL,SetWindowTextW,(HWND,LPCWSTR));
API("user32",int,GetWindowTextW,(HWND,LPWSTR,int));
API("user32",int,GetWindowTextLengthW,(HWND));
API("user32",int,MessageBoxW,(HWND,LPCWSTR,LPCWSTR,UINT));
API("user32",HCURSOR,LoadCursorW,(HINSTANCE,LPCWSTR));
API("user32",HWND,SetFocus,(HWND));
API("user32",BOOL,EnableWindow,(HWND,BOOL));
API("user32",BOOL,IsWindowEnabled,(HWND));
API("gdi32",HGDIOBJ,GetStockObject,(int));
API("advapi32",BOOL,GetUserNameW,(LPWSTR,DWORD*));
API("advapi32",BOOL,CredEnumerateW,(LPCWSTR,DWORD,DWORD*,CREDENTIALW***));
API("advapi32",BOOL,CredWriteW,(CREDENTIALW*,DWORD));
API("advapi32",BOOL,CredDeleteW,(LPCWSTR,DWORD,DWORD));
API("advapi32",void,CredFree,(LPVOID));
API("kernel32",HINSTANCE,GetModuleHandleW,(LPCWSTR));
API("kernel32",void,ExitProcess,(UINT));
API("kernel32",DWORD,GetLastError,(void));
API("kernel32",DWORD,GetSystemDirectoryW,(LPWSTR,UINT));
API("kernel32",LPWSTR,GetCommandLineW,(void));
API("kernel32",DWORD,GetCurrentProcessId,(void));
API("kernel32",DWORD,FormatMessageW,(DWORD,LPCVOID,DWORD,DWORD,LPWSTR,DWORD,void*));
API("shell32",HINSTANCE,ShellExecuteW,(HWND,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,int));

constexpr DWORD WS_OVERLAPPEDWINDOW=0x00CF0000;
constexpr DWORD WS_VISIBLE=0x10000000;
constexpr DWORD WS_CHILD=0x40000000;
constexpr DWORD WS_TABSTOP=0x00010000;
constexpr DWORD WS_BORDER=0x00800000;
constexpr DWORD WS_VSCROLL=0x00200000;
constexpr DWORD WS_CLIPCHILDREN=0x02000000;
constexpr DWORD WS_EX_CLIENTEDGE=0x00000200;
constexpr DWORD ES_AUTOHSCROLL=0x0080;
constexpr DWORD ES_PASSWORD=0x0020;
constexpr DWORD LBS_NOTIFY=0x0001;
constexpr DWORD LBS_NOINTEGRALHEIGHT=0x0100;
constexpr DWORD BS_DEFPUSHBUTTON=0x0001;
constexpr UINT WM_CREATE=0x0001;
constexpr UINT WM_DESTROY=0x0002;
constexpr UINT WM_SIZE=0x0005;
constexpr UINT WM_GETMINMAXINFO=0x0024;
constexpr UINT WM_COMMAND=0x0111;
constexpr UINT WM_SETFONT=0x0030;
constexpr UINT EM_LIMITTEXT=0x00C5;
constexpr UINT LB_ADDSTRING=0x0180;
constexpr UINT LB_RESETCONTENT=0x0184;
constexpr UINT LB_GETCURSEL=0x0188;
constexpr UINT LB_SETCURSEL=0x0186;
constexpr UINT LBN_SELCHANGE=1;
constexpr UINT LBN_DBLCLK=2;
constexpr LONG_PTR LB_ERR=-1;
constexpr DWORD CRED_TYPE_GENERIC=1;
constexpr DWORD CRED_TYPE_DOMAIN_PASSWORD=2;
constexpr DWORD CRED_PERSIST_LOCAL_MACHINE=2;
constexpr DWORD CRED_PRESERVE_CREDENTIAL_BLOB=1;
constexpr DWORD ERROR_NOT_FOUND=1168;
constexpr DWORD FORMAT_MESSAGE_FROM_SYSTEM=0x00001000;
constexpr DWORD FORMAT_MESSAGE_IGNORE_INSERTS=0x00000200;
constexpr UINT MB_OK=0x0000;
constexpr UINT MB_YESNO=0x0004;
constexpr UINT MB_ICONERROR=0x0010;
constexpr UINT MB_ICONWARNING=0x0030;
constexpr UINT MB_ICONINFORMATION=0x0040;
constexpr UINT MB_DEFBUTTON2=0x0100;
constexpr int IDYES=6;
constexpr int SW_SHOWNORMAL=1;
constexpr int SW_SHOW=5;
constexpr int COLOR_WINDOW=5;
constexpr int DEFAULT_GUI_FONT=17;

// Additional UI, Credential Manager, DPI and GDI declarations (Win64 ABI).
typedef ULONG_PTR DWORD_PTR;
typedef DWORD COLORREF;
typedef HANDLE HPEN;
struct NMHDR { HWND hwndFrom; ULONG_PTR idFrom; UINT code; };
struct NMLISTVIEW { NMHDR hdr; int iItem; int iSubItem; UINT uNewState; UINT uOldState; UINT uChanged; POINT ptAction; LPARAM lParam; };
struct LVCOLUMNW { UINT mask; int fmt; int cx; LPWSTR pszText; int cchTextMax; int iSubItem; int iImage; int iOrder; int cxMin; int cxDefault; int cxIdeal; };
struct LVITEMW { UINT mask; int iItem; int iSubItem; UINT state; UINT stateMask; LPWSTR pszText; int cchTextMax; int iImage; LPARAM lParam; int iIndent; int iGroupId; UINT cColumns; UINT* puColumns; int* piColFmt; int iGroup; };
struct INITCOMMONCONTROLSEX { DWORD dwSize; DWORD dwICC; };
struct PAINTSTRUCT { HANDLE hdc; BOOL fErase; RECT rcPaint; BOOL fRestore; BOOL fIncUpdate; BYTE rgbReserved[32]; };
struct SIZE { LONG cx; LONG cy; };
#if defined(_M_IX86)
static_assert(sizeof(NMHDR)==12 && sizeof(LVITEMW)==60 && sizeof(LVCOLUMNW)==44,
              "x86 common control ABI mismatch");
#else
static_assert(sizeof(NMHDR)==24 && sizeof(LVITEMW)==88 && sizeof(LVCOLUMNW)==56,
              "x64/ARM64 common control ABI mismatch");
#endif
API("comctl32",BOOL,InitCommonControlsEx,(const INITCOMMONCONTROLSEX*));
API("advapi32",BOOL,CredReadW,(LPCWSTR,DWORD,DWORD,CREDENTIALW**));
API("user32",BOOL,SetProcessDpiAwarenessContext,(HANDLE));
API("user32",UINT,GetDpiForWindow,(HWND));
API("user32",BOOL,SetWindowPos,(HWND,HWND,int,int,int,int,UINT));
API("user32",int,GetDlgCtrlID,(HWND));
API("user32",BOOL,InvalidateRect,(HWND,const RECT*,BOOL));
API("user32",HANDLE,BeginPaint,(HWND,PAINTSTRUCT*));
API("user32",BOOL,EndPaint,(HWND,const PAINTSTRUCT*));
API("gdi32",HFONT,CreateFontW,(int,int,int,int,int,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPCWSTR));
API("gdi32",HBRUSH,CreateSolidBrush,(COLORREF));
API("gdi32",BOOL,DeleteObject,(HANDLE));
API("gdi32",COLORREF,SetTextColor,(HANDLE,COLORREF));
API("gdi32",int,SetBkMode,(HANDLE,int));
API("user32",int,FillRect,(HANDLE,const RECT*,HBRUSH));
API("kernel32",void,GetSystemTimeAsFileTime,(void*));
API("kernel32",BOOL,FileTimeToLocalFileTime,(const void*,void*));
API("kernel32",BOOL,FileTimeToSystemTime,(const void*,void*));
constexpr UINT WM_NOTIFY=0x004E, WM_PAINT=0x000F,WM_ERASEBKGND=0x0014;
constexpr UINT WM_CTLCOLORSTATIC=0x0138,WM_CTLCOLOREDIT=0x0133,WM_DPICHANGED=0x02E0;
constexpr UINT EN_CHANGE=0x0300;
constexpr DWORD ES_READONLY=0x0800;
constexpr DWORD WS_HSCROLL=0x00100000;
constexpr DWORD WS_GROUP=0x00020000;
constexpr DWORD CBS_DROPDOWNLIST=0x0003;
constexpr UINT CB_ADDSTRING=0x0143, CB_SETCURSEL=0x014E, CB_GETCURSEL=0x0147;
constexpr UINT CBN_SELCHANGE=1;
constexpr UINT LVM_FIRST=0x1000,LVM_GETITEMCOUNT=LVM_FIRST+4,LVM_DELETEALLITEMS=LVM_FIRST+9;
constexpr UINT LVM_GETNEXTITEM=LVM_FIRST+12,LVM_SETITEMSTATE=LVM_FIRST+43;
constexpr UINT LVM_SETEXTENDEDLISTVIEWSTYLE=LVM_FIRST+54;
constexpr UINT LVM_INSERTITEMW=LVM_FIRST+77,LVM_INSERTCOLUMNW=LVM_FIRST+97;
constexpr UINT LVM_SETITEMTEXTW=LVM_FIRST+116;
constexpr UINT LVS_REPORT=0x0001,LVS_SINGLESEL=0x0004,LVS_SHOWSELALWAYS=0x0008;
constexpr UINT LVS_EX_FULLROWSELECT=0x00000020,LVS_EX_DOUBLEBUFFER=0x00010000;
constexpr UINT LVIF_TEXT=0x0001,LVIF_PARAM=0x0004;
constexpr UINT LVCF_FMT=0x0001,LVCF_WIDTH=0x0002,LVCF_TEXT=0x0004,LVCF_SUBITEM=0x0008;
constexpr UINT LVNI_SELECTED=0x0002,LVIS_SELECTED=0x0002,LVIS_FOCUSED=0x0001;
constexpr UINT LVN_FIRST=(UINT)-100,LVN_ITEMCHANGED=(UINT)-101,NM_DBLCLK=(UINT)-3;
constexpr DWORD ICC_LISTVIEW_CLASSES=0x00000001;
constexpr UINT SWP_NOZORDER=0x0004, SWP_NOACTIVATE=0x0010;
constexpr DWORD FW_NORMAL=400,FW_SEMIBOLD=600,FW_BOLD=700;
constexpr UINT MB_ICONQUESTION=0x0020, MB_YESNOCANCEL=0x0003;
constexpr int IDNO=7,IDCANCEL=2;
constexpr UINT TRANSPARENT=1;
constexpr UINT LVM_SETCOLUMNWIDTH=LVM_FIRST+30;

// Native scaling, scrolling, modern owner-drawn buttons and window sizing.
struct SCROLLINFO { UINT cbSize; UINT fMask; int nMin; int nMax; UINT nPage; int nPos; int nTrackPos; };
struct DRAWITEMSTRUCT { UINT CtlType; UINT CtlID; UINT itemID; UINT itemAction; UINT itemState; HWND hwndItem; HANDLE hDC; RECT rcItem; ULONG_PTR itemData; };
#if defined(_M_IX86)
static_assert(sizeof(SCROLLINFO)==28 && sizeof(DRAWITEMSTRUCT)==48,
              "x86 window ABI mismatch");
#else
static_assert(sizeof(SCROLLINFO)==28 && sizeof(DRAWITEMSTRUCT)==64,
              "x64/ARM64 window ABI mismatch");
#endif
API("user32",int,SetScrollInfo,(HWND,int,const SCROLLINFO*,BOOL));
API("user32",BOOL,GetScrollInfo,(HWND,int,SCROLLINFO*));
API("user32",BOOL,GetWindowRect,(HWND,RECT*));
API("user32",int,GetSystemMetrics,(int));
API("user32",BOOL,SystemParametersInfoW,(UINT,UINT,LPVOID,UINT));
API("user32",UINT,GetDpiForSystem,(void));
API("user32",int,DrawTextW,(HANDLE,LPCWSTR,int,RECT*,UINT));
API("user32",BOOL,DrawFocusRect,(HANDLE,const RECT*));
API("gdi32",HPEN,CreatePen,(int,int,COLORREF));
API("gdi32",HGDIOBJ,SelectObject,(HANDLE,HGDIOBJ));
API("gdi32",BOOL,RoundRect,(HANDLE,int,int,int,int,int,int));
API("uxtheme",LONG,SetWindowTheme,(HWND,LPCWSTR,LPCWSTR));
constexpr UINT WM_CLOSE=0x0010,WM_VSCROLL=0x0115,WM_MOUSEWHEEL=0x020A,WM_DRAWITEM=0x002B,WM_SETFOCUS=0x0007;
constexpr DWORD BS_OWNERDRAW=0x000B;
constexpr DWORD WS_EX_CONTROLPARENT=0x00010000;
constexpr int SB_VERT=1,SB_LINEUP=0,SB_LINEDOWN=1,SB_PAGEUP=2,SB_PAGEDOWN=3,SB_THUMBPOSITION=4,SB_THUMBTRACK=5,SB_TOP=6,SB_BOTTOM=7;
constexpr UINT SIF_RANGE=0x0001,SIF_PAGE=0x0002,SIF_POS=0x0004,SIF_TRACKPOS=0x0010,SIF_ALL=0x0017;
constexpr int SPI_GETWORKAREA=0x0030,SM_CXSCREEN=0,SM_CYSCREEN=1;
constexpr UINT ODT_BUTTON=4,ODS_SELECTED=0x0001,ODS_DISABLED=0x0004,ODS_FOCUS=0x0010;
constexpr UINT DT_CENTER=0x0001,DT_VCENTER=0x0004,DT_SINGLELINE=0x0020,DT_END_ELLIPSIS=0x00008000;
constexpr UINT LVM_SETBKCOLOR=LVM_FIRST+1,LVM_SETTEXTCOLOR=LVM_FIRST+36,LVM_SETTEXTBKCOLOR=LVM_FIRST+38;
constexpr UINT EM_SETSEL=0x00B1;
constexpr UINT EN_SETFOCUS=0x0100;
constexpr UINT LVM_ENSUREVISIBLE=LVM_FIRST+19;

API("shell32",UINT,ExtractIconExW,(LPCWSTR,int,HICON*,HICON*,UINT));
API("user32",BOOL,DestroyIcon,(HICON));
