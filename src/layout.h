#pragma once
// Logical 96-DPI coordinates. Side-by-side on desktop, stacked on narrow windows.
namespace layout {
struct Rectangle { int x,y,w,h; };
struct Geometry {
    bool stacked;
    int canvasHeight;
    Rectangle leftCard,rightCard;
    Rectangle title,subtitle,summary;
    Rectangle listHeading,searchLabel,search,filter,list,connect,remove;
    Rectangle editorHeading,hostLabel,portLabel,host,port;
    Rectangle userLabel,user,passLabel,pass,targetLabel,mode,preview,help;
    Rectangle save,convert,cleanup,cancel,status;
    Rectangle newButton,refreshButton;
};
inline Geometry Calculate(int width,int height) {
    if(width<540) width=540;
    if(height<320) height=320;
    Geometry g{};
    g.stacked=width<900;
    const int margin=24,gap=18;
    g.canvasHeight = g.stacked ? (height>1340?height:1340) : (height>850?height:850);
    int cw=g.canvasHeight;
    g.title={margin,15,width-360,36};
    g.subtitle={margin,58,width-350,22};
    g.newButton=g.stacked?Rectangle{margin,98,158,36}:Rectangle{width-294,24,126,38};
    g.refreshButton=g.stacked?Rectangle{margin+172,98,145,36}:Rectangle{width-156,24,132,38};
    g.summary={margin,g.stacked?150:113,width-2*margin,58};
    int leftx=margin,ly=g.stacked?220:190,lw=0,rx=0,ry=0,rw=0;
    if(g.stacked) {
        lw=width-2*margin;rx=margin;rw=lw;ry=660;
        g.leftCard={margin,ly,lw,422};
        g.rightCard={margin,ry,rw,cw-722};
    } else {
        lw=((width-2*margin-gap)*53)/100;
        rx=margin+lw+gap;rw=width-margin-rx;ry=190;
        g.leftCard={margin,190,lw,cw-250};
        g.rightCard={rx,190,rw,cw-250};
    }
    const int lm=20,rm=20;
    g.listHeading={leftx+lm,ly+20,lw-2*lm,31};
    g.searchLabel={leftx+lm,ly+73,52,24};
    int searchWidth=(lw-2*lm)*54/100;
    g.search={leftx+lm+55,ly+69,searchWidth-55,35};
    g.filter={leftx+lm+searchWidth+10,ly+69,lw-2*lm-searchWidth-10,160};
    int actionY=g.stacked?ly+356:cw-135;
    int listBottom=actionY-16;
    g.list={leftx+lm,ly+118,lw-2*lm,listBottom-(ly+118)};
    int actionW=(lw-2*lm-12)/2;
    g.connect={leftx+lm,actionY,actionW,40};
    g.remove={leftx+lm+actionW+12,actionY,actionW,40};
    g.editorHeading={rx+rm,ry+20,rw-2*rm,31};
    int hy=ry+77;
    int editorWidth=rw-2*rm;
    int hostWidth=(editorWidth-12)*72/100;
    g.hostLabel={rx+rm,hy,hostWidth,22};
    g.portLabel={rx+rm+hostWidth+12,hy,editorWidth-hostWidth-12,22};
    g.host={rx+rm,hy+24,hostWidth,36};
    g.port={rx+rm+hostWidth+12,hy+24,editorWidth-hostWidth-12,36};
    g.userLabel={rx+rm,hy+76,editorWidth,22};
    g.user={rx+rm,hy+100,editorWidth,36};
    g.passLabel={rx+rm,hy+152,editorWidth,22};
    g.pass={rx+rm,hy+176,editorWidth,36};
    g.targetLabel={rx+rm,hy+226,editorWidth,22};
    g.mode={rx+rm,hy+251,editorWidth,180};
    g.preview={rx+rm,hy+296,editorWidth,40};
    g.help={rx+rm,hy+340,editorWidth,55};
    int firstY=g.stacked?cw-185:cw-169;
    int btnW=(editorWidth-12)/2;
    g.save={rx+rm,firstY,btnW,40};
    g.convert={rx+rm+btnW+12,firstY,btnW,40};
    g.cleanup={rx+rm,firstY+50,btnW,40};
    g.cancel={rx+rm+btnW+12,firstY+50,btnW,40};
    g.status={margin,cw-41,width-2*margin,25};
    return g;
}
inline int ClampScroll(int offset,int viewport,int canvas) {
    const int maximum=canvas>viewport?canvas-viewport:0;
    return offset<0?0:(offset>maximum?maximum:offset);
}
}
