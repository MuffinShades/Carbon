#include <iostream>

#define RECT_VERTS(x, y, w, h, ad) {\
    (x), (y), ad, (x), (y+h), ad, (x+w), (y), ad, \
    (x+w), (y+h), ad, (x+w), (y), ad, (x), (y+h), ad \
  }

#define RECT_VERTS_EX(x, y, w, h, tl_ad, tr_ad, br_ad, bl_ad) {\
    (x), (y), tl_ad, (x), (y+h), bl_ad, (x+w), (y), tr_ad, \
    (x+w), (y+h), br_ad, (x+w), (y), tr_ad, (x), (y+h), bl_ad \
  }

#define RECT_STROKE_VERTS(x, y, w, h, thickness)

#define RECT_VERT_TL 0
#define RECT_VERT_TR 1

namespace Geo {
class Rect {
public:
    
};
};