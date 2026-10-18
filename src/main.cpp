#include "win32_min.h"
#include "core.h"
#include "layout.h"
#include "favorites.h"

// RDP Credential Manager — native Windows x86/x64/ARM64 GUI, no third-party runtime.
namespace {
constexpr unsigned MAX_ENTRIES=2048;
constexpr int ID_TITLE=10,ID_SUBTITLE=11,ID_COUNT=12,ID_SECTION=13,ID_FORMTITLE=14;
constexpr int ID_HOSTLABEL=15,ID_PORTLABEL=16,ID_USERLABEL=17,ID_PASSLABEL=18;
constexpr int ID_TARGETLABEL=19,ID_PREVIEW=20,ID_HELP=21,ID_STATUS=22,ID_SEARCHLABEL=23;
constexpr int ID_HOST=100,ID_PORT=101,ID_USER=102,ID_PASS=103,ID_MODE=104;
constexpr int ID_SEARCH=105,ID_FILTER=106,ID_LIST=107;
constexpr int ID_NEW=120,ID_REFRESH=121,ID_SAVE=122,ID_CONVERT=123;
constexpr int ID_CLEANUP=124,ID_CONNECT=125,ID_DELETE=126,ID_CANCEL=127;
constexpr int ID_TAB_FAVORITES=130,ID_TAB_CREDENTIALS=131,ID_TOGGLE_FAVORITE=132;
constexpr int ID_FAVORITES_LIST=140,ID_FAVORITES_SEARCH=141,ID_FAVORITES_NAME=142,ID_FAVORITES_PORT=143;
constexpr int ID_FAVORITES_CONNECT=144,ID_FAVORITES_REMOVE=145,ID_FAVORITES_SAVE=146,ID_FAVORITES_EDIT=147;
constexpr int ID_FAVORITES_HEADING=148,ID_FAVORITES_EDITHEADING=149,ID_FAVORITES_NAMELABEL=150;
constexpr int ID_FAVORITES_PORTLABEL=151,ID_FAVORITES_INFO=152,ID_FAVORITES_HINT=153;
constexpr DWORD UI_BG=0x00FAF7F3, UI_WHITE=0x00FFFFFF, UI_NAVY=0x003B2914;
constexpr DWORD UI_ACCENT=0x0088730B, UI_SOFT=0x00F8F2E9,UI_BORDER=0x00E9E3DA;
constexpr DWORD UI_MUTED=0x00877969, UI_TITLE=0x00463420,UI_RED=0x003B3BC0;

struct Entry {
    wchar_t target[520];
    wchar_t user[520];
    wchar_t comment[260];
    DWORD type;
};
Entry items[MAX_ENTRIES]{};
unsigned count=0,shown=0;
unsigned visible[MAX_ENTRIES]{};
int selected=-1;
struct BookmarkStore {
    DWORD signature,version,size;
    favorites::Entry entries[favorites::Limit];
};
static_assert(sizeof(favorites::Entry)==1240,"Bookmark format must match all Windows architectures");
BookmarkStore bookmarks{};
BookmarkStore undoBookmarks{};
unsigned favoriteCount=0, favoriteShown=0;
unsigned favoriteVisible[favorites::Limit]{};
int selectedFavorite=-1;
bool favoriteStoreWritable=true;
bool favoritesTab=false;
wchar_t currentUser[260]{};
wchar_t pendingOld[520]{},pendingNew[520]{},pendingUser[520]{};
bool conversionPending=false;
HWND frame=nullptr;
HWND title=nullptr,subtitle=nullptr,counts=nullptr,section=nullptr,formTitle=nullptr;
HWND hostLabel=nullptr,portLabel=nullptr,userLabel=nullptr,passLabel=nullptr;
HWND targetLabel=nullptr,preview=nullptr,help=nullptr,status=nullptr,searchLabel=nullptr;
HWND hostBox=nullptr,portBox=nullptr,userBox=nullptr,passBox=nullptr,modeBox=nullptr;
HWND searchBox=nullptr,filterBox=nullptr,listView=nullptr;
HWND newBtn=nullptr,refreshBtn=nullptr,saveBtn=nullptr,convertBtn=nullptr;
HWND cleanupBtn=nullptr,cancelBtn=nullptr,connectBtn=nullptr,deleteBtn=nullptr;
HWND favoritesTabBtn=nullptr,credentialsTabBtn=nullptr,toggleFavoriteBtn=nullptr;
HWND favoritesList=nullptr,favoritesSearch=nullptr,favoritesName=nullptr,favoritesPort=nullptr;
HWND favoritesConnect=nullptr,favoritesRemove=nullptr,favoritesSave=nullptr,favoritesEdit=nullptr;
HWND favoritesHeading=nullptr,favoritesEditHeading=nullptr,favoritesNameLabel=nullptr;
HWND favoritesPortLabel=nullptr,favoritesInfo=nullptr,favoritesHint=nullptr;
HBRUSH whiteBrush=nullptr,navyBrush=nullptr,bgBrush=nullptr,softBrush=nullptr;
HFONT regular=nullptr,strong=nullptr,large=nullptr,small=nullptr;
HINSTANCE application=nullptr;
HICON applicationIcon=nullptr,smallIcon=nullptr;
int dpi=96,scrollY=0;
layout::Geometry geometry{};
bool uiUpdating=false;
bool headlessSelfTest=false;
void RefreshFavorites(bool preserveSelection=true);
void UpdateFavoriteActions();
void ShowCurrentTab();
void SelectFavorite(int index);
void ChangeTab(bool favorite);

int S(int v) {return (v*dpi+48)/96;}
void Wipe(wchar_t* data,unsigned size) {volatile wchar_t* p=data;while(size--) *p++=0;}
void SetText(HWND h,const wchar_t* text) {if(h) SetWindowTextW(h,text);}
void Notice(const wchar_t* text) {if(headlessSelfTest) return; MessageBoxW(frame,text,L"RDP 凭据管理器",MB_ICONINFORMATION|MB_OK);}
void Alert(const wchar_t* text) {if(headlessSelfTest) return; MessageBoxW(frame,text,L"RDP 凭据管理器",MB_ICONWARNING|MB_OK);}
void Status(const wchar_t* text) {SetText(status,text);}
void Failure(const wchar_t* titleText,DWORD code) {
    if(headlessSelfTest) return;
    wchar_t buffer[880]={},num[20]={},details[440]={};
    core::Copy(buffer,880,titleText);
    core::Append(buffer,880,L"\r\n\r\nWindows 错误代码：");
    core::FormatUnsigned(code,num,20);core::Append(buffer,880,num);
    if(FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                      nullptr,code,0,details,440,nullptr)) {
        core::Append(buffer,880,L"\r\n");core::Append(buffer,880,details);
    }
    MessageBoxW(frame,buffer,L"操作失败",MB_ICONERROR|MB_OK);
}
HWND Control(const wchar_t* classname,const wchar_t* label,DWORD flags,DWORD ex,int id,HFONT face) {
    HWND h=CreateWindowExW(ex,classname,label,WS_CHILD|WS_VISIBLE|flags,
        0,0,S(40),S(24),frame,(HMENU)(ULONG_PTR)id,application,nullptr);
    if(h) SendMessageW(h,WM_SETFONT,(WPARAM)face,1);
    return h;
}
void TextFont(HWND h,HFONT f) {if(h) SendMessageW(h,WM_SETFONT,(WPARAM)f,1);}
void Fonts() {
    HFONT oldRegular=regular,oldStrong=strong,oldLarge=large,oldSmall=small;
    regular=CreateFontW(-S(15),0,0,0,FW_NORMAL,0,0,0,1,0,0,0,0,L"Segoe UI");
    strong=CreateFontW(-S(16),0,0,0,FW_SEMIBOLD,0,0,0,1,0,0,0,0,L"Segoe UI");
    large=CreateFontW(-S(28),0,0,0,FW_BOLD,0,0,0,1,0,0,0,0,L"Segoe UI");
    small=CreateFontW(-S(13),0,0,0,FW_NORMAL,0,0,0,1,0,0,0,0,L"Segoe UI");
    if(title) {
        HWND regularItems[]={subtitle,hostLabel,portLabel,userLabel,passLabel,targetLabel,
          searchLabel,hostBox,portBox,userBox,passBox,searchBox,modeBox,filterBox,listView,
          refreshBtn,convertBtn,cleanupBtn,cancelBtn,deleteBtn,
          favoritesSearch,favoritesList,favoritesName,favoritesPort,favoritesRemove,
          favoritesEdit,favoritesPortLabel,favoritesNameLabel,favoritesInfo,favoritesHint};
        for(HWND item:regularItems) TextFont(item,regular);
        HWND strongItems[]={counts,section,formTitle,newBtn,saveBtn,connectBtn,
            favoritesTabBtn,credentialsTabBtn,toggleFavoriteBtn,favoritesHeading,
            favoritesEditHeading,favoritesConnect,favoritesSave};
        for(HWND item:strongItems) TextFont(item,strong);
        HWND smallItems[]={preview,help,status};
        for(HWND item:smallItems) TextFont(item,small);
        TextFont(title,large);
    }
    if(oldRegular) DeleteObject(oldRegular);
    if(oldStrong) DeleteObject(oldStrong);
    if(oldLarge) DeleteObject(oldLarge);
    if(oldSmall) DeleteObject(oldSmall);
}
void Pos(HWND h,layout::Rectangle r) {
    if(h) MoveWindow(h,S(r.x),S(r.y-scrollY),S(r.w>1?r.w:1),S(r.h>1?r.h:1),1);
}
void Layout() {
    if(!frame || !title) return;
    RECT rc{};GetClientRect(frame,&rc);
    int w=(int)(rc.right*96/dpi), h=(int)(rc.bottom*96/dpi);
    if(w<540) w=540;
    geometry=layout::Calculate(w,h);
    scrollY=layout::ClampScroll(scrollY,h,geometry.canvasHeight);
    SCROLLINFO si{sizeof(si),SIF_RANGE|SIF_PAGE|SIF_POS,0,geometry.canvasHeight-1,(UINT)h,scrollY,0};
    SetScrollInfo(frame,SB_VERT,&si,1);
    Pos(title,geometry.title);Pos(subtitle,geometry.subtitle);
    Pos(newBtn,geometry.newButton);Pos(refreshBtn,geometry.refreshButton);
    Pos(section,geometry.listHeading);
    Pos(searchLabel,geometry.searchLabel);Pos(searchBox,geometry.search);
    Pos(filterBox,geometry.filter);Pos(listView,geometry.list);
    Pos(connectBtn,geometry.connect);Pos(deleteBtn,geometry.remove);
    Pos(formTitle,geometry.editorHeading);
    Pos(hostLabel,geometry.hostLabel);Pos(portLabel,geometry.portLabel);
    Pos(hostBox,geometry.host);Pos(portBox,geometry.port);
    Pos(userLabel,geometry.userLabel);Pos(userBox,geometry.user);
    Pos(passLabel,geometry.passLabel);Pos(passBox,geometry.pass);
    Pos(targetLabel,geometry.targetLabel);Pos(modeBox,geometry.mode);
    Pos(preview,geometry.preview);Pos(help,geometry.help);
    Pos(saveBtn,geometry.save);Pos(convertBtn,geometry.convert);
    Pos(cleanupBtn,geometry.cleanup);Pos(cancelBtn,geometry.cancel);
    Pos(status,geometry.status);
    const auto summary=geometry.summary;
    Pos(favoritesTabBtn,{summary.x+14,summary.y+10,134,38});
    Pos(credentialsTabBtn,{summary.x+160,summary.y+10,146,38});
    // The summary label occupies the remaining horizontal space.
    const int summaryStart=summary.x+320;
    Pos(counts,{summaryStart,summary.y+12,
                summary.x+summary.w-summaryStart-12,34});
    Pos(toggleFavoriteBtn,{geometry.editorHeading.x,geometry.editorHeading.y+40,
                           geometry.editorHeading.w,31});
    Pos(favoritesHeading,geometry.listHeading);
    Pos(favoritesEditHeading,geometry.editorHeading);
    Pos(favoritesSearch,{geometry.search.x,geometry.search.y,
                         geometry.list.w-(geometry.search.x-geometry.list.x),35});
    Pos(favoritesList,geometry.list);
    Pos(favoritesConnect,geometry.connect);Pos(favoritesRemove,geometry.remove);
    auto editor=geometry.editorHeading;
    const int fx=editor.x, fw=editor.w, y=editor.y+77;
    Pos(favoritesNameLabel,{fx,y,fw,22});
    Pos(favoritesName,{fx,y+25,fw,36});
    Pos(favoritesPortLabel,{fx,y+82,fw,22});
    Pos(favoritesPort,{fx,y+108,fw,36});
    Pos(favoritesInfo,{fx,y+166,fw,70});
    Pos(favoritesHint,{fx,y+256,fw,90});
    Pos(favoritesSave,geometry.save);Pos(favoritesEdit,geometry.convert);
    int favWidth=geometry.list.w-10;
    SendMessageW(favoritesList,LVM_SETCOLUMNWIDTH,0,(LPARAM)S(favWidth*39/100));
    SendMessageW(favoritesList,LVM_SETCOLUMNWIDTH,1,(LPARAM)S(favWidth*28/100));
    SendMessageW(favoritesList,LVM_SETCOLUMNWIDTH,2,(LPARAM)S(favWidth*13/100));
    SendMessageW(favoritesList,LVM_SETCOLUMNWIDTH,3,(LPARAM)S(favWidth*20/100));
    ShowCurrentTab();
    int colTotal=geometry.list.w-10;
    SendMessageW(listView,LVM_SETCOLUMNWIDTH,0,(LPARAM)S(colTotal*42/100));
    SendMessageW(listView,LVM_SETCOLUMNWIDTH,1,(LPARAM)S(colTotal*27/100));
    SendMessageW(listView,LVM_SETCOLUMNWIDTH,2,(LPARAM)S(colTotal*19/100));
    SendMessageW(listView,LVM_SETCOLUMNWIDTH,3,(LPARAM)S(colTotal*12/100));
    InvalidateRect(frame,nullptr,0);
}
void ScrollTo(int wanted) {
    if(!frame) return;
    RECT rc{};GetClientRect(frame,&rc);
    int h=(int)(rc.bottom*96/dpi);
    int next=layout::ClampScroll(wanted,h,geometry.canvasHeight);
    if(next!=scrollY) {scrollY=next;Layout();}
}
int GetFilter() {return (int)SendMessageW(filterBox,CB_GETCURSEL,0,0);}
void UpdateActions() {
    const bool domain=selected>=0 && items[selected].type==CRED_TYPE_DOMAIN_PASSWORD;
    EnableWindow(convertBtn,domain && !conversionPending);
    EnableWindow(saveBtn,!conversionPending);
    EnableWindow(deleteBtn,selected>=0 && !conversionPending);
    EnableWindow(newBtn,!conversionPending);
    EnableWindow(listView,!conversionPending);
    EnableWindow(searchBox,!conversionPending);
    EnableWindow(filterBox,!conversionPending);
    EnableWindow(hostBox,!conversionPending);
    EnableWindow(portBox,!conversionPending);
    EnableWindow(userBox,!conversionPending);
    EnableWindow(passBox,!conversionPending);
    EnableWindow(modeBox,!conversionPending);
    EnableWindow(toggleFavoriteBtn,selected>=0 && favoriteStoreWritable && !conversionPending);
    if(selected>=0) {
        bool included=favorites::Find(bookmarks.entries,favoriteCount,items[selected].target,items[selected].type)>=0;
        SetText(toggleFavoriteBtn,included?L"★ 已收藏 · 取消收藏":L"☆ 收藏此凭据");
    } else SetText(toggleFavoriteBtn,L"☆ 选中凭据后收藏");
    EnableWindow(cleanupBtn,conversionPending);
    EnableWindow(cancelBtn,conversionPending);
    SetText(saveBtn,conversionPending?L"等待连接确认":(domain?L"保存为通用凭据":
           (selected>=0?L"更新 / 复制凭据":L"保存通用凭据")));
    SetText(formTitle,conversionPending?L"通用凭据已保存 · 等待登录确认":
        (selected<0?L"新建通用凭据":
         (domain?L"域密码凭据 · 转换":L"通用凭据 · 详情 / 编辑")));
}
void PreviewTarget() {
    if(!hostBox || !modeBox) return;
    wchar_t host[260]={},p[16]={},target[520]={},out[580]=L"凭据目标名：";
    GetWindowTextW(hostBox,host,260);GetWindowTextW(portBox,p,16);
    core::Trim(host);core::Trim(p);
    unsigned port=0;
    bool include=SendMessageW(modeBox,CB_GETCURSEL,0,0)==1;
    if(core::ParsePort(p,&port) && core::BuildTargetWithPort(host,port,include,target,520)) {
        core::Append(out,580,target);
        if(port!=3389 && !include)
            core::Append(out,580,L"  （目标名称不含端口，请核对 MSTSC 保存的名称）");
    } else core::Append(out,580,L"请填写有效的主机名及端口");
    SetText(preview,out);
}
void ClearEditor() {
    uiUpdating=true;
    selected=-1;
    SetText(hostBox,L"");SetText(portBox,L"");SetText(userBox,currentUser);
    SetText(passBox,L"");SendMessageW(modeBox,CB_SETCURSEL,0,0);
    LVITEMW selection{};
    selection.stateMask=LVIS_SELECTED|LVIS_FOCUSED;
    selection.state=0;
    SendMessageW(listView,LVM_SETITEMSTATE,(WPARAM)-1,(LPARAM)&selection);
    uiUpdating=false;
    PreviewTarget();UpdateActions();
    Status(L"新建凭据：服务器与端口留空，用户名已填写为当前 Windows 用户。");
    SetFocus(hostBox);
}
void Select(int index) {
    if(index<0 || (unsigned)index>=count) return;
    selected=index;
    const Entry& e=items[index];
    wchar_t host[300]={},port[16]={};unsigned explicitPort=0,n=3389;
    if(!core::ParseTargetEndpoint(e.target,host,300,&explicitPort)) return;
    if(explicitPort) n=explicitPort;
    else core::PortFromComment(e.comment,&n);
    core::FormatUnsigned(n,port,16);
    uiUpdating=true;
    SetText(hostBox,host);SetText(portBox,port);SetText(userBox,e.user);
    SetText(passBox,L"");
    SendMessageW(modeBox,CB_SETCURSEL,explicitPort?1:0,0);
    uiUpdating=false;
    PreviewTarget();UpdateActions();
    if(e.type==CRED_TYPE_DOMAIN_PASSWORD)
        Status(L"域密码凭据：选择“转换为通用凭据”，输入密码后可测试并清理旧记录。");
    else Status(L"通用凭据：密码留空可保留原密码；更改目标名将复制凭据并保留原记录。");
}
void FillRow(int row,unsigned idx) {
    const Entry& e=items[idx];
    LVITEMW line{};line.mask=LVIF_TEXT|LVIF_PARAM;
    line.iItem=row;line.pszText=(wchar_t*)e.target;line.lParam=(LPARAM)idx;
    int inserted=(int)SendMessageW(listView,LVM_INSERTITEMW,0,(LPARAM)&line);
    if(inserted<0) return;
    wchar_t host[300]={},port[16]=L"3389";unsigned explicitPort=0,n=3389;
    if(core::ParseTargetEndpoint(e.target,host,300,&explicitPort)) {
        if(explicitPort) n=explicitPort;
        else core::PortFromComment(e.comment,&n);
        core::FormatUnsigned(n,port,16);
    } else core::Copy(port,16,L"—");
    LVITEMW cell{};cell.iItem=inserted;
    cell.iSubItem=1;cell.pszText=(wchar_t*)e.user;
    SendMessageW(listView,LVM_SETITEMTEXTW,(WPARAM)inserted,(LPARAM)&cell);
    cell.iSubItem=2;cell.pszText=(wchar_t*)(e.type==CRED_TYPE_GENERIC?L"通用凭据":L"域密码");
    SendMessageW(listView,LVM_SETITEMTEXTW,(WPARAM)inserted,(LPARAM)&cell);
    cell.iSubItem=3;cell.pszText=port;
    SendMessageW(listView,LVM_SETITEMTEXTW,(WPARAM)inserted,(LPARAM)&cell);
}
void FilterRows() {
    if(!listView) return;
    wchar_t term[260]={};GetWindowTextW(searchBox,term,260);core::Trim(term);
    int filter=GetFilter();
    uiUpdating=true;
    SendMessageW(listView,LVM_DELETEALLITEMS,0,0);
    shown=0;bool selectedVisible=false;
    for(unsigned i=0;i<count;++i) {
        if(filter==1 && items[i].type!=CRED_TYPE_GENERIC) continue;
        if(filter==2 && items[i].type!=CRED_TYPE_DOMAIN_PASSWORD) continue;
        if(!core::ContainsInsensitive(items[i].target,term) &&
           !core::ContainsInsensitive(items[i].user,term)) continue;
        visible[shown]=i;
        FillRow((int)shown,i);
        if((int)i==selected) {
            selectedVisible=true;
            LVITEMW state{};state.stateMask=LVIS_SELECTED|LVIS_FOCUSED;
            state.state=LVIS_SELECTED|LVIS_FOCUSED;
            SendMessageW(listView,LVM_SETITEMSTATE,(WPARAM)shown,(LPARAM)&state);
        }
        ++shown;
    }
    uiUpdating=false;
    if(selected>=0 && !selectedVisible) {
        selected=-1;
        UpdateActions();
        Status(L"筛选结果不包含之前选中的凭据，已取消该凭据的选中状态。");
    }
}
void Refresh(bool preserve=true) {
    wchar_t oldTarget[520]={};DWORD oldType=0;
    if(preserve && selected>=0 && (unsigned)selected<count) {
        core::Copy(oldTarget,520,items[selected].target);oldType=items[selected].type;
    }
    DWORD total=0;CREDENTIALW** found=nullptr;
    BOOL enumerated=CredEnumerateW(L"TERMSRV/*",0,&total,&found);
    if(!enumerated && GetLastError()!=ERROR_NOT_FOUND) {
        Failure(L"读取 Windows 凭据管理器失败。",GetLastError());return;
    }
    count=0;selected=-1;
    if(enumerated) {
        for(DWORD i=0;i<total && count<MAX_ENTRIES;++i) {
            CREDENTIALW* c=found[i];
            if(c->Type!=CRED_TYPE_GENERIC && c->Type!=CRED_TYPE_DOMAIN_PASSWORD) continue;
            if(!c->TargetName || !core::EqualsInsensitivePrefix(c->TargetName,L"TERMSRV/")) continue;
            Entry& e=items[count];
            if(!core::Copy(e.target,520,c->TargetName) ||
               !core::Copy(e.user,520,c->UserName?c->UserName:L"") ||
               !core::Copy(e.comment,260,c->Comment?c->Comment:L"")) continue;
            e.type=c->Type;
            if(oldType==e.type && core::SameTarget(oldTarget,e.target)) selected=(int)count;
            ++count;
        }
        CredFree(found);
    }
    unsigned normal=0,legacy=0;
    for(unsigned i=0;i<count;++i) {
        if(items[i].type==CRED_TYPE_GENERIC) ++normal;else ++legacy;
    }
    wchar_t t[160]=L"凭据总数  ";wchar_t num[20];core::FormatUnsigned(count,num,20);
    core::Append(t,160,num);core::Append(t,160,L"       通用凭据  ");
    core::FormatUnsigned(normal,num,20);core::Append(t,160,num);
    core::Append(t,160,L"       域密码凭据  ");
    core::FormatUnsigned(legacy,num,20);core::Append(t,160,num);
    if(total>MAX_ENTRIES) core::Append(t,160,L"       （仅显示前 2048 项）");
    SetText(counts,t);FilterRows();UpdateActions();
    if(selected>=0) Select(selected);
    RefreshFavorites(true);ShowCurrentTab();
    if(selected<0) Status(count?L"已从当前 Windows 用户的凭据管理器加载记录。":L"尚未发现 RDP 凭据。可在右侧创建。 ");
}
// Always display and select the newly written record, including after converting
// a legacy record with the same target name and a different credential type.
// Clearing filters is intentional: a hidden record must not leave the editor
// showing fields unrelated to the current list selection.
bool ShowCredential(const wchar_t* target,DWORD type) {
    int index=core::FindByTargetAndType(items,count,target,type);
    if(index<0) return false;
    uiUpdating=true;
    SetText(searchBox,L"");
    SendMessageW(filterBox,CB_SETCURSEL,0,0);
    uiUpdating=false;
    selected=index;
    FilterRows();
    Select(index);
    for(unsigned row=0;row<shown;++row) {
        if(visible[row]!=(unsigned)index) continue;
        LVITEMW state{};
        state.stateMask=LVIS_SELECTED|LVIS_FOCUSED;
        state.state=LVIS_SELECTED|LVIS_FOCUSED;
        uiUpdating=true;
        SendMessageW(listView,LVM_SETITEMSTATE,(WPARAM)row,(LPARAM)&state);
        SendMessageW(listView,LVM_ENSUREVISIBLE,(WPARAM)row,0);
        uiUpdating=false;
        return true;
    }
    return false;
}
int SelectedFromList() {
    LONG_PTR line=SendMessageW(listView,LVM_GETNEXTITEM,(WPARAM)-1,LVNI_SELECTED);
    if(line<0 || (unsigned)line>=shown) return -1;
    return (int)visible[line];
}
bool Fields(wchar_t* host,wchar_t* portStr,wchar_t* username,unsigned* port,bool usernameRequired) {
    GetWindowTextW(hostBox,host,260);GetWindowTextW(portBox,portStr,16);
    GetWindowTextW(userBox,username,520);
    core::Trim(host);core::Trim(portStr);core::Trim(username);
    if(!core::ValidateHost(host)) {
        Alert(L"服务器地址无效。请填写主机名、IPv4 地址或带方括号的 IPv6 地址。\r\n请在独立的端口字段填写端口号。");
        SetFocus(hostBox);return false;
    }
    if(!core::ParsePort(portStr,port)) {
        Alert(L"端口必须介于 1 至 65535 之间，留空表示默认端口 3389。");
        SetFocus(portBox);return false;
    }
    if(usernameRequired && !username[0]) {
        Alert(L"请输入远程桌面登录用户名。 ");SetFocus(userBox);return false;
    }
    return true;
}

int CredentialPresence(const wchar_t* target,DWORD type) {
    CREDENTIALW* c=nullptr;
    if(CredReadW(target,type,0,&c)) {CredFree(c);return 1;}
    DWORD error=GetLastError();
    if(error==ERROR_NOT_FOUND) return 0;
    Failure(L"无法检查 Windows 凭据。",error);
    return -1;
}
bool SameBytes(const BYTE* a,const BYTE* b,DWORD size) {
    unsigned different=0;
    for(DWORD i=0;i<size;++i) different |= unsigned(a[i]^b[i]);
    return different==0;
}
bool WriteAndVerify(const wchar_t* target,const wchar_t* user,unsigned port,
                    const BYTE* blob,DWORD blobSize,bool preserve) {
    wchar_t comment[80]=L"RDPCredManager.Port=";
    wchar_t p[16];core::FormatUnsigned(port,p,16);core::Append(comment,80,p);
    CREDENTIALW c{};
    c.Type=CRED_TYPE_GENERIC;c.TargetName=(LPWSTR)target;
    c.UserName=(LPWSTR)user;c.Comment=comment;c.Persist=CRED_PERSIST_LOCAL_MACHINE;
    c.CredentialBlob=(LPBYTE)blob;c.CredentialBlobSize=blobSize;
    if(!CredWriteW(&c,preserve?CRED_PRESERVE_CREDENTIAL_BLOB:0)) {
        Failure(L"写入通用凭据失败。",GetLastError());return false;
    }
    CREDENTIALW* check=nullptr;
    if(!CredReadW(target,CRED_TYPE_GENERIC,0,&check)) {
        Failure(L"凭据已写入，但重新读取验证失败。",GetLastError());return false;
    }
    bool ok=check->UserName && core::EqualInsensitive(check->UserName,user) &&
            (preserve || (check->CredentialBlobSize==blobSize &&
                          (blobSize==0 || (check->CredentialBlob && SameBytes(blob,check->CredentialBlob,blobSize)))));
    CredFree(check);
    if(!ok) Alert(L"Windows 已写入凭据，但读取验证结果不一致。请重新检查该记录。 ");
    return ok;
}
void Save(bool converting) {
    // The Save action handles the selected legacy credential as a conversion.
    if(!converting && selected>=0 && items[selected].type==CRED_TYPE_DOMAIN_PASSWORD)
        converting=true;
    if(conversionPending) {
        Alert(L"请先完成当前凭据转换，或取消转换并保留两条凭据。");return;
    }
    if(converting && (selected<0 || items[selected].type!=CRED_TYPE_DOMAIN_PASSWORD)) {
        Alert(L"请在列表中选择一条域密码凭据，再开始转换。 ");return;
    }
    wchar_t host[260]={},portText[16]={},user[520]={},target[520]={};unsigned port=3389;
    if(!Fields(host,portText,user,&port,true)) return;
    bool withPort=SendMessageW(modeBox,CB_GETCURSEL,0,0)==1;
    if(!core::BuildTargetWithPort(host,port,withPort,target,520)) {
        Alert(L"凭据目标名称过长或地址无效。 ");return;
    }
    wchar_t password[1025]={};
    int length=GetWindowTextW(passBox,password,1025);
    if(length<0 || length>512) {
        Wipe(password,1025);Alert(L"密码长度不得超过 512 个 UTF-16 字符。");return;
    }
    const bool same=selected>=0 && items[selected].type==CRED_TYPE_GENERIC &&
                    core::SameTarget(items[selected].target,target);
    if(converting && !length) {
        Wipe(password,1025);
        Alert(L"旧式域密码凭据的密码不可由本程序读取。转换时请重新输入登录密码。");return;
    }
    CREDENTIALW* source=nullptr;
    bool preserve=false;
    const BYTE* blob=(const BYTE*)password;
    DWORD blobSize=(DWORD)length*sizeof(wchar_t);
    if(!length && !converting && same) {
        if(CredentialPresence(target,CRED_TYPE_GENERIC)!=1) {
            Wipe(password,1025);Alert(L"原通用凭据不存在，请输入密码后重新保存。 ");return;
        }
        blob=nullptr;blobSize=0;preserve=true;
    } else if(!length && !converting && selected>=0 && items[selected].type==CRED_TYPE_GENERIC) {
        if(!CredReadW(items[selected].target,CRED_TYPE_GENERIC,0,&source)) {
            DWORD err=GetLastError();Wipe(password,1025);
            Failure(L"无法读取原通用凭据。请重新输入密码后重试。",err);return;
        }
        blob=source->CredentialBlob;blobSize=source->CredentialBlobSize;
        if(!blob || !blobSize) {
            CredFree(source);Wipe(password,1025);
            Alert(L"原通用凭据中没有可复制的密码。请重新输入密码。 ");return;
        }
    } else if(!length) {
        Wipe(password,1025);Alert(L"请输入密码，才能创建新凭据。 ");return;
    }
    if(converting) {
        if(MessageBoxW(frame,
            L"将使用重新输入的密码创建通用凭据。\r\n\r\n通用凭据可由同一 Windows 用户身份下的应用读取，安全保护级别与域密码凭据不同。\r\n\r\n原域密码凭据会暂时保留，待您确认连接成功后再删除。\r\n\r\n是否继续？",
            L"确认转换凭据",MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2)!=IDYES) {
            if(source) CredFree(source);Wipe(password,1025);return;
        }
    }
    int targetPresence=CredentialPresence(target,CRED_TYPE_GENERIC);
    if(targetPresence<0) {if(source) CredFree(source);Wipe(password,1025);return;}
    if(converting && targetPresence==1) {
        if(source) CredFree(source);
        Wipe(password,1025);
        Alert(L"目标名称下已存在通用凭据。为避免覆盖已经保存的密码，转换操作已取消。\r\n\r\n请检查已有的通用凭据，或选择其他正确的凭据目标名称。");
        return;
    }
    if(targetPresence==1 && !same) {
        if(MessageBoxW(frame,
            L"目标名称下已有通用凭据。继续将覆盖其用户名与密码。\r\n\r\n是否覆盖此凭据？",
            L"确认覆盖现有通用凭据",MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2)!=IDYES) {
            if(source) CredFree(source);Wipe(password,1025);return;
        }
    }
    bool saved=WriteAndVerify(target,user,port,blob,blobSize,preserve);
    if(source) CredFree(source);
    Wipe(password,1025);SetText(passBox,L"");
    if(!saved) return;
    if(converting) {
        conversionPending=true;
        core::Copy(pendingOld,520,items[selected].target);
        core::Copy(pendingNew,520,target);
        core::Copy(pendingUser,520,user);
    }
    // Select the *new* generic record. The legacy record intentionally remains
    // visible until the user confirms that an actual RDP login succeeded.
    Refresh(false);
    if(!ShowCredential(target,CRED_TYPE_GENERIC))
        Alert(L"保存后没有在凭据列表中找到通用凭据。请刷新列表，检查新凭据是否存在。");
    UpdateActions();
    if(converting) {
        UpdateActions();
        Status(L"通用凭据已写入并验证。请连接测试，确认成功后点击右侧“确认连接成功并清理旧记录”。");
        Notice(L"新通用凭据已保存并通过 Windows 凭据管理器的读取验证。\r\n\r\n请点击“连接远程桌面”进行实际登录测试。\r\n成功登录后，再点击“确认连接成功并清理旧记录”。\r\n\r\n程序不会自动判断 RDP 登录是否成功，也不会提前删除旧记录。");
    } else Status(same?L"已更新通用凭据，并验证保存结果。":L"已保存通用凭据。若复制到新目标名，旧记录仍保留，可自行删除。");
}
void CancelConversion() {
    if(!conversionPending) return;
    if(MessageBoxW(frame,
        L"取消转换确认流程？\r\n\r\n新创建的通用凭据和原域密码凭据都会保留在 Windows 凭据管理器中，稍后可以分别管理。",
        L"保留当前凭据",MB_YESNO|MB_ICONINFORMATION|MB_DEFBUTTON2)!=IDYES) return;
    wchar_t newTarget[520]={};core::Copy(newTarget,520,pendingNew);
    conversionPending=false;pendingOld[0]=0;pendingNew[0]=0;pendingUser[0]=0;
    Refresh(false);
    ShowCredential(newTarget,CRED_TYPE_GENERIC);
    UpdateActions();
    Status(L"已结束转换确认流程，新通用凭据与旧域密码凭据均已保留。");
}
bool MigrateConvertedFavorite(const wchar_t* oldTarget,const wchar_t* newTarget);
void Cleanup() {
    if(!conversionPending) return;
    int newPresent=CredentialPresence(pendingNew,CRED_TYPE_GENERIC);
    if(newPresent<0) return;
    if(newPresent==0) {
        Alert(L"未找到刚才创建的通用凭据。旧凭据将保留。");return;
    }
    CREDENTIALW* pendingCheck=nullptr;
    if(!CredReadW(pendingNew,CRED_TYPE_GENERIC,0,&pendingCheck)) {
        Failure(L"无法复核新通用凭据，旧凭据仍然保留。",GetLastError());return;
    }
    bool userMatches=pendingCheck->UserName && core::EqualInsensitive(pendingCheck->UserName,pendingUser);
    CredFree(pendingCheck);
    if(!userMatches) {
        Alert(L"新凭据的用户名已发生变化，停止清理旧凭据。请重新检查连接信息。");return;
    }
    int oldPresent=CredentialPresence(pendingOld,CRED_TYPE_DOMAIN_PASSWORD);
    if(oldPresent<0) return;
    if(oldPresent==0) {
        Alert(L"原域密码凭据已不存在，清理流程将结束。");
        conversionPending=false;pendingOld[0]=0;pendingNew[0]=0;pendingUser[0]=0;
        UpdateActions();return;
    }
    wchar_t msg[1250]=L"请确认：您已经使用新保存的通用凭据成功登录远程桌面。\r\n\r\n旧凭据：";
    core::Append(msg,1250,pendingOld);
    core::Append(msg,1250,L"\r\n新凭据：");core::Append(msg,1250,pendingNew);
    core::Append(msg,1250,L"\r\n\r\n选择“是”将永久删除旧域密码凭据。是否已成功登录并确认删除？");
    if(MessageBoxW(frame,msg,L"确认登录成功并删除旧凭据",MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2)!=IDYES) return;
    if(!MigrateConvertedFavorite(pendingOld,pendingNew)) {
        Alert(L"保存转换后的收藏夹关联信息失败。旧凭据暂时保留。 ");return;
    }
    if(!CredDeleteW(pendingOld,CRED_TYPE_DOMAIN_PASSWORD,0)) {
        Failure(L"删除原域密码凭据失败，新通用凭据已保留。",GetLastError());return;
    }
    wchar_t newTarget[520]={};core::Copy(newTarget,520,pendingNew);
    conversionPending=false;pendingOld[0]=0;pendingNew[0]=0;pendingUser[0]=0;
    Refresh(false);
    ShowCredential(newTarget,CRED_TYPE_GENERIC);
    UpdateActions();
    Status(L"已删除原域密码凭据，当前选中的是新通用凭据。 ");
}
void DeleteSelected() {
    if(selected<0 || (unsigned)selected>=count) {Alert(L"请先选择要删除的凭据。 ");return;}
    const Entry& e=items[selected];
    wchar_t msg[760]=L"将从当前 Windows 用户的凭据管理器删除以下记录：\r\n\r\n";
    core::Append(msg,760,e.target);
    core::Append(msg,760,e.type==CRED_TYPE_GENERIC?L"\r\n类型：通用凭据":L"\r\n类型：域密码凭据");
    core::Append(msg,760,L"\r\n\r\n此操作无法撤销。确定删除吗？");
    if(MessageBoxW(frame,msg,L"删除凭据",MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2)!=IDYES) return;
    if(!CredDeleteW(e.target,e.type,0)) {Failure(L"删除凭据失败。",GetLastError());return;}
    if(conversionPending && ((e.type==CRED_TYPE_DOMAIN_PASSWORD &&
        core::SameTarget(pendingOld,e.target)) || (e.type==CRED_TYPE_GENERIC &&
        core::SameTarget(pendingNew,e.target)))) {
        conversionPending=false;pendingOld[0]=0;pendingNew[0]=0;pendingUser[0]=0;
    }
    Refresh(false);ClearEditor();Status(L"已删除选定的 Windows 凭据。 ");
}
bool LaunchEndpoint(const wchar_t* host,unsigned port,bool forcePort=false) {
    wchar_t endpoint[290]={},args[320]=L"/v:",systemPath[340]={};
    if(!core::BuildEndpoint(host,port,endpoint,290) ||
       (port==3389 && forcePort && !core::Append(endpoint,290,L":3389")) ||
       !core::Append(args,320,endpoint)) {
        Alert(L"无法生成有效的远程桌面连接目标。");return false;
    }
    DWORD n=GetSystemDirectoryW(systemPath,340);
    if(!n || n+11>=340 || !core::Append(systemPath,340,L"\\mstsc.exe")) {
        Alert(L"无法定位系统远程桌面客户端 mstsc.exe。");return false;
    }
    HINSTANCE result=ShellExecuteW(frame,L"open",systemPath,args,nullptr,SW_SHOWNORMAL);
    if((ULONG_PTR)result<=32) {
        Alert(L"启动 mstsc.exe 失败。请确认系统安装了远程桌面客户端。");return false;
    }
    return true;
}
void Connect() {
    wchar_t host[260]={},portText[16]={},user[520]={};unsigned port=3389;
    if(conversionPending) {
        unsigned explicitPort=0;
        if(!core::ParseTargetEndpoint(pendingNew,host,260,&explicitPort)) {
            Alert(L"转换记录中的服务器目标名称无效。");return;
        }
        port=explicitPort?explicitPort:3389;
        if(!explicitPort) {
            CREDENTIALW* c=nullptr;
            if(!CredReadW(pendingNew,CRED_TYPE_GENERIC,0,&c)) {
                Failure(L"无法读取待测试的通用凭据。",GetLastError());return;
            }
            core::PortFromComment(c->Comment,&port);
            CredFree(c);
        }
    } else if(!Fields(host,portText,user,&port,false)) return;
    if(!LaunchEndpoint(host,port)) return;
    Status(conversionPending?L"已打开待验证目标。确认远程桌面登录成功后，才能删除旧凭据。":
                              L"已启动 mstsc.exe。请确认远程登录是否成功。");
}
// Favorites metadata lives in HKCU; Windows Credential Manager remains the only password store.
const wchar_t* FavoritesPath() {
    return headlessSelfTest ? L"Software\\RDPCredManager\\SelfTest" : L"Software\\RDPCredManager";
}
HKEY FavoritesRoot() {return (HKEY)(LONG_PTR)(-2147483647);}
void LoadFavorites() {
    favoriteCount=0;selectedFavorite=-1;
    Wipe((wchar_t*)&bookmarks,(unsigned)(sizeof(bookmarks)/sizeof(wchar_t)));
    favoriteStoreWritable=true;
    HKEY key=nullptr;
    LONG result=RegOpenKeyExW(FavoritesRoot(),FavoritesPath(),0,KEY_QUERY_VALUE,&key);
    if(result==ERROR_FILE_NOT_FOUND) return;
    if(result!=0) {favoriteStoreWritable=false;Failure(L"无法打开收藏夹设置。",(DWORD)result);return;}
    DWORD kind=0,bytes=sizeof(bookmarks);
    result=RegQueryValueExW(key,L"Favorites",nullptr,&kind,(BYTE*)&bookmarks,&bytes);
    RegCloseKey(key);
    if(result==ERROR_FILE_NOT_FOUND) return;
    if(result || kind!=REG_BINARY || bytes<12 ||
       bookmarks.signature!=favorites::Signature ||
       bookmarks.version!=favorites::FormatVersion ||
       bookmarks.size>favorites::Limit ||
       bytes!=12+bookmarks.size*sizeof(favorites::Entry)) {
        favoriteStoreWritable=false;
        Alert(L"收藏夹数据无法读取或格式不兼容。为避免覆盖原记录，已暂停收藏夹编辑。原始数据仍保留在注册表中。");
        return;
    }
    favoriteCount=bookmarks.size;
    for(unsigned i=0;i<favoriteCount;++i) {
        // Require NUL terminators before any string operations.
        if(bookmarks.entries[i].target[519] || bookmarks.entries[i].alias[95] ||
           !favorites::Valid(bookmarks.entries[i])) {
            favoriteStoreWritable=false;
            favoriteCount=0;
            Alert(L"收藏夹内容验证失败，已暂时禁用写入，原始数据仍保留。 ");
            return;
        }
        for(unsigned j=0;j<i;++j) if(bookmarks.entries[j].credentialType==bookmarks.entries[i].credentialType &&
            core::SameTarget(bookmarks.entries[j].target,bookmarks.entries[i].target)) {
            favoriteStoreWritable=false;favoriteCount=0;
            Alert(L"收藏夹包含重复条目，已暂停写入以保护原始数据。 ");return;
        }
    }
}
bool PersistFavorites() {
    if(!favoriteStoreWritable) return false;
    bookmarks.signature=favorites::Signature;
    bookmarks.version=favorites::FormatVersion;
    bookmarks.size=favoriteCount;
    HKEY key=nullptr;
    LONG result=RegCreateKeyExW(FavoritesRoot(),FavoritesPath(),0,nullptr,0,
                                KEY_SET_VALUE,nullptr,&key,nullptr);
    if(result!=0){Failure(L"无法创建收藏夹存储位置。",(DWORD)result);return false;}
    result=RegSetValueExW(key,L"Favorites",0,REG_BINARY,(BYTE*)&bookmarks,
                         12+favoriteCount*sizeof(favorites::Entry));
    RegCloseKey(key);
    if(result!=0){Failure(L"收藏夹保存失败。",(DWORD)result);return false;}
    return true;
}
int FavoriteCredentialIndex(const favorites::Entry& e) {
    return core::FindByTargetAndType(items,count,e.target,e.credentialType);
}
void SelectFavorite(int index) {
    selectedFavorite=index;
    uiUpdating=true;
    if(index>=0 && (unsigned)index<favoriteCount) {
        const favorites::Entry& e=bookmarks.entries[index];
        SetText(favoritesName,e.alias);
        wchar_t p[16];core::FormatUnsigned(e.port,p,16);SetText(favoritesPort,p);
        int credential=FavoriteCredentialIndex(e);
        wchar_t details[1300]=L"凭据：";
        core::Append(details,1300,e.target);
        core::Append(details,1300,e.credentialType==CRED_TYPE_GENERIC?L"\r\n类型：通用凭据":L"\r\n类型：域密码凭据");
        if(credential>=0) {
            core::Append(details,1300,L"\r\n登录用户：");core::Append(details,1300,items[credential].user);
            core::Append(details,1300,L"\r\n状态：可启动（远程登录结果由服务器决定）");
        } else core::Append(details,1300,L"\r\n状态：凭据已缺失，请在凭据管理页重新创建");
        SetText(favoritesInfo,details);
    } else {
        SetText(favoritesName,L"");SetText(favoritesPort,L"");
        SetText(favoritesInfo,favoriteCount?L"选择一条收藏会话查看详情。":
                    L"收藏夹暂无会话。请前往“凭据管理”，选中凭据后点击收藏按钮。");
    }
    uiUpdating=false;
    UpdateFavoriteActions();
}
void UpdateFavoriteActions() {
    bool has=selectedFavorite>=0 && (unsigned)selectedFavorite<favoriteCount;
    EnableWindow(favoritesSave,has && favoriteStoreWritable);
    EnableWindow(favoritesRemove,has && favoriteStoreWritable);
    EnableWindow(favoritesEdit,has && FavoriteCredentialIndex(bookmarks.entries[selectedFavorite])>=0);
    EnableWindow(favoritesConnect,has && FavoriteCredentialIndex(bookmarks.entries[selectedFavorite])>=0);
    EnableWindow(favoritesName,has && favoriteStoreWritable);
    EnableWindow(favoritesPort,has && favoriteStoreWritable);
}
void RefreshFavorites(bool preserveSelection) {
    wchar_t oldTarget[520]={};unsigned oldType=0;
    if(preserveSelection && selectedFavorite>=0 && (unsigned)selectedFavorite<favoriteCount) {
        core::Copy(oldTarget,520,bookmarks.entries[selectedFavorite].target);
        oldType=bookmarks.entries[selectedFavorite].credentialType;
    }
    selectedFavorite=-1;
    if(oldTarget[0]) selectedFavorite=favorites::Find(bookmarks.entries,favoriteCount,oldTarget,oldType);
    wchar_t term[260]={};GetWindowTextW(favoritesSearch,term,260);core::Trim(term);
    uiUpdating=true;
    SendMessageW(favoritesList,LVM_DELETEALLITEMS,0,0);
    favoriteShown=0;
    for(unsigned i=0;i<favoriteCount;++i) {
        const auto& fav=bookmarks.entries[i];
        int credential=FavoriteCredentialIndex(fav);
        const wchar_t* username=credential>=0?items[credential].user:L"（凭据缺失）";
        if(!core::ContainsInsensitive(favorites::Name(fav),term) &&
           !core::ContainsInsensitive(fav.target,term) &&
           !core::ContainsInsensitive(username,term)) continue;
        favoriteVisible[favoriteShown]=i;
        LVITEMW row{};row.mask=LVIF_TEXT|LVIF_PARAM;
        row.iItem=(int)favoriteShown;row.lParam=i;row.pszText=(wchar_t*)favorites::Name(fav);
        int inserted=(int)SendMessageW(favoritesList,LVM_INSERTITEMW,0,(LPARAM)&row);
        if(inserted<0) continue;
        LVITEMW cell{};cell.iItem=inserted;cell.iSubItem=1;
        cell.pszText=(wchar_t*)username;
        SendMessageW(favoritesList,LVM_SETITEMTEXTW,(WPARAM)inserted,(LPARAM)&cell);
        wchar_t port[16];core::FormatUnsigned(fav.port,port,16);
        cell.iSubItem=2;cell.pszText=port;
        SendMessageW(favoritesList,LVM_SETITEMTEXTW,(WPARAM)inserted,(LPARAM)&cell);
        cell.iSubItem=3;cell.pszText=(wchar_t*)(credential>=0?L"可连接":L"凭据缺失");
        SendMessageW(favoritesList,LVM_SETITEMTEXTW,(WPARAM)inserted,(LPARAM)&cell);
        if(selectedFavorite==(int)i) {
            LVITEMW state{};state.stateMask=LVIS_SELECTED|LVIS_FOCUSED;
            state.state=LVIS_SELECTED|LVIS_FOCUSED;
            SendMessageW(favoritesList,LVM_SETITEMSTATE,(WPARAM)favoriteShown,(LPARAM)&state);
        }
        ++favoriteShown;
    }
    if(selectedFavorite>=0) {
        bool visible=false;for(unsigned i=0;i<favoriteShown;++i)
            if(favoriteVisible[i]==(unsigned)selectedFavorite) visible=true;
        if(!visible) selectedFavorite=-1;
    }
    if(selectedFavorite<0 && favoriteShown>0 && !term[0]) {
        selectedFavorite=(int)favoriteVisible[0];
        LVITEMW state{};state.stateMask=LVIS_SELECTED|LVIS_FOCUSED;
        state.state=LVIS_SELECTED|LVIS_FOCUSED;
        SendMessageW(favoritesList,LVM_SETITEMSTATE,0,(LPARAM)&state);
    }
    uiUpdating=false;
    SelectFavorite(selectedFavorite);
}
int FavoriteSelectionFromList() {
    LONG_PTR row=SendMessageW(favoritesList,LVM_GETNEXTITEM,(WPARAM)-1,LVNI_SELECTED);
    if(row<0 || (unsigned)row>=favoriteShown) return -1;
    return (int)favoriteVisible[row];
}
void ChangeTab(bool favorite) {
    favoritesTab=favorite;
    scrollY=0;
    ShowCurrentTab();
    Layout();
    SetText(status,favoritesTab?L"收藏夹：双击会话即可启动远程桌面。":
                              L"凭据管理：可以编辑、转换和收藏已保存的凭据。");
}
void ShowCurrentTab() {
    if(!frame || !title) return;
    HWND credentialControls[]={section,formTitle,hostLabel,portLabel,userLabel,passLabel,
        targetLabel,preview,help,hostBox,portBox,userBox,passBox,modeBox,
        searchBox,filterBox,listView,newBtn,refreshBtn,saveBtn,convertBtn,cleanupBtn,
        cancelBtn,connectBtn,deleteBtn,toggleFavoriteBtn};
    HWND favoriteControls[]={favoritesHeading,favoritesEditHeading,favoritesNameLabel,
        favoritesPortLabel,favoritesInfo,favoritesHint,favoritesName,favoritesPort,
        favoritesSearch,favoritesList,favoritesConnect,favoritesRemove,favoritesSave,favoritesEdit};
    for(HWND h:credentialControls) if(h) ShowWindow(h,favoritesTab?SW_HIDE:SW_SHOW);
    for(HWND h:favoriteControls) if(h) ShowWindow(h,favoritesTab?SW_SHOW:SW_HIDE);
    SetText(favoritesTabBtn,favoritesTab?L"★ 收藏夹 · 当前":L"☆ 收藏夹");
    SetText(credentialsTabBtn,favoritesTab?L"凭据管理":L"凭据管理 · 当前");
    wchar_t summary[160]=L"收藏会话  ";wchar_t total[16];core::FormatUnsigned(favoriteCount,total,16);
    core::Append(summary,160,total);
    if(!favoritesTab) {
        core::Append(summary,160,L"    凭据  ");core::FormatUnsigned(count,total,16);
        core::Append(summary,160,total);
    }
    SetText(counts,summary);
}
void ToggleFavorite() {
    if(!favoriteStoreWritable || selected<0 || (unsigned)selected>=count || conversionPending) return;
    Entry& credential=items[selected];
    int index=favorites::Find(bookmarks.entries,favoriteCount,credential.target,credential.type);
    undoBookmarks=bookmarks;unsigned before=favoriteCount;
    if(index>=0) {
        favorites::Remove(bookmarks.entries,&favoriteCount,(unsigned)index);
    } else {
        wchar_t host[300]={};unsigned directPort=0,port=3389;
        if(!core::ParseTargetEndpoint(credential.target,host,300,&directPort)) {
            Alert(L"此凭据目标名称无效，无法收藏。 ");return;
        }
        if(directPort) port=directPort;
        else core::PortFromComment(credential.comment,&port);
        if(!favorites::Add(bookmarks.entries,&favoriteCount,credential.target,credential.type,port)) {
            Alert(L"收藏数量达到上限（128）或记录重复。 ");return;
        }
    }
    if(!PersistFavorites()) {bookmarks=undoBookmarks;favoriteCount=before;return;}
    if(index>=0) selectedFavorite=-1;
    else selectedFavorite=favorites::Find(bookmarks.entries,favoriteCount,credential.target,credential.type);
    RefreshFavorites(index<0);UpdateActions();ShowCurrentTab();
    Status(index>=0?L"已取消收藏，Windows 凭据保持不变。":L"已加入收藏夹。切换到收藏夹即可一键连接。");
}
bool MigrateConvertedFavorite(const wchar_t* oldTarget,const wchar_t* newTarget) {
    int oldIndex=favorites::Find(bookmarks.entries,favoriteCount,oldTarget,CRED_TYPE_DOMAIN_PASSWORD);
    if(oldIndex<0) return true;
    if(!favoriteStoreWritable) return false;
    undoBookmarks=bookmarks;
    unsigned previousCount=favoriteCount;
    int newIndex=favorites::Find(bookmarks.entries,favoriteCount,newTarget,CRED_TYPE_GENERIC);
    if(newIndex>=0) {
        // The generic credential already has its own bookmark; retain it.
        favorites::Remove(bookmarks.entries,&favoriteCount,(unsigned)oldIndex);
    } else {
        auto& item=bookmarks.entries[oldIndex];
        if(!core::Copy(item.target,520,newTarget)) return false;
        item.credentialType=CRED_TYPE_GENERIC;
    }
    if(!PersistFavorites()) {
        bookmarks=undoBookmarks;favoriteCount=previousCount;return false;
    }
    selectedFavorite=-1;RefreshFavorites(false);
    return true;
}
void SaveFavoriteDetails() {
    if(!favoriteStoreWritable || selectedFavorite<0 || (unsigned)selectedFavorite>=favoriteCount) return;
    wchar_t alias[96]={},portText[16]={};unsigned port=0;
    GetWindowTextW(favoritesName,alias,96);GetWindowTextW(favoritesPort,portText,16);
    core::Trim(alias);core::Trim(portText);
    if(!core::ParsePort(portText,&port) || !portText[0]) {
        Alert(L"收藏会话的端口必须是 1 至 65535 之间的整数。 ");return;
    }
    wchar_t targetHost[300]={};unsigned credentialPort=0;
    if(!core::ParseTargetEndpoint(bookmarks.entries[selectedFavorite].target,targetHost,300,&credentialPort) ||
       (credentialPort!=0 && credentialPort!=port)) {
        Alert(L"此凭据目标名称已经包含固定端口。请在凭据管理中创建与新端口匹配的凭据，再将其添加到收藏夹。");
        return;
    }
    favorites::Entry backup=bookmarks.entries[selectedFavorite];
    if(!favorites::Update(bookmarks.entries,favoriteCount,(unsigned)selectedFavorite,alias,port)) {
        Alert(L"会话名称或端口无效。 ");return;
    }
    if(!PersistFavorites()) {bookmarks.entries[selectedFavorite]=backup;return;}
    RefreshFavorites(true);
    Status(L"已保存收藏会话的显示名称和连接端口。凭据密码未发生变化。");
}
void RemoveFavorite() {
    if(!favoriteStoreWritable || selectedFavorite<0 || (unsigned)selectedFavorite>=favoriteCount) return;
    if(MessageBoxW(frame,L"确定取消收藏此会话吗？\r\n\r\nWindows 凭据仍会保留。",
                   L"取消收藏",MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)!=IDYES) return;
    undoBookmarks=bookmarks;unsigned before=favoriteCount;
    favorites::Remove(bookmarks.entries,&favoriteCount,(unsigned)selectedFavorite);
    if(!PersistFavorites()) {bookmarks=undoBookmarks;favoriteCount=before;return;}
    selectedFavorite=-1;RefreshFavorites(false);ShowCurrentTab();
    Status(L"已取消收藏，关联的 Windows 凭据没有删除。");
}
void ConnectFavorite() {
    if(selectedFavorite<0 || (unsigned)selectedFavorite>=favoriteCount) return;
    const favorites::Entry& fav=bookmarks.entries[selectedFavorite];
    if(FavoriteCredentialIndex(fav)<0) {
        Alert(L"无法启动：该收藏项对应的 Windows 凭据已经不存在。请先在凭据管理页重新创建凭据。 ");return;
    }
    wchar_t host[300]={};unsigned explicitPort=0;
    if(!core::ParseTargetEndpoint(fav.target,host,300,&explicitPort)) {
        Alert(L"收藏会话中的服务器地址无效。 ");return;
    }
    if(LaunchEndpoint(host,fav.port,explicitPort!=0)) Status(L"已启动收藏的 RDP 会话。");
}
void EditFavoriteCredential() {
    if(selectedFavorite<0 || (unsigned)selectedFavorite>=favoriteCount) return;
    auto fav=bookmarks.entries[selectedFavorite];
    int index=FavoriteCredentialIndex(fav);
    if(index<0) {Alert(L"原凭据已经不存在，请重新创建。 ");return;}
    ChangeTab(false);
    ShowCredential(fav.target,fav.credentialType);
}
void Columns() {
    const wchar_t* names[]={L"目标名称",L"用户名",L"凭据类型",L"端口"};
    for(int i=0;i<4;++i) {
        LVCOLUMNW col{};col.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
        col.pszText=(LPWSTR)names[i];col.cx=S(i==0?230:i==1?145:i==2?105:70);col.iSubItem=i;
        SendMessageW(listView,LVM_INSERTCOLUMNW,(WPARAM)i,(LPARAM)&col);
    }
    SendMessageW(listView,LVM_SETEXTENDEDLISTVIEWSTYLE,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER,
                 LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);
}
void Init() {
    Fonts();
    whiteBrush=CreateSolidBrush(UI_WHITE);
    navyBrush=CreateSolidBrush(UI_NAVY);
    bgBrush=CreateSolidBrush(UI_BG);
    softBrush=CreateSolidBrush(UI_SOFT);
    title=Control(L"STATIC",L"RDP Credential Manager",0,0,ID_TITLE,large);
    subtitle=Control(L"STATIC",L"Windows 远程桌面凭据管理  ·  本地存储  ·  转换及连接测试",0,0,ID_SUBTITLE,regular);
    counts=Control(L"STATIC",L"正在加载凭据…",0,0,ID_COUNT,strong);
    section=Control(L"STATIC",L"已保存的 RDP 凭据",0,0,ID_SECTION,strong);
    formTitle=Control(L"STATIC",L"凭据详情  /  编辑",0,0,ID_FORMTITLE,strong);
    hostLabel=Control(L"STATIC",L"服务器地址",0,0,ID_HOSTLABEL,regular);
    portLabel=Control(L"STATIC",L"连接端口",0,0,ID_PORTLABEL,regular);
    userLabel=Control(L"STATIC",L"远程登录用户名",0,0,ID_USERLABEL,regular);
    passLabel=Control(L"STATIC",L"密码（更新通用凭据时可留空）",0,0,ID_PASSLABEL,regular);
    targetLabel=Control(L"STATIC",L"凭据目标名称规则",0,0,ID_TARGETLABEL,regular);
    preview=Control(L"STATIC",L"凭据目标名：",0,0,ID_PREVIEW,small);
    help=Control(L"STATIC",L"请使目标名称与 MSTSC 实际保存的名称一致。转换域密码凭据时需重新输入密码，旧记录会保留至您确认登录成功。",0,0,ID_HELP,small);
    searchLabel=Control(L"STATIC",L"搜索",0,0,ID_SEARCHLABEL,regular);
    status=Control(L"STATIC",L"准备就绪",0,0,ID_STATUS,small);
    hostBox=Control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,WS_EX_CLIENTEDGE,ID_HOST,regular);
    portBox=Control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,WS_EX_CLIENTEDGE,ID_PORT,regular);
    userBox=Control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,WS_EX_CLIENTEDGE,ID_USER,regular);
    passBox=Control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL|ES_PASSWORD,WS_EX_CLIENTEDGE,ID_PASS,regular);
    searchBox=Control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,WS_EX_CLIENTEDGE,ID_SEARCH,regular);
    modeBox=Control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,0,ID_MODE,regular);
    filterBox=Control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,0,ID_FILTER,regular);
    listView=Control(L"SysListView32",L"",LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS|WS_TABSTOP|WS_BORDER,
                     WS_EX_CLIENTEDGE,ID_LIST,regular);
    newBtn=Control(L"BUTTON",L"新建凭据",WS_TABSTOP|BS_OWNERDRAW,0,ID_NEW,strong);
    refreshBtn=Control(L"BUTTON",L"刷新列表",WS_TABSTOP|BS_OWNERDRAW,0,ID_REFRESH,regular);
    saveBtn=Control(L"BUTTON",L"保存通用凭据",WS_TABSTOP|BS_OWNERDRAW,0,ID_SAVE,strong);
    convertBtn=Control(L"BUTTON",L"转换通用凭据",WS_TABSTOP|BS_OWNERDRAW,0,ID_CONVERT,regular);
    cleanupBtn=Control(L"BUTTON",L"确认成功并清理旧记录",WS_TABSTOP|BS_OWNERDRAW,0,ID_CLEANUP,regular);
    cancelBtn=Control(L"BUTTON",L"取消转换并保留凭据",WS_TABSTOP|BS_OWNERDRAW,0,ID_CANCEL,regular);
    connectBtn=Control(L"BUTTON",L"连接 / 测试 RDP",WS_TABSTOP|BS_OWNERDRAW,0,ID_CONNECT,strong);
    deleteBtn=Control(L"BUTTON",L"删除选中凭据",WS_TABSTOP|BS_OWNERDRAW,0,ID_DELETE,regular);
    favoritesTabBtn=Control(L"BUTTON",L"☆ 收藏夹",BS_OWNERDRAW|WS_TABSTOP,0,ID_TAB_FAVORITES,strong);
    credentialsTabBtn=Control(L"BUTTON",L"凭据管理",BS_OWNERDRAW|WS_TABSTOP,0,ID_TAB_CREDENTIALS,strong);
    toggleFavoriteBtn=Control(L"BUTTON",L"☆ 收藏此凭据",BS_OWNERDRAW|WS_TABSTOP,0,ID_TOGGLE_FAVORITE,strong);
    favoritesHeading=Control(L"STATIC",L"★ 我的远程桌面会话",0,0,ID_FAVORITES_HEADING,strong);
    favoritesEditHeading=Control(L"STATIC",L"会话详情 · 自定义",0,0,ID_FAVORITES_EDITHEADING,strong);
    favoritesNameLabel=Control(L"STATIC",L"会话显示名称（可留空）",0,0,ID_FAVORITES_NAMELABEL,regular);
    favoritesPortLabel=Control(L"STATIC",L"连接端口",0,0,ID_FAVORITES_PORTLABEL,regular);
    favoritesInfo=Control(L"STATIC",L"",0,0,ID_FAVORITES_INFO,regular);
    favoritesHint=Control(L"STATIC",L"双击收藏的会话即可启动。此处可自定义名称和连接端口。收藏夹只保存会话信息，登录密码仍由 Windows 管理。",0,0,ID_FAVORITES_HINT,small);
    favoritesSearch=Control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,WS_EX_CLIENTEDGE,ID_FAVORITES_SEARCH,regular);
    favoritesName=Control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,WS_EX_CLIENTEDGE,ID_FAVORITES_NAME,regular);
    favoritesPort=Control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,WS_EX_CLIENTEDGE,ID_FAVORITES_PORT,regular);
    favoritesList=Control(L"SysListView32",L"",LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS|WS_TABSTOP|WS_BORDER,
                        WS_EX_CLIENTEDGE,ID_FAVORITES_LIST,regular);
    favoritesConnect=Control(L"BUTTON",L"连接收藏会话",BS_OWNERDRAW|WS_TABSTOP,0,ID_FAVORITES_CONNECT,strong);
    favoritesRemove=Control(L"BUTTON",L"取消收藏",BS_OWNERDRAW|WS_TABSTOP,0,ID_FAVORITES_REMOVE,regular);
    favoritesSave=Control(L"BUTTON",L"保存会话设置",BS_OWNERDRAW|WS_TABSTOP,0,ID_FAVORITES_SAVE,strong);
    favoritesEdit=Control(L"BUTTON",L"管理关联凭据",BS_OWNERDRAW|WS_TABSTOP,0,ID_FAVORITES_EDIT,regular);
    SendMessageW(favoritesSearch,EM_LIMITTEXT,250,0);
    SendMessageW(favoritesName,EM_LIMITTEXT,95,0);
    SendMessageW(favoritesPort,EM_LIMITTEXT,5,0);
    const wchar_t* favCols[]={L"会话",L"登录用户",L"端口",L"状态"};
    for(int i=0;i<4;++i) {
        LVCOLUMNW col{};col.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
        col.pszText=(LPWSTR)favCols[i];col.cx=S(i==0?210:i==1?150:90);col.iSubItem=i;
        SendMessageW(favoritesList,LVM_INSERTCOLUMNW,(WPARAM)i,(LPARAM)&col);
    }
    SendMessageW(favoritesList,LVM_SETEXTENDEDLISTVIEWSTYLE,
                 LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER,
                 LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);
    SetWindowTheme(favoritesList,L"Explorer",nullptr);
    SendMessageW(favoritesList,LVM_SETBKCOLOR,0,UI_WHITE);
    SendMessageW(favoritesList,LVM_SETTEXTBKCOLOR,0,UI_WHITE);
    SendMessageW(favoritesList,LVM_SETTEXTCOLOR,0,UI_TITLE);
    SendMessageW(hostBox,EM_LIMITTEXT,253,0);
    SendMessageW(portBox,EM_LIMITTEXT,5,0);
    SendMessageW(userBox,EM_LIMITTEXT,513,0);
    SendMessageW(passBox,EM_LIMITTEXT,512,0);
    SendMessageW(searchBox,EM_LIMITTEXT,250,0);
    SendMessageW(modeBox,CB_ADDSTRING,0,(LPARAM)L"仅主机名：TERMSRV/host");
    SendMessageW(modeBox,CB_ADDSTRING,0,(LPARAM)L"主机名和端口：TERMSRV/host:port");
    SendMessageW(modeBox,CB_SETCURSEL,0,0);
    SendMessageW(filterBox,CB_ADDSTRING,0,(LPARAM)L"所有凭据");
    SendMessageW(filterBox,CB_ADDSTRING,0,(LPARAM)L"仅通用凭据");
    SendMessageW(filterBox,CB_ADDSTRING,0,(LPARAM)L"仅域密码凭据");
    SendMessageW(filterBox,CB_SETCURSEL,0,0);
    DWORD capacity=260;
    if(!GetUserNameW(currentUser,&capacity)) currentUser[0]=0;
    SetText(userBox,currentUser);
    LoadFavorites();
    favoritesTab=favoriteCount>0;
    Columns();
    SetWindowTheme(listView,L"Explorer",nullptr);
    SendMessageW(listView,LVM_SETBKCOLOR,0,UI_WHITE);
    SendMessageW(listView,LVM_SETTEXTBKCOLOR,0,UI_WHITE);
    SendMessageW(listView,LVM_SETTEXTCOLOR,0,UI_TITLE);
    Layout();PreviewTarget();UpdateActions();Refresh(false);
    RefreshFavorites(false);ShowCurrentTab();
}
void Card(HANDLE dc,layout::Rectangle r,COLORREF fill,COLORREF outline,int radius=16) {
    const int x=S(r.x), y=S(r.y-scrollY),w=S(r.w),h=S(r.h);
    HBRUSH fillBrush=CreateSolidBrush(fill);
    HPEN line=CreatePen(0,S(1),outline);
    if(!fillBrush || !line) {
        if(fillBrush) DeleteObject(fillBrush);
        if(line) DeleteObject(line);
        return;
    }
    HGDIOBJ oldBrush=SelectObject(dc,fillBrush),oldPen=SelectObject(dc,line);
    RoundRect(dc,x,y,x+w,y+h,S(radius),S(radius));
    SelectObject(dc,oldBrush);SelectObject(dc,oldPen);
    DeleteObject(fillBrush);DeleteObject(line);
}
void PaintButton(DRAWITEMSTRUCT* draw) {
    if(!draw || draw->CtlType!=ODT_BUTTON) return;
    const UINT id=draw->CtlID;
    const bool disabled=(draw->itemState&ODS_DISABLED)!=0;
    const bool down=(draw->itemState&ODS_SELECTED)!=0;
    COLORREF fill=UI_WHITE,textColor=UI_TITLE,border=UI_BORDER;
    if(id==ID_SAVE || id==ID_CONNECT || id==ID_NEW || id==ID_FAVORITES_CONNECT || id==ID_FAVORITES_SAVE || id==ID_TAB_FAVORITES || id==ID_TAB_CREDENTIALS) {
        fill=down?0x00726109:UI_ACCENT;
        textColor=UI_WHITE;
        border=fill;
    } else if(id==ID_CONVERT) {
        fill=UI_SOFT;border=0x00D5C2AD;textColor=UI_NAVY;
    } else if(id==ID_DELETE || id==ID_FAVORITES_REMOVE) {
        textColor=UI_RED;
    } else if(id==ID_CLEANUP) {
        fill=UI_NAVY;border=UI_NAVY;textColor=UI_WHITE;
    }
    if((id==ID_TAB_FAVORITES && !favoritesTab) || (id==ID_TAB_CREDENTIALS && favoritesTab)) {
        fill=UI_WHITE;textColor=UI_TITLE;border=UI_BORDER;
    }
    if(disabled) {fill=0x00F1F0ED;textColor=0x00AAA7A3;border=0x00E5E3E0;}
    else if(down && id!=ID_SAVE && id!=ID_CONNECT && id!=ID_NEW) fill=UI_SOFT;
    HBRUSH brush=CreateSolidBrush(fill);
    HPEN pen=CreatePen(0,S(1),border);
    if(!brush || !pen) {
        if(brush) DeleteObject(brush);
        if(pen) DeleteObject(pen);
        return;
    }
    HANDLE dc=draw->hDC;
    HGDIOBJ oldBrush=SelectObject(dc,brush);
    HGDIOBJ oldPen=SelectObject(dc,pen);
    RECT r=draw->rcItem;
    RoundRect(dc,r.left+S(1),r.top+S(1),r.right-S(1),r.bottom-S(1),S(12),S(12));
    SelectObject(dc,oldBrush);SelectObject(dc,oldPen);
    DeleteObject(brush);DeleteObject(pen);
    wchar_t name[160]={};GetWindowTextW(draw->hwndItem,name,160);
    SetBkMode(dc,TRANSPARENT);SetTextColor(dc,textColor);
    HFONT font=(id==ID_NEW||id==ID_SAVE||id==ID_CONNECT||id==ID_FAVORITES_CONNECT||id==ID_FAVORITES_SAVE||id==ID_TAB_FAVORITES||id==ID_TAB_CREDENTIALS)?strong:regular;
    HGDIOBJ previous=SelectObject(dc,font);
    RECT textRect=r;
    textRect.left+=S(7);textRect.right-=S(7);
    DrawTextW(dc,name,-1,&textRect,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
    SelectObject(dc,previous);
    if(draw->itemState&ODS_FOCUS) {
        RECT focus=r;focus.left+=S(5);focus.top+=S(5);
        focus.right-=S(5);focus.bottom-=S(5);
        DrawFocusRect(dc,&focus);
    }
}
LRESULT CALLBACK WindowProc(HWND window,UINT message,WPARAM wParam,LPARAM lParam) {
    switch(message) {
        case WM_CREATE: frame=window;dpi=(int)GetDpiForWindow(window);if(!dpi) dpi=96;Init();return 0;
        case WM_SIZE: Layout();return 0;
        case WM_CLOSE:
            if(conversionPending && MessageBoxW(window,
               L"有一条凭据转换正在等待连接确认。关闭程序后，两条凭据都会保留。\r\n\r\n确定关闭吗？",
               L"确认退出",MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2)!=IDYES) return 0;
            return DefWindowProcW(window,message,wParam,lParam);
        case WM_VSCROLL: {
            SCROLLINFO si{sizeof(si),SIF_ALL,0,0,0,0,0};
            GetScrollInfo(window,SB_VERT,&si);
            int next=scrollY;
            switch((int)(wParam&0xffff)) {
                case SB_LINEUP:next-=40;break;
                case SB_LINEDOWN:next+=40;break;
                case SB_PAGEUP:next-=(int)si.nPage;break;
                case SB_PAGEDOWN:next+=(int)si.nPage;break;
                case SB_THUMBPOSITION:case SB_THUMBTRACK:next=si.nTrackPos;break;
                case SB_TOP:next=0;break;
                case SB_BOTTOM:next=geometry.canvasHeight;break;
            }
            ScrollTo(next);return 0;
        }
        case WM_MOUSEWHEEL: {
            short ticks=(short)((wParam>>16)&0xffff);
            ScrollTo(scrollY-(int)ticks*65/120);return 0;
        }
        case WM_DRAWITEM:
            PaintButton((DRAWITEMSTRUCT*)lParam);return 1;
        case WM_GETMINMAXINFO: {
            MINMAXINFO* min=(MINMAXINFO*)lParam;
            int maxW=GetSystemMetrics(SM_CXSCREEN)-S(36);
            int maxH=GetSystemMetrics(SM_CYSCREEN)-S(36);
            min->ptMinTrackSize.x=S(560)<maxW?S(560):maxW;
            min->ptMinTrackSize.y=S(390)<maxH?S(390):maxH;
            return 0;
        }
        case WM_DPICHANGED: {
            dpi=(int)(wParam&0xffff);if(!dpi) dpi=96;
            RECT* suggested=(RECT*)lParam;
            SetWindowPos(window,nullptr,suggested->left,suggested->top,
                         suggested->right-suggested->left,suggested->bottom-suggested->top,
                         SWP_NOZORDER|SWP_NOACTIVATE);
            Fonts();Layout();return 0;
        }
        case WM_NOTIFY: {
            NMHDR* nm=(NMHDR*)lParam;
            if(nm && nm->idFrom==ID_FAVORITES_LIST && !uiUpdating) {
                if(nm->code==LVN_ITEMCHANGED) {
                    int index=FavoriteSelectionFromList();
                    if(index!=selectedFavorite) SelectFavorite(index);
                } else if(nm->code==NM_DBLCLK && selectedFavorite>=0) ConnectFavorite();
            }
            if(nm && nm->idFrom==ID_LIST && !uiUpdating) {
                if(nm->code==LVN_ITEMCHANGED) {
                    int index=SelectedFromList();
                    if(index>=0 && index!=selected) Select(index);
                    else if(index<0 && selected>=0) {
                        selected=-1;UpdateActions();
                        Status(L"未选中任何列表记录，可继续编辑右侧内容或新建凭据。");
                    }
                } else if(nm->code==NM_DBLCLK && selected>=0) Connect();
            }
            return 0;
        }
        case WM_COMMAND: {
            int id=(int)(wParam&0xffff),event=(int)((wParam>>16)&0xffff);
            if(id==ID_TAB_FAVORITES) ChangeTab(true);
            else if(id==ID_TAB_CREDENTIALS) ChangeTab(false);
            else if(id==ID_TOGGLE_FAVORITE) ToggleFavorite();
            else if(id==ID_FAVORITES_CONNECT) ConnectFavorite();
            else if(id==ID_FAVORITES_REMOVE) RemoveFavorite();
            else if(id==ID_FAVORITES_SAVE) SaveFavoriteDetails();
            else if(id==ID_FAVORITES_EDIT) EditFavoriteCredential();
            else if(id==ID_FAVORITES_SEARCH && event==EN_CHANGE && !uiUpdating) RefreshFavorites(true);
            else if(id==ID_NEW) ClearEditor();
            else if(id==ID_REFRESH) Refresh();
            else if(id==ID_SAVE) Save(false);
            else if(id==ID_CONVERT) Save(true);
            else if(id==ID_CLEANUP) Cleanup();
            else if(id==ID_CANCEL) CancelConversion();
            else if(id==ID_CONNECT) Connect();
            else if(id==ID_DELETE) DeleteSelected();
            else if(id==ID_FILTER && event==CBN_SELCHANGE) FilterRows();
            else if(id==ID_SEARCH && event==EN_CHANGE && !uiUpdating) FilterRows();
            else if((id==ID_HOST||id==ID_PORT) && event==EN_CHANGE && !uiUpdating) PreviewTarget();
            else if(id==ID_MODE && event==CBN_SELCHANGE && !uiUpdating) PreviewTarget();
            return 0;
        }
        case WM_CTLCOLORSTATIC: {
            HANDLE dc=(HANDLE)wParam;HWND h=(HWND)lParam;
            int id=GetDlgCtrlID(h);
            SetBkMode(dc,TRANSPARENT);
            if(id==ID_TITLE || id==ID_SUBTITLE) {
                SetTextColor(dc,UI_WHITE);return (LRESULT)navyBrush;
            }
            SetTextColor(dc,id==ID_PREVIEW||id==ID_HELP||id==ID_STATUS||id==ID_FAVORITES_HINT?UI_MUTED:UI_TITLE);
            if(id==ID_COUNT) return (LRESULT)softBrush;
            if(id==ID_STATUS) return (LRESULT)bgBrush;
            return (LRESULT)whiteBrush;
        }
        case WM_ERASEBKGND: return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps{};HANDLE dc=BeginPaint(window,&ps);
            RECT rc{};GetClientRect(window,&rc);
            FillRect(dc,&rc,bgBrush);
            RECT header{0,-S(scrollY),rc.right,S(94-scrollY)};
            FillRect(dc,&header,navyBrush);
            Card(dc,geometry.summary,UI_SOFT,UI_SOFT,16);
            Card(dc,geometry.leftCard,UI_WHITE,UI_BORDER,18);
            Card(dc,geometry.rightCard,UI_WHITE,UI_BORDER,18);
            EndPaint(window,&ps);return 0;
        }
        case WM_DESTROY:
            SetText(passBox,L"");
            if(regular) DeleteObject(regular);if(strong) DeleteObject(strong);
            if(large) DeleteObject(large);if(small) DeleteObject(small);
            if(whiteBrush) DeleteObject(whiteBrush);if(navyBrush) DeleteObject(navyBrush);
            if(bgBrush) DeleteObject(bgBrush);
            if(softBrush) DeleteObject(softBrush);
            if(applicationIcon) DestroyIcon(applicationIcon);
            if(smallIcon) DestroyIcon(smallIcon);
            PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(window,message,wParam,lParam);
}
// This isolated Windows CI integration mode runs inside the GUI process, so
// it can exercise native input controls without remote-process UI privileges.
// It uses uniquely named disposable credentials and never starts an RDP session.
int NativeGuiSelfTest() {
    headlessSelfTest=true;
    if(!IsWindowEnabled(saveBtn) || !IsWindowEnabled(portBox)) return 30;
    wchar_t host[260]{},portText[16]{};
    GetWindowTextW(hostBox,host,260);GetWindowTextW(portBox,portText,16);
    if(host[0] || portText[0]) return 31;
    wchar_t pid[20]{};core::FormatUnsigned(GetCurrentProcessId(),pid,20);
    wchar_t server[100]=L"rdpcm-gui-ci-";
    core::Append(server,100,pid);core::Append(server,100,L".invalid");
    wchar_t full[520]{},bare[520]{};
    core::BuildTargetWithPort(server,3721,true,full,520);
    core::BuildTargetWithPort(server,3389,false,bare,520);
    SetText(hostBox,server);SetText(portBox,L"3721");
    SetText(userBox,L"CI\\DisposableUser");
    SetText(passBox,L"disposable-ci-test-password");
    SendMessageW(modeBox,CB_SETCURSEL,1,0);
    Save(false);
    int result=32;
    CREDENTIALW* record=nullptr;
    if(CredReadW(full,CRED_TYPE_GENERIC,0,&record)) {
        bool correct=record->UserName &&
            core::EqualInsensitive(record->UserName,L"CI\\DisposableUser");
        CredFree(record);
        if(correct && selected>=0 && items[selected].type==CRED_TYPE_GENERIC &&
            core::SameTarget(items[selected].target,full)) {
            wchar_t editedPort[16]{};GetWindowTextW(portBox,editedPort,16);
            if(core::EqualInsensitive(editedPort,L"3721")) {
                SetText(portBox,L"3389");
                SendMessageW(modeBox,CB_SETCURSEL,0,0);
                Save(false);
                if(CredReadW(bare,CRED_TYPE_GENERIC,0,&record)) {
                    CredFree(record);
                    if(selected>=0 && items[selected].type==CRED_TYPE_GENERIC &&
                       core::SameTarget(items[selected].target,bare) &&
                       CredReadW(full,CRED_TYPE_GENERIC,0,&record)) {
                        CredFree(record);
                        result=0;
                    } else result=34;
                } else result=33;
            }
        }
    }
    SetText(passBox,L"");
    // Isolated registry key is used by self-test; real user favorites are untouched.
    if(result==0) {
        int credential=core::FindByTargetAndType(items,count,bare,CRED_TYPE_GENERIC);
        if(credential<0) result=40;
        else {
            selected=credential;
            ToggleFavorite();
            if(favoriteCount!=1 || !favoriteStoreWritable) result=41;
            else {
                selectedFavorite=0;
                SetText(favoritesName,L"集成测试会话");SetText(favoritesPort,L"3721");
                SaveFavoriteDetails();
                if(favoriteCount!=1 || bookmarks.entries[0].port!=3721 ||
                   !core::EqualInsensitive(bookmarks.entries[0].alias,L"集成测试会话")) result=42;
                else {
                    LoadFavorites();
                    if(favoriteCount!=1 || bookmarks.entries[0].port!=3721) result=43;
                    else {
                        favoritesTab=(favoriteCount>0);
                        if(!favoritesTab) result=46;
                        CredDeleteW(full,CRED_TYPE_GENERIC,0);
                        CredDeleteW(bare,CRED_TYPE_GENERIC,0);
                        Refresh(false);
                        if(FavoriteCredentialIndex(bookmarks.entries[0])>=0) result=48;
                        favorites::Remove(bookmarks.entries,&favoriteCount,0);
                        if(!PersistFavorites()) result=44;
                        else {
                            LoadFavorites();
                            if(favoriteCount!=0) result=45;
                            else {favoritesTab=favoriteCount>0;if(favoritesTab) result=47;}
                        }
                    }
                }
            }
        }
    }
    CredDeleteW(full,CRED_TYPE_GENERIC,0);
    CredDeleteW(bare,CRED_TYPE_GENERIC,0);
    HKEY testKey=nullptr;
    if(RegOpenKeyExW(FavoritesRoot(),FavoritesPath(),0,KEY_QUERY_VALUE,&testKey)==0) {
        RegCloseKey(testKey);
        RegDeleteKeyW(FavoritesRoot(),FavoritesPath());
    }
    return result;
}
bool RequestedSelfTest() {
    const wchar_t* line=GetCommandLineW();
    if(!line) return false;
    unsigned length=core::Length(line);
    return length>=12 && line[length-12]==L' ' &&
           core::EqualInsensitive(line+length-11,L"--self-test");
}
bool RequestedInitTest() {
    const wchar_t* line=GetCommandLineW();
    const wchar_t* suffix=L" --self-test-init";
    if(!line) return false;
    unsigned length=core::Length(line), suffixLength=core::Length(suffix);
    return length>=suffixLength && core::EqualInsensitive(line+length-suffixLength,suffix);
}
int Run() {
    application=GetModuleHandleW(nullptr);
    headlessSelfTest=RequestedSelfTest() || RequestedInitTest();
    SetProcessDpiAwarenessContext((HANDLE)(LONG_PTR)-4);
    INITCOMMONCONTROLSEX icc{sizeof(INITCOMMONCONTROLSEX),ICC_LISTVIEW_CLASSES};
    if(!InitCommonControlsEx(&icc)) return 1;
    WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.lpfnWndProc=WindowProc;wc.hInstance=application;
    wc.hCursor=LoadCursorW(nullptr,(LPCWSTR)(ULONG_PTR)32512);
    wchar_t iconSource[340]={};
    UINT dirLength=GetSystemDirectoryW(iconSource,340);
    if(dirLength && dirLength+12<340 && core::Append(iconSource,340,L"\\mstsc.exe"))
        ExtractIconExW(iconSource,0,&applicationIcon,&smallIcon,1);
    wc.hIcon=applicationIcon;wc.hIconSm=smallIcon;
    wc.hbrBackground=(HBRUSH)(ULONG_PTR)(COLOR_WINDOW+1);
    wc.lpszClassName=L"RDPCredManagerPro";
    if(!RegisterClassExW(&wc)) return 2;
    RECT work={0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN)};
    SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);
    UINT screenDpi=GetDpiForSystem();if(!screenDpi) screenDpi=96;
    int workW=(int)(work.right-work.left),workH=(int)(work.bottom-work.top);
    int winW=(int)(1160*screenDpi/96),winH=(int)(920*screenDpi/96);
    if(winW>workW-24) winW=workW-24;
    if(winH>workH-24) winH=workH-24;
    HWND window=CreateWindowExW(0,wc.lpszClassName,L"RDP Credential Manager 4.2",
        WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_VSCROLL,
        work.left+(workW-winW)/2,work.top+(workH-winH)/2,winW,winH,
        nullptr,nullptr,application,nullptr);
    if(!window) return 3;
    if(RequestedInitTest()) return 0;
    if(RequestedSelfTest()) return NativeGuiSelfTest();
    ShowWindow(window,SW_SHOW);UpdateWindow(window);
    MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0) {
        if(!IsDialogMessageW(window,&msg)) {TranslateMessage(&msg);DispatchMessageW(&msg);}
    }
    return (int)msg.wParam;
}
} // namespace
extern "C" void __cdecl WinMainCRTStartup() {ExitProcess((UINT)Run());}
extern "C" void* __cdecl memcpy(void* dst,const void* src,decltype(sizeof(0)) size) {
    volatile BYTE* d=(volatile BYTE*)dst;
    const volatile BYTE* s=(const volatile BYTE*)src;
    for(decltype(sizeof(0)) i=0;i<size;++i) d[i]=s[i];
    return dst;
}
extern "C" void* __cdecl memset(void* dst,int value,decltype(sizeof(0)) size) {
    volatile BYTE* d=(volatile BYTE*)dst;
    for(decltype(sizeof(0)) i=0;i<size;++i) d[i]=(BYTE)value;
    return dst;
}
