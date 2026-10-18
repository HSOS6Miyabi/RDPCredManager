// Run native Windows GUI integration tests in the application's own process.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
int wmain() {
    wchar_t exe[MAX_PATH]{};
    if(!GetFullPathNameW(L"RDPCredManager.exe",MAX_PATH,exe,nullptr)) return 1;
    wchar_t command[MAX_PATH+32]{};
    if(swprintf_s(command,L"\"%ls\" --self-test",exe)<0) return 2;
    STARTUPINFOW si{};si.cb=sizeof(si);
    PROCESS_INFORMATION pi{};
    if(!CreateProcessW(exe,command,nullptr,nullptr,FALSE,0,nullptr,nullptr,&si,&pi)) {
        std::printf("GUI process startup failed: %lu\n",GetLastError());return 3;
    }
    DWORD wait=WaitForSingleObject(pi.hProcess,15000);
    DWORD code=999;
    if(wait==WAIT_TIMEOUT) TerminateProcess(pi.hProcess,99);
    else GetExitCodeProcess(pi.hProcess,&code);
    CloseHandle(pi.hThread);CloseHandle(pi.hProcess);
    if(wait!=WAIT_OBJECT_0 || code!=0) {
        std::printf("Native GUI integration failed: wait=%lu code=%lu\n",wait,code);return 4;
    }
    std::puts("Native GUI save, port editing, selection and cleanup: passed");
    return 0;
}
