#include "../src/ui_geometry.h"
#include <cassert>
#include <initializer_list>
int main() {
    for (int width : {540,600,760,899,900,960,1280,1600,1920}) {
        for (int height : {320,540,720,920,1200,1600}) {
            const auto layout=layout::Calculate(width,height);
            const auto tabs=ui_geometry::Tabs(layout);
            const auto e=ui_geometry::Editor(layout);
            assert(tabs.favorites.w>=200 && tabs.credentials.w>=200);
            assert(!ui_geometry::Overlaps(tabs.favorites,tabs.credentials));
            assert(tabs.favorites.x>=layout.summary.x);
            assert(tabs.credentials.x+tabs.credentials.w<=layout.summary.x+layout.summary.w);
            if(tabs.showSummary) {
                assert(tabs.summary.w>=100);
                assert(!ui_geometry::Overlaps(tabs.credentials,tabs.summary));
            }
            assert(e.heading.w>=175 && e.star.w>=190);
            assert(!ui_geometry::Overlaps(e.heading,e.star));
            assert(!ui_geometry::Overlaps(e.star,layout.hostLabel));
            assert(!ui_geometry::Overlaps(e.star,layout.portLabel));
            assert(e.info.h>=105);
            assert(e.info.y>=layout.port.y+layout.port.h);
            assert(!ui_geometry::Overlaps(e.info,e.hint));
            assert(!ui_geometry::Overlaps(e.info,layout.save));
            assert(e.hint.y+e.hint.h<layout.save.y);
            for (int dpi : {96,120,144,192,240,288}) {
                auto scale=[dpi](int a){return (a*dpi+48)/96;};
                assert(scale(tabs.favorites.w)>=scale(200));
                assert(scale(e.info.h)>=scale(105));
                assert(scale(e.star.y+e.star.h)<=scale(layout.hostLabel.y));
            }
        }
    }
}
