// End-to-end Windows GUI test. Only creates isolated, disposable credentials.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincred.h>
#include <cstdio>
#include <cwchar>

namespace {
constexpr int ID_HOST=100,ID_PORT=101,ID_USER=102,ID_PASS=103,ID_MODE=104;
constexpr int ID_SAVE=122,ID_LIST=107;
bool PutText(HWND h,const wchar_t* text) {
    SendMessageW(h,WM_SETTEXT,0,reinterpret_cast<LPARAM>(text));
    wchar_t buffer[520]{};
    SendMessageW(h,WM_GETTEXT,520,reinterpret_cast<LPARAM>(buffer));
    return std::wcscmp(text,buffer)==0;
}
void ReadText(HWND h,wchar_t* out,int len) {
    SendMessageW(h,WM_GETTEXT,static_cast<WPARAM>(len),reinterpret_cast<LPARAM>(out));
}
void Error(const char* msg) { std::fprintf(stderr,"GUI test: %s (Win32 %lu)\n",msg,GetLastError()); }
bool WaitForCredential(const wchar_t* target) {
    for(int tries=0;tries<40;++tries) {
        CREDENTIALW* c=nullptr;
        if(CredReadW(target,CRED_TYPE_GENERIC,0,&c)) {
            bool good=c->Type==CRED_TYPE_GENERIC && c->UserName &&
                 std::wcscmp(c->UserName,L"CI\\DisposableUser")==0;
            CredFree(c);
            if(good) return true;
        }
        Sleep(100);
    }
    return false;
}
}
int wmain() {
    wchar_t app[MAX_PATH]{};
    GetFullPathNameW(L"RDPCredManager.exe",MAX_PATH,app,nullptr);
    STARTUPINFOW si{};si.cb=sizeof(si);
    PROCESS_INFORMATION pi{};
    if(!CreateProcessW(app,nullptr,nullptr,nullptr,FALSE,0,nullptr,nullptr,&si,&pi)) {
        Error("CreateProcess failed");return 1;
    }
    wchar_t hostname[120]{},oldTarget[200]{},newTarget[200]{};
    swprintf_s(hostname,L"rdpcm-gui-ci-%lu.invalid",pi.dwProcessId);
    swprintf_s(oldTarget,L"TERMSRV/%s:3721",hostname);
    swprintf_s(newTarget,L"TERMSRV/%s",hostname);
    int result=2;
    HWND window=nullptr;
    for(int i=0;i<120;++i) {
        window=FindWindowW(L"RDPCredManagerPro",nullptr);
        if(window && GetDlgItem(window,ID_SAVE)) break;
        if(WaitForSingleObject(pi.hProcess,0)==WAIT_OBJECT_0) break;
        Sleep(100);
    }
    if(!window || !GetDlgItem(window,ID_SAVE)) {Error("Main window did not open");goto done;}
    {
        HWND host=GetDlgItem(window,ID_HOST),port=GetDlgItem(window,ID_PORT);
        HWND user=GetDlgItem(window,ID_USER),password=GetDlgItem(window,ID_PASS);
        HWND mode=GetDlgItem(window,ID_MODE),save=GetDlgItem(window,ID_SAVE);
        wchar_t value[120]{};
        ReadText(host,value,120);
        if(value[0]) {Error("Host must be empty on startup");goto done;}
        ReadText(port,value,120);
        if(value[0]) {Error("Port must be empty on startup");goto done;}
        if(!IsWindowEnabled(port) || !IsWindowEnabled(save)) {
            Error("Port and Save must be enabled");goto done;
        }
        if(!PutText(host,hostname) || !PutText(port,L"3721") ||
           !PutText(user,L"CI\\DisposableUser") ||
           !PutText(password,L"disposable-ci-test-password")) {
            Error("Failed to populate remote-process input controls");goto done;
        }
        SendMessageW(mode,CB_SETCURSEL,1,0);
        // WM_COMMAND for selected target name rule (same event sent by user choice).
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(ID_MODE,CBN_SELCHANGE),reinterpret_cast<LPARAM>(mode));
        // Queue a click so unexpected modal dialogs cannot block the test process.
        PostMessageW(window,WM_COMMAND,MAKEWPARAM(ID_SAVE,BN_CLICKED),reinterpret_cast<LPARAM>(save));
        if(!WaitForCredential(oldTarget)) {Error("Save did not persist host:port credential");goto done;}
        // After Save the GUI should display the newly selected GENERIC credential.
        wchar_t caption[130]{};
        ReadText(GetDlgItem(window,14),caption,130);
        if(!std::wcsstr(caption,L"通用凭据")) {
            Error("Editor did not select generic credential after saving");goto done;
        }
        // Change to default port, retaining the existing password and adding a new target.
        if(!PutText(port,L"3389")) {Error("Failed to edit port");goto done;}
        SendMessageW(mode,CB_SETCURSEL,0,0);
        SendMessageW(window,WM_COMMAND,MAKEWPARAM(ID_MODE,CBN_SELCHANGE),reinterpret_cast<LPARAM>(mode));
        PostMessageW(window,WM_COMMAND,MAKEWPARAM(ID_SAVE,BN_CLICKED),reinterpret_cast<LPARAM>(save));
        if(!WaitForCredential(newTarget)) {Error("Edit did not create default-port credential");goto done;}
        if(!WaitForCredential(oldTarget)) {Error("Editing deleted the original credential");goto done;}
        result=0;
    }
done:
    CredDeleteW(oldTarget,CRED_TYPE_GENERIC,0);
    CredDeleteW(newTarget,CRED_TYPE_GENERIC,0);
    if(window && IsWindow(window)) PostMessageW(window,WM_CLOSE,0,0);
    if(WaitForSingleObject(pi.hProcess,2500)==WAIT_TIMEOUT) TerminateProcess(pi.hProcess,99);
    CloseHandle(pi.hThread);CloseHandle(pi.hProcess);
    if(result==0) std::puts("Windows GUI startup, port editing, save and reselection: passed");
    return result;
}
