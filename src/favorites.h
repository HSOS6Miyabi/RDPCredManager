#pragma once
#include "core.h"

// Session bookmarks contain only references to Credential Manager entries.
namespace favorites {
constexpr unsigned Limit=128;
constexpr unsigned FormatVersion=1;
constexpr unsigned Signature=0x46504452; // 'RDPF' in little-endian storage.
struct Entry {
    wchar_t target[520];
    wchar_t alias[96];
    unsigned credentialType;
    unsigned port;
};
inline bool Valid(const Entry& item) {
    wchar_t host[300]{};
    unsigned explicitPort=0;
    return core::ParseTargetEndpoint(item.target,host,300,&explicitPort) &&
           (item.credentialType==1 || item.credentialType==2) &&
           item.port>=1 && item.port<=65535 &&
           core::Length(item.alias)<96;
}
inline int Find(const Entry* entries,unsigned count,const wchar_t* target,unsigned type) {
    for(unsigned i=0;i<count;++i)
        if(entries[i].credentialType==type && core::SameTarget(entries[i].target,target))
            return int(i);
    return -1;
}
inline bool Add(Entry* entries,unsigned* count,const wchar_t* target,unsigned type,unsigned port) {
    if(!entries || !count || !target || *count>=Limit || Find(entries,*count,target,type)>=0) return false;
    Entry item{};
    if(!core::Copy(item.target,520,target)) return false;
    item.credentialType=type;
    item.port=port;
    if(!Valid(item)) return false;
    entries[(*count)++]=item;
    return true;
}
inline bool Remove(Entry* entries,unsigned* count,unsigned index) {
    if(!entries || !count || index>=*count) return false;
    for(unsigned i=index+1;i<*count;++i) entries[i-1]=entries[i];
    entries[--(*count)]=Entry{};
    return true;
}
inline bool Update(Entry* entries,unsigned count,unsigned index,const wchar_t* alias,unsigned port) {
    if(!entries || index>=count || !alias || core::Length(alias)>=96 || !port || port>65535) return false;
    Entry candidate=entries[index];
    if(!core::Copy(candidate.alias,96,alias)) return false;
    candidate.port=port;
    if(!Valid(candidate)) return false;
    entries[index]=candidate;
    return true;
}
inline const wchar_t* Name(const Entry& e) {
    return e.alias[0] ? e.alias : e.target+8;
}
}
