#include "../src/core.h"
#include <cassert>
#include <cwchar>
int main() {
    unsigned p=0;
    assert(core::ParsePort(L"",&p) && p==3389);
    assert(core::ParsePort(L"1",&p) && p==1);
    assert(core::ParsePort(L"3721",&p) && p==3721);
    assert(core::ParsePort(L"65535",&p) && p==65535);
    assert(!core::ParsePort(L"0",&p));
    assert(!core::ParsePort(L"65536",&p));
    assert(!core::ParsePort(L"33 89",&p));
    assert(!core::ParsePort(L"+3389",&p));
    assert(!core::ParsePort(L"123456789999999999",&p));
    assert(core::ValidateHost(L"example.org"));
    assert(core::ValidateHost(L"my-server_01.local"));
    assert(core::ValidateHost(L"192.168.1.1"));
    assert(core::ValidateHost(L"[2001:db8::1]"));
    assert(!core::ValidateHost(L""));
    assert(!core::ValidateHost(L"bad host"));
    assert(!core::ValidateHost(L"bad/host"));
    assert(!core::ValidateHost(L"bad:3389"));
    assert(!core::ValidateHost(L"\"evil\""));
    assert(!core::ValidateHost(L"--oops-"));
    assert(!core::ValidateHost(L"[2001:db8::bad"));
    wchar_t str[300]={};
    assert(core::BuildTarget(L"example.org",str,300));
    assert(std::wcscmp(str,L"TERMSRV/example.org")==0);
    assert(core::BuildEndpoint(L"example.org",3389,str,300));
    assert(std::wcscmp(str,L"example.org")==0);
    assert(core::BuildEndpoint(L"example.org",3721,str,300));
    assert(std::wcscmp(str,L"example.org:3721")==0);
    assert(core::ReadTargetHost(L"TERMSRV/example.org",str,300));
    assert(std::wcscmp(str,L"example.org")==0);
    assert(!core::ReadTargetHost(L"TERMSR",str,300));
    unsigned explicitPort=0;
    assert(core::ParseTargetEndpoint(L"TERMSRV/host.example:3721",str,300,&explicitPort));
    assert(explicitPort==3721 && std::wcscmp(str,L"host.example")==0);
    assert(core::ParseTargetEndpoint(L"TERMSRV/[2001:db8::1]:3390",str,300,&explicitPort));
    assert(explicitPort==3390 && std::wcscmp(str,L"[2001:db8::1]")==0);
    assert(core::ParseTargetEndpoint(L"TERMSRV/[2001:db8::1]",str,300,&explicitPort));
    assert(explicitPort==0);
    assert(core::ParseTargetEndpoint(L"TERMSRV/example.org",str,300,&explicitPort));
    assert(explicitPort==0);
    assert(core::EqualInsensitive(L"Host.EXAMPLE",L"host.example"));
    assert(core::PortFromComment(L"RDPCredManager.Port=3721",&p) && p==3721);
    assert(!core::PortFromComment(L"not an app marker",&p));
    wchar_t padded[32]=L"  example.org  \t";
    core::Trim(padded);
    assert(std::wcscmp(padded,L"example.org")==0);
    assert(!core::Copy(str,3,L"long"));
    assert(core::BuildTargetWithPort(L"host.example",3721,true,str,300));
    assert(std::wcscmp(str,L"TERMSRV/host.example:3721")==0);
    assert(core::BuildTargetWithPort(L"host.example",3721,false,str,300));
    assert(std::wcscmp(str,L"TERMSRV/host.example")==0);
    assert(core::BuildTargetWithPort(L"[2001:db8::1]",3390,true,str,300));
    assert(std::wcscmp(str,L"TERMSRV/[2001:db8::1]:3390")==0);
    assert(core::ContainsInsensitive(L"TERMSRV/Host.Example:3721",L"host.example"));
    assert(core::ContainsInsensitive(L"Administrator",L"MINIS"));
    assert(!core::ContainsInsensitive(L"Administrator",L"domain"));
    assert(core::SameTarget(L"TERMSRV/HOST",L"termsrv/host"));
    struct Example { const wchar_t* target; unsigned type; };
    const Example examples[]={
        {L"TERMSRV/host.example",2},
        {L"TERMSRV/host.example",1},
        {L"TERMSRV/another.example",1}
    };
    assert(core::FindByTargetAndType(examples,3,L"termsrv/HOST.example",1)==1);
    assert(core::FindByTargetAndType(examples,3,L"TERMSRV/host.example",2)==0);
    assert(core::FindByTargetAndType(examples,3,L"TERMSRV/missing",1)==-1);
    return 0;
}
