#include "../src/favorites.h"
#include <cassert>
int main() {
    favorites::Entry rows[favorites::Limit]{};
    unsigned count=0;
    assert(favorites::Add(rows,&count,L"TERMSRV/server.example.com",1,3389));
    assert(!favorites::Add(rows,&count,L"TERMSRV/SERVER.EXAMPLE.COM",1,3389));
    assert(favorites::Add(rows,&count,L"TERMSRV/server.example.com",2,3389));
    assert(count==2);
    assert(favorites::Find(rows,count,L"termsrv/server.example.com",2)==1);
    assert(favorites::Update(rows,count,0,L"工作服务器",3721));
    assert(rows[0].port==3721);
    assert(core::EqualInsensitive(favorites::Name(rows[0]),L"工作服务器"));
    assert(!favorites::Update(rows,count,0,L"Bad",0));
    assert(rows[0].port==3721);
    assert(favorites::Remove(rows,&count,0));
    assert(count==1 && rows[0].credentialType==2);
    assert(!favorites::Remove(rows,&count,2));
    assert(!favorites::Add(rows,&count,L"NOTRDP/srv",1,3389));
    return 0;
}
