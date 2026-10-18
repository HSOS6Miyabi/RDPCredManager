#include "../src/layout.h"
#include <initializer_list>
#include <cassert>
int main(){
    for(int w: {540,640,760,899,900,960,1024,1366,1600}) for(int h: {320,430,540,700,900,1200}){
        auto g=layout::Calculate(w,h);
        assert(g.stacked==(w<900));
        assert(g.canvasHeight>=h);
        assert(g.title.x>=0 && g.title.y>=0);
        assert(g.leftCard.x>=0 && g.leftCard.x+g.leftCard.w<=w);
        assert(g.rightCard.x>=0 && g.rightCard.x+g.rightCard.w<=w);
        assert(g.list.w>100 && g.list.h>0);
        assert(g.host.w>100 && g.port.w>40);
        assert(g.save.w>90 && g.convert.w>90);
        assert(g.cleanup.y+g.cleanup.h+14<=g.rightCard.y+g.rightCard.h);
        assert(g.cleanup.y+g.cleanup.h<g.status.y);
        assert(g.cancel.y+g.cancel.h<g.status.y);
        assert(g.editorHeading.y>=g.rightCard.y);
        assert(g.help.y+g.help.h<g.save.y);
        assert(g.list.y+g.list.h<g.connect.y);
        assert(g.list.x>=g.leftCard.x && g.list.x+g.list.w<=g.leftCard.x+g.leftCard.w);
        assert(g.connect.y+g.connect.h<g.leftCard.y+g.leftCard.h);
        assert(g.host.x>=g.rightCard.x && g.port.x+g.port.w<=g.rightCard.x+g.rightCard.w);
        assert(g.mode.y<g.preview.y);
        for(int scale: {96,120,144,192,240,288}) {
            auto physical=[](int v,int dpi){return (v*dpi+48)/96;};
            assert(physical(g.list.w,scale)>0);
            assert(physical(g.rightCard.x+g.rightCard.w,scale)<=physical(w,scale)+1);
            assert(physical(g.canvasHeight,scale)>=physical(h,scale));
        }
        assert(layout::ClampScroll(9000,h,g.canvasHeight)==g.canvasHeight-h);
        assert(layout::ClampScroll(-25,h,g.canvasHeight)==0);
    }
}
