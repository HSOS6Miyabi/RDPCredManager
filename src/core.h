#pragma once
// Small platform-independent routines. These functions never access passwords.
namespace core {
inline unsigned Length(const wchar_t* text) {
    if (!text) return 0;
    unsigned count=0;
    while(text[count]) ++count;
    return count;
}
inline bool Copy(wchar_t* out, unsigned capacity, const wchar_t* text) {
    if (!out || !capacity || !text) return false;
    unsigned i=0;
    for (; text[i] && i+1<capacity; ++i) out[i]=text[i];
    out[i]=0;
    return text[i]==0;
}
inline bool Append(wchar_t* out, unsigned capacity, const wchar_t* src) {
    unsigned len=Length(out);
    return len<capacity && Copy(out+len, capacity-len, src);
}
inline bool EqualsInsensitivePrefix(const wchar_t* text, const wchar_t* prefix) {
    while (*prefix) {
        if (!*text) return false;
        wchar_t a=*text++, b=*prefix++;
        if (a>=L'A' && a<=L'Z') a+=32;
        if (b>=L'A' && b<=L'Z') b+=32;
        if (a!=b) return false;
    }
    return true;
}
inline void Trim(wchar_t* value) {
    unsigned start=0;
    while (value[start]==L' ' || value[start]==L'\t' || value[start]==L'\r' || value[start]==L'\n') ++start;
    unsigned end=Length(value);
    while(end>start && (value[end-1]==L' ' || value[end-1]==L'\t' || value[end-1]==L'\r' || value[end-1]==L'\n')) --end;
    unsigned i=0;
    for (; start<end; ++start) value[i++]=value[start];
    value[i]=0;
}
inline bool ParsePort(const wchar_t* port, unsigned* value) {
    if (!value) return false;
    if (!port || !*port) { *value=3389; return true; }
    unsigned n=0;
    for (unsigned i=0; port[i]; ++i) {
        if (port[i]<L'0' || port[i]>L'9') return false;
        n=n*10+unsigned(port[i]-L'0');
        if (n>65535) return false;
    }
    if (!n) return false;
    *value=n;
    return true;
}
inline void FormatUnsigned(unsigned n, wchar_t* buffer, unsigned cap) {
    if (!cap) return;
    wchar_t tmp[16]; unsigned count=0;
    do {tmp[count++]=wchar_t(L'0'+n%10); n/=10;} while(n && count<16);
    unsigned i=0;
    while(count && i+1<cap) buffer[i++]=tmp[--count];
    buffer[i]=0;
}
inline bool ValidateHost(const wchar_t* host) {
    unsigned n=Length(host);
    if (n==0 || n>253) return false;
    if (host[0]==L'[') {
        if(n<4 || host[n-1]!=L']') return false;
        bool colon=false;
        for(unsigned i=1;i+1<n;++i) {
            wchar_t c=host[i];
            if (c==L':') colon=true;
            else if (!((c>=L'0'&&c<=L'9') || (c>=L'a'&&c<=L'f') || (c>=L'A'&&c<=L'F') || c==L'.')) return false;
        }
        return colon;
    }
    if(host[0]==L'.'||host[0]==L'-'||host[n-1]==L'-') return false;
    for(unsigned i=0;i<n;++i) {
        wchar_t c=host[i];
        if ((c>=L'a'&&c<=L'z')||(c>=L'A'&&c<=L'Z')||
            (c>=L'0'&&c<=L'9')||c==L'.'||c==L'-'||c==L'_'||c>127) continue;
        return false;
    }
    return true;
}
inline bool BuildTarget(const wchar_t* host, wchar_t* out, unsigned cap) {
    if(!ValidateHost(host) || !Copy(out,cap,L"TERMSRV/")) return false;
    return Append(out,cap,host);
}
inline bool BuildEndpoint(const wchar_t* host, unsigned port, wchar_t* out, unsigned cap) {
    if(!ValidateHost(host) || !Copy(out,cap,host)) return false;
    if (port==3389) return true;
    wchar_t suffix[16]; FormatUnsigned(port,suffix,16);
    return Append(out,cap,L":") && Append(out,cap,suffix);
}
inline bool ReadTargetHost(const wchar_t* target,wchar_t* out,unsigned cap) {
    if(!target || !EqualsInsensitivePrefix(target,L"TERMSRV/")) return false;
    return Copy(out,cap,target+8);
}
inline bool EqualInsensitive(const wchar_t* a,const wchar_t* b) {
    return Length(a)==Length(b) && EqualsInsensitivePrefix(a,b);
}
// Parse a Windows Credential Manager TERMSRV target, optionally containing a port.
inline bool ParseTargetEndpoint(const wchar_t* target, wchar_t* host, unsigned cap,
                               unsigned* explicitPort) {
    if(!ReadTargetHost(target,host,cap)) return false;
    if(explicitPort) *explicitPort=0;
    unsigned len=Length(host);
    unsigned colon=len;
    for(unsigned i=0;i<len;++i) if(host[i]==L':') colon=i;
    if(colon<len) {
        bool suffix=false;
        if(host[0]==L'[') {
            suffix=colon>0 && host[colon-1]==L']';
        } else {
            suffix=true;
            for(unsigned i=0;i<colon;++i) if(host[i]==L':') suffix=false;
        }
        if(suffix && colon+1<len) {
            unsigned value=0;
            if(ParsePort(host+colon+1,&value)) {
                host[colon]=0;
                if(ValidateHost(host)) {
                    if(explicitPort) *explicitPort=value;
                    return true;
                }
                host[colon]=L':';
            }
        }
    }
    return ValidateHost(host);
}
inline bool PortFromComment(const wchar_t* comment, unsigned* port) {
    if(!comment || !EqualsInsensitivePrefix(comment,L"RDPCredManager.Port=")) return false;
    const wchar_t* number=comment+20;
    return *number && ParsePort(number,port);
}
} // namespace core

namespace core {
inline bool BuildTargetWithPort(const wchar_t* host, unsigned port, bool includePort,
                                wchar_t* target, unsigned capacity) {
    if (!BuildTarget(host,target,capacity)) return false;
    if (!includePort) return true;
    if (port<1 || port>65535) return false;
    wchar_t buffer[16]; FormatUnsigned(port,buffer,16);
    return Append(target,capacity,L":") && Append(target,capacity,buffer);
}
inline bool ContainsInsensitive(const wchar_t* haystack, const wchar_t* needle) {
    if (!haystack || !needle) return false;
    if (!*needle) return true;
    for (unsigned i=0; haystack[i]; ++i) {
        unsigned j=0;
        while (needle[j] && haystack[i+j]) {
            wchar_t a=haystack[i+j],b=needle[j];
            if (a>=L'A' && a<=L'Z') a+=32;
            if (b>=L'A' && b<=L'Z') b+=32;
            if (a!=b) break;
            ++j;
        }
        if (!needle[j]) return true;
    }
    return false;
}
inline bool SameTarget(const wchar_t* a,const wchar_t* b) {
    return EqualInsensitive(a,b);
}
}

// Match both the target and the Windows credential type. The same TERMSRV
// target can have both a domain-password and a generic credential.
namespace core {
template <typename Entry>
inline int FindByTargetAndType(const Entry* entries,unsigned count,
                               const wchar_t* target,unsigned type) {
    for(unsigned i=0;i<count;++i)
        if(entries[i].type==type && SameTarget(entries[i].target,target))
            return static_cast<int>(i);
    return -1;
}
}
