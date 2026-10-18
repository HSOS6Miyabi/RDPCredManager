// Windows runner test: only stores an isolated, disposable test credential.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincred.h>
#include <cstdio>
#include <cwchar>
#include <cstring>

int main() {
    wchar_t target[160]{};
    if(swprintf_s(target, L"TERMSRV/rdpcm-ci-%lu.invalid", GetCurrentProcessId())<0) return 1;
    constexpr wchar_t username[]=L"CI\\DisposableUser";
    constexpr wchar_t password[]=L"test-credential-not-a-real-password";
    CREDENTIALW c{};
    c.Type=CRED_TYPE_GENERIC;
    c.TargetName=target;
    c.UserName=const_cast<wchar_t*>(username);
    c.CredentialBlob=reinterpret_cast<LPBYTE>(const_cast<wchar_t*>(password));
    c.CredentialBlobSize=static_cast<DWORD>((sizeof(password)-sizeof(wchar_t)));
    c.Persist=CRED_PERSIST_SESSION;
    if(!CredWriteW(&c,0)) {
        std::printf("CredWriteW failed: %lu\n", GetLastError());return 2;
    }
    CREDENTIALW* result=nullptr;
    bool matched=false;
    if(CredReadW(target,CRED_TYPE_GENERIC,0,&result)) {
        matched=result->Type==CRED_TYPE_GENERIC && result->UserName &&
                std::wcscmp(result->UserName,username)==0 &&
                result->CredentialBlobSize==c.CredentialBlobSize &&
                result->CredentialBlob &&
                std::memcmp(result->CredentialBlob,c.CredentialBlob,c.CredentialBlobSize)==0;
        CredFree(result);
    }
    DWORD total=0;
    PCREDENTIALW* records=nullptr;
    bool enumerated=false;
    if(CredEnumerateW(L"TERMSRV/*",0,&total,&records)) {
        for(DWORD i=0;i<total;++i) {
            if(records[i]->Type==CRED_TYPE_GENERIC &&
               std::wcscmp(records[i]->TargetName,target)==0) enumerated=true;
        }
        CredFree(records);
    }
    BOOL deleted=CredDeleteW(target,CRED_TYPE_GENERIC,0);
    std::printf("Credential read=%d enumerate=%d delete=%d\n",int(matched),int(enumerated),int(deleted));
    return matched && enumerated && deleted ? 0 : 3;
}
