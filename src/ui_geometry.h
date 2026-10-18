#pragma once
#include "layout.h"

namespace ui_geometry {
struct TabStrip {
    layout::Rectangle favorites;
    layout::Rectangle credentials;
    layout::Rectangle summary;
    bool showSummary;
};
inline TabStrip Tabs(const layout::Geometry& g) {
    const int x=g.summary.x+14;
    const int y=g.summary.y+10;
    const int available=g.summary.w-28;
    const int gap=14;
    // Both tab labels have enough room even when the window is narrow.
    if(available<665) {
        const int first=(available-gap)/2;
        return {{x,y,first,38},{x+first+gap,y,available-first-gap,38},
                {0,0,0,0},false};
    }
    const int first=216, second=232;
    return {{x,y,first,38},{x+first+gap,y,second,38},
            {x+first+gap+second+20,y+2,available-first-gap-second-20,34},true};
}
struct Details {
    layout::Rectangle heading;
    layout::Rectangle star;
    layout::Rectangle info;
    layout::Rectangle hint;
};
inline Details Editor(const layout::Geometry& g) {
    const auto h=g.editorHeading;
    constexpr int button=196, gap=14;
    const layout::Rectangle heading={h.x,h.y,h.w-button-gap,h.h};
    const layout::Rectangle star={h.x+h.w-button,h.y,button,32};
    // Detail text is scrollable and uses all remaining space above footer buttons.
    const int infoTop=h.y+242;
    const int hintTop=g.save.y-88;
    const layout::Rectangle info={h.x,infoTop,h.w,hintTop-infoTop-12};
    const layout::Rectangle hint={h.x,hintTop,h.w,72};
    return {heading,star,info,hint};
}
inline bool Overlaps(layout::Rectangle a,layout::Rectangle b) {
    return a.x<b.x+b.w && b.x<a.x+a.w && a.y<b.y+b.h && b.y<a.y+a.h;
}
}
