#include "ttf_render.hpp"
#include "msutil.hpp"
#include "bitmap_render.hpp"
#include "logger.hpp"
#include <vector>

#define MSFL_TTFRENDER_DEBUG

/**
 *
 * All le code for rendering dem bezier curves
 *
 * Written by muffinshades 2024
 *
 */

Point pLerp(Point p0, Point p1, float t) {
    return {
        lerp(p0.x, p1.x, t),
        lerp(p0.y, p1.y, t)
    };
}

Point bezier3(Point p0, Point p1, Point p2, float t) {
    Point i0 = pLerp(p0, p1, t),
        i1 = pLerp(p1, p2, t);

    return pLerp(i0, i1, t);
}

Point bezier4(Point p0, Point p1, Point p2, Point p3, float t) {
    Point i0 = pLerp(p0, p1, t),
        i1 = pLerp(p1, p2, t),
        i2 = pLerp(p2, p3, t),
        ii0 = pLerp(i0, i1, t),
        ii1 = pLerp(i1, i2, t);

    return pLerp(ii0, ii1, t);
}

#include <cassert>

Point bezier(std::vector<Point> points, float t) {
    assert(points.size() >= 3);
    std::vector<Point>* i = new std::vector<Point>();
    std::vector<Point>* tg = &points;
    size_t itr = 0;
    do {
        const size_t tlp = tg->size() - 1;
        for (size_t p = 0; p < tlp; p++)
            i->push_back(
                pLerp((*tg)[p], (*tg)[p + 1], t)
            );
        if (itr > 0)
            delete tg;
        tg = i;
        i = new std::vector<Point>();
        itr++;
    } while (tg->size() > 1);

    Point r = (*tg)[0];
    delete tg, i;
    return r;
};

Point ScalePoint(Point p, float s) {
    return {
        p.x * s,
        p.y * s
    };
}

void DrawPoint(BitmapGraphics* g, float x, float y) {
    float xf = floor(x), xc = ceil(x),
        yf = floor(y), yc = ceil(y);

    float cIx = xc - x, fIx = x - xf,
        cIy = yc - y, fIy = y - yf;

    float i = ((cIx + fIx) / 2.0f + (cIy + fIy) / 2.0f) / 2.0f;

    i *= 255.0f;

    fIx *= 255.0f;
    fIy *= 255.0f;

    g->SetColor(fIx, fIx, fIx, 255);
    g->DrawPixel(x - 1, y);

    g->SetColor(cIx, cIx, cIx, 255);
    g->DrawPixel(x, y);
}

constexpr float epsilon = 0.0001f;
constexpr float invEpsilon = 1.0f / epsilon;
#define EPSILIZE(v) (floor((v)*invEpsilon)*epsilon)

struct f_roots {
    float r0 = 0.0f, r1 = 0.0f;
    i32 nRoots = 0;
};

/**
 *
 * getRoots
 *
 * returns the number of roots a
 * quadratic function has and their
 * values
 *
 */
f_roots getRoots(float a, float b, float c) {
    if (EPSILIZE(a) == 0.0f)
        return {
            .nRoots = 0
    };
    float root = EPSILIZE(
        (b * b) - (4.0f * a * c)
    ), ida = 1.0f / (2.0f * a);
    if (root < 0.0f)
        return {
            .nRoots = 0
    };
    root = EPSILIZE(sqrtf(root));
    if (root != 0.0f)
        return {
            .r0 = (-b + root) * ida,
            .r1 = (-b - root) * ida,
            .nRoots = 2
    };
    else
        return {
            .r0 = (-b + root) * ida,
            .nRoots = 1
    };
}

//verifys a given root is a valid intersection
bool _vRoot(float r, Point p0, Point p1, Point p2, Point e) {
    return r >= 0.0f && r <= 1.0f && bezier3(p0, p1, p2, r).x > e.x;
};

/**
 *
 * intersectsCurve
 *
 * function to determine if a given point, e
 * intersects a 3-points bezier curve denoted
 * by the points p0, p1, and p2
 *
 * Returns the # of interesctions that were made
 * 0, 1, or 2
 *
 */
i32 intersectsCurve(Point p0, Point p1, Point p2, Point e) {

    //offset points
    p0.y -= e.y;
    p1.y -= e.y;
    p2.y -= e.y;

    const float a = (p0.y - 2 * p1.y) + p2.y, b = 2 * p1.y - 2 * p0.y, c = p0.y;

    f_roots _roots = getRoots(a, b, c);

    if (_roots.nRoots <= 0) return 0; //no roots so no intersection

    //check le roots
    i32 nRoots = 0;

    if (_vRoot(_roots.r0, p0, p1, p2, e)) nRoots++;
    if (_roots.nRoots > 1 && _vRoot(_roots.r1, p0, p1, p2, e)) nRoots++;

    return nRoots;
}

struct gPData {
    std::vector<Point> p;
    std::vector<i32> f;
};

/**
 *
 * cleanGlyphPoints
 *
 * takes the raw points from a Glyph and
 * adds the implied points and contour ends
 * cleaning up the glyph making it much easier
 * to do the rendering.
 *
 */
gPData cleanGlyphPoints(Glyph tGlyph) {
    gPData res;

    size_t currentContour = 0;

    //first add implied points
    for (size_t i = 0; i < tGlyph.nPoints; i++) {
        res.p.push_back(tGlyph.points[i]);
        res.f.push_back(tGlyph.flags[i]);

        i32 pFlag = tGlyph.flags[i];
        Point p = tGlyph.points[i];

        if (i == tGlyph.contourEnds[currentContour] || i >= tGlyph.nPoints - 1) {
            size_t cPos = (currentContour > 0) ? tGlyph.contourEnds[currentContour - 1] + 1 : 0;
            assert(cPos < tGlyph.nPoints);

            //check for an implied point
            i32 flg = tGlyph.flags[cPos];
            bool oc = GetFlagValue(flg, PointFlag_onCurve);

            //add implied point in-between if needed
            if (oc == GetFlagValue(pFlag, PointFlag_onCurve)) {
                res.p.push_back(pLerp(p, tGlyph.points[cPos], 0.5f));
                res.f.push_back(ModifyFlagValue(pFlag, PointFlag_onCurve, !oc));
            }

            //add point
            res.p.push_back(tGlyph.points[cPos]);
            res.f.push_back(flg);

            currentContour++;
#ifdef MSFL_TTFRENDER_DEBUG
            std::cout << "Finished Contour: " << currentContour << " / " << tGlyph.nContours << std::endl;
#endif
            continue;
        }

        u32 oCurve;

        if (
            i < tGlyph.nPoints - 1 &&
            (oCurve = GetFlagValue(pFlag, PointFlag_onCurve)) == GetFlagValue(tGlyph.flags[i + 1], PointFlag_onCurve)
            ) {
            //add implied point
            res.p.push_back(pLerp(p, tGlyph.points[i + 1], 0.5f));
            res.f.push_back(ModifyFlagValue(pFlag, PointFlag_onCurve, !oCurve));
        }
    }

    return res;
}

/**
 *
 * ttfRender::RenderGlyphToBitmap
 *
 * renders a given ttfGlyph to a bitmap with
 * the r, g, b, a values being multipliers to
 * whatever given color you want to render the
 * text as
 *
 */
i32 ttfRender::RenderGlyphToBitmap(Glyph tGlyph, Bitmap* bmp, float scale) {
    i32 mapW = tGlyph.xMax - tGlyph.xMin,
        mapH = tGlyph.yMax - tGlyph.yMin;

    if (scale <= 0.0f)
        return 1;

    mapW *= scale;
    mapH *= scale;

    mapW++;
    mapH++;

    bmp->header.w = mapW;
    bmp->header.h = mapH;
    bmp->data = new byte[mapW * mapH * sizeof(u32)];
    bmp->header.fSz = mapW * mapH * sizeof(u32);
    bmp->header.bitsPerPixel = 32;

    ZeroMem(bmp->data, bmp->header.fSz);

    //render the glyph
    //gonna render white for now

    size_t onCurve = 0, offCurve = 0;

    const float nSteps = 150.0f, invStep = 1.0f / nSteps;

    BitmapGraphics g(bmp);

    gPData cleanDat = cleanGlyphPoints(tGlyph);
    std::vector<Point> fPoints = cleanDat.p;
    std::vector<i32> fFlags = cleanDat.f;

#ifdef MSFL_TTFRENDER_DEBUG
    srand(time(NULL));
#endif

    struct _curve {
        Point p0, p1, p2;
    };

    std::vector<_curve> bCurves;

    for (size_t i = 0; i < fPoints.size(); i++) {
        i32 pFlag = fFlags[i];
        Point p = fPoints[i];

        if ((bool)GetFlagValue(pFlag, PointFlag_onCurve)) {
#ifdef MSFL_TTFRENDER_DEBUG
            g.SetColor(255, 255, 255, 255);
            if (i == 0)
                g.SetColor(255, 255, 0, 255);
            g.DrawPixel(p.x * scale - tGlyph.xMin * scale, p.y * scale - tGlyph.yMin * scale);
#endif

            if (onCurve == 0 && offCurve == 0) {
                onCurve++;
                continue;
            }

            //there shouldnt be a stright line so...
            if (offCurve == 0)
                continue;

            g.SetColor(255.0f, 255.0f, 255.0f, 255.0f);

            //else draw le curve
            for (float t = 0.0f; t <= 1.0f; t += invStep) {
                const _curve currentCurve = {
                    .p0 = ScalePoint(fPoints[i - 2], scale),
                    .p1 = ScalePoint(fPoints[i - 1], scale),
                    .p2 = ScalePoint(p, scale)
                };

                Point np = bezier3(
                    currentCurve.p0,
                    currentCurve.p1,
                    currentCurve.p2,
                    t
                );

                //add a curve
                bCurves.push_back(currentCurve);

                //g.SetColor((np.x / (float)mapW) * 255.0f, (np.y / (float)mapH) * 255.0f, 255, 255);
                //DrawPoint(&g, np.x - tGlyph.xMin * scale, np.y - tGlyph.yMin * scale);
                //g.SetColor(128.0f, 128.0f, 128.0f, 255.0f);
                //g.DrawPixel(np.x - tGlyph.xMin * scale, np.y - tGlyph.yMin * scale);
            }

            offCurve = onCurve = 0;
        }
        else {
            offCurve++;
#ifdef MSFL_TTFRENDER_DEBUG
            g.SetColor((i / (float)fPoints.size()) * 255.0f, 0.0f, 255.0f - ((i / (float)fPoints.size()) * 255.0f), 255);
            g.DrawPixel(p.x * scale, p.y * scale);
#endif
        }
    }

    const float _Tx = -tGlyph.xMin * scale, _Ty = -tGlyph.yMin * scale;

    //try just intersting over all the pixels
    /*g.SetColor(255, 255, 255, 255);
    for (float y = 0; y < mapH; y++) {
        for (float x = 0; x < mapW; x++) {
            i32 i = 0;
            //intersection thingy
            for (auto& c : bCurves)
                i += intersectsCurve(c.p0, c.p1, c.p2, { x, y });

            if ((i & 1) != 0)
                g.DrawPixel(x + _Tx, y + _Ty);
        }
    }*/

    return 0;
}

f32 pointCross(Point a, Point b) {
    return a.x * b.y - a.y * b.x;
}

struct BCurve {
    Point p[3];
};

struct Edge {
    BCurve *curves = nullptr;
    size_t nCurves = 0;
};

struct PDistInfo {
    f32 dx = INFINITY,dy = INFINITY,d = INFINITY,t = 0.0f;
    BCurve curve;
};

const PDistInfo pSquareDist(Point p0, Point p1) {
    PDistInfo inf;
    inf.dx = p1.x - p0.x;
    inf.dy = p1.y - p0.y;
    inf.d = inf.dx*inf.dx + inf.dy*inf.dy;
    return inf;
}



//derivative of a bezier 3
const Point dBdt3(Point p0, Point p1, Point p2, f32 t) {
    return {
        .x = 2.0f * ((1.0f - t) * (p1.x - p0.x) + t * (p2.x - p1.x)),
        .y = 2.0f * ((1.0f - t) * (p1.y - p0.y) + t * (p2.y - p1.y))
    };
}

PDistInfo EdgePointSignedDist(Point p, Edge e) {
    if (!e.curves || e.nCurves == 0) return {.d=INFINITY};

    constexpr size_t nCheckSteps = 256;
    constexpr f64 dt = 1.0f / (f32) nCheckSteps;

    PDistInfo d = {
        .d = INFINITY
    }, pd;

    Point refPoint;

    i32 c;
    f64 t;

    BCurve tCurve;

    for (c = 0; c < e.nCurves; c++) {
        tCurve = e.curves[c];
        for (t = 0; t <= 1.0f; t += dt) {
            refPoint = bezier3(tCurve.p[0],tCurve.p[1],tCurve.p[2],t);

            pd = pSquareDist(p, refPoint);
            
            if (pd.d < d.d) {
                d = pd;
                d.t = t;
                d.curve = tCurve;
            }
        }
    }

    tCurve = d.curve;

    d.d = mu_sign(pointCross(
        dBdt3(tCurve.p[0],tCurve.p[1],tCurve.p[2],d.t),
        {d.dx,d.dy}
    )) * sqrtf(d.d);

    return d;
}

struct EdgeDistInfo {
    Edge tEdge;
    size_t edgeIdx;
    PDistInfo signedDist;
    u32 dbgVal = 0;
};

Point pointNormalize(Point p) {
    const f32 f = 1.0f / sqrtf(p.x*p.x + p.y*p.y);

    return {
        .x = p.x * f,
        .y = p.y * f
    };
}

//compute orthoganality of a curve at a given point
f32 curveOrtho(BCurve c, Point p, f32 t) {
    const Point b = bezier3(c.p[0], c.p[1], c.p[2], t);
    return abs(pointCross(pointNormalize(
        dBdt3(c.p[0], c.p[1], c.p[2], t)
    ), pointNormalize(
        {p.x-b.x,p.y-b.y}
    )));
}

EdgeDistInfo MinEdgeDist(Point p, std::vector<Edge> edges) {
    EdgeDistInfo inf = {
        .signedDist = INFINITY
    };

    f32 minAbsDist = INFINITY;
    f32 sd, ad;
    PDistInfo sdInf;
    i32 eIdx = 0;

    std::vector<EdgeDistInfo> duplicateEdges;

    for (Edge e : edges) {
        sdInf = EdgePointSignedDist(p, e);
        sd = sdInf.d;
        ad = abs(sd);

        //std::cout << "pds: " << sd << std::endl;

        if (abs(ad - minAbsDist) <= 0.01f) {
            EdgeDistInfo dInf = {
                .tEdge = e,
                .edgeIdx = (size_t) eIdx,
                .signedDist = sdInf
            };

            duplicateEdges.push_back(dInf);
        } else if (ad < minAbsDist) {
            inf.tEdge = e;
            inf.edgeIdx = eIdx;
            inf.signedDist = sdInf;
            minAbsDist = ad;
            duplicateEdges.clear();
        }

        eIdx++;
    }

    const size_t nDuplicates = duplicateEdges.size();

    //if there are duplicate distances then we need to maximize orthogonality between edges
    if (nDuplicates > 0) {
        f32 maxOrtho = curveOrtho(inf.signedDist.curve, p, inf.signedDist.t), eOrtho;

        for (EdgeDistInfo e : duplicateEdges) {
            eOrtho = curveOrtho(e.signedDist.curve, p, e.signedDist.t);

            if (eOrtho > maxOrtho){
                inf = e;
                maxOrtho = eOrtho;
            }
        }
    }

    if (nDuplicates > 0) inf.dbgVal = 1;

    return inf;
}

i32 ttfRender::RenderGlyphSDFToBitMap(Glyph tGlyph, Bitmap* map, sdf_dim size) {
    if (!map)
        return 1;

    //do some dimension calculations
    u32 sdfW = 0, sdfH = 0;

    const f32 glyphW = tGlyph.xMax - tGlyph.xMin,
              glyphH = tGlyph.yMax - tGlyph.yMin,
              glyphYxRatio = glyphH / glyphW;

    switch (size.slc) {
    case sdf_dim_ty::Width:
        map->header.w = (sdfW = size.m.w);
        map->header.h = (sdfH = (size_t) ceil(sdfW * glyphYxRatio));
        break;
    case sdf_dim_ty::Height:
        map->header.h = (sdfH = size.m.h);
        map->header.w = (sdfW = (size_t) ceil(sdfH / glyphYxRatio));
        break;
    case sdf_dim_ty::Scale:
        map->header.h = (sdfH = size.m.scale * glyphH);
        map->header.w = (sdfW = size.m.scale * glyphW);
        break;
    }

    //clean the glyph up
    gPData cleanDat = cleanGlyphPoints(tGlyph);

    //get num points
    const size_t nPoints = cleanDat.p.size();

    map->header.bitsPerPixel = 32;
    map->header.fSz = map->header.h * map->header.w * 4;

    byte *bmpData = new byte[map->header.fSz];
    ZeroMem(bmpData, map->header.fSz);

    //blank glyph so blank sdf
    if (nPoints == 0)
        return 0;

    map->data = bmpData;

    //curve and edge generation, glyph clean up, and more

    std::vector<Edge> glyphEdges;
    
    const size_t nCurves = nPoints >> 1;
    BCurve *curveBuffer = new BCurve[nCurves]; //stores curves of current edge
    ZeroMem(curveBuffer, nCurves);

    BCurve *workingCurve = curveBuffer;

    //generate glyph edges
    i32 i;
    i32 pSelect = 0, nEdgeCurves = 0;
    Point p, nextPoint, prevPoint;
    f32 cross;
    bool workingOnACurve = false, pOnCurve;
    Edge newEdge;

    for (i = 0; i < nPoints - 1; ++i) {
        p = cleanDat.p[i];

        nextPoint = cleanDat.p[i + 1];

        if (i > 0)
            prevPoint = cleanDat.p[i - 1];
        else
            prevPoint = p;

        pOnCurve = GetFlagValue(cleanDat.f[i], PointFlag_onCurve);

        std::cout << pOnCurve << std::endl;

        workingCurve->p[pSelect++] = p; //add point to the working curve

        if (!pOnCurve) continue;

        if (workingOnACurve) {
            nEdgeCurves++;

            cross = pointCross(
                {p.x - prevPoint.x, p.y - prevPoint.y}, 
                {nextPoint.x - p.x, nextPoint.y - p.y}
            );

            //the curves are not on the same edge so contruct final edge
            if (abs(cross) > 0.01f) {
                BCurve *edgeData = new BCurve[nEdgeCurves];
                in_memcpy(edgeData, curveBuffer, sizeof(BCurve) * nEdgeCurves);

                newEdge.curves = edgeData;
                newEdge.nCurves = nEdgeCurves;
                
                glyphEdges.push_back(newEdge);

                workingCurve = curveBuffer;
                nEdgeCurves = 0;
            } else
                workingCurve ++; //go to next working curve

            pSelect = 0;
            workingOnACurve = false;

            if (!GetFlagValue(cleanDat.f[i+1], PointFlag_onCurve))
                i--; //reuse current point for the next curve if the next point isnt on the curve
        } else
            workingOnACurve = true;
    }

    Point final_point = cleanDat.p[nPoints-1];

    //add last point to the curve
    workingCurve->p[pSelect] = cleanDat.p[nPoints - 1];

    //generate final edge
    nEdgeCurves++;
    BCurve *edgeData = new BCurve[nEdgeCurves];
    in_memcpy(edgeData, curveBuffer, sizeof(BCurve) * nEdgeCurves);

    newEdge.curves = edgeData;
    newEdge.nCurves = nEdgeCurves;

    glyphEdges.push_back(newEdge);

    //generate single channel sdf
    i32 x,y;

    const f32
        wc = glyphW / (f32) sdfW,
        hc = glyphH / (f32) sdfH,
        maxPossibleDist = sqrtf(glyphW*glyphW + glyphH*glyphH);

    byte color;

    for (y = 0; y < sdfH; ++y) {
        for (x = 0; x < sdfW; ++x) {
            p.x = (((f32)x) + 0.5f) * wc + tGlyph.xMin;
            p.y = (((f32)y) + 0.5f) * hc + tGlyph.yMin;

            EdgeDistInfo fieldDist = MinEdgeDist(p, glyphEdges);

            color = mu_min(
                mu_max(
                    (fieldDist.signedDist.d / maxPossibleDist) * 128.0f + 127, 0),
            255);

            const size_t mp = (x+y*sdfW) << 2;

            if (mp + 3 >= map->header.fSz)
                continue;

            map->data[mp+0] = color;
            map->data[mp+1] = color;
            map->data[mp+2] = color;
            map->data[mp+3] = 255;
        }
    }

    //oh look were managing memory :O
    for (Edge e : glyphEdges) {
        _safe_free_a(e.curves);
        e.nCurves = 0;
    }

    _safe_free_a(curveBuffer);

    return 0;
}

f32 smoothstep(f32 t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return t*t*(3.0f - 2.0f*t);
}

i32 ttfRender::RenderSDFToBitmap(Bitmap* sdf, Bitmap* bmp, size_t thresh) {
    if (!sdf || !bmp) return 1;

    Bitmap::Free(bmp);

    if (!sdf->data) return 2;
    if (sdf->header.bitsPerPixel < 24) return 3;

    const size_t by_pp = sdf->header.bitsPerPixel >> 3;

    bmp->header = sdf->header;
    bmp->header.fSz = sdf->header.w * sdf->header.h * by_pp;
    bmp->data = new byte[bmp->header.fSz];
    ZeroMem(bmp->data, bmp->header.fSz);

    i32 x, y;
    size_t p;

    for (y = 0; y < sdf->header.h; ++y) {
        for (x = 0; x < sdf->header.w; ++x) {
            p = (x + y * sdf->header.w) * by_pp;

            /*forrange(by_pp)
                bmp->data[p+i] = 255.0f - smoothstep((f32)-((signed)sdf->data[p] - 127) / (f32)thresh) * 255.0f;*/

                //bmp->data[p] = 255.0f - smoothstep((f32)-((signed)sdf->data[p] - 127) / (f32)thresh) * 255.0f;

            if ((signed)sdf->data[p] - 127 > 0)
                forrange(by_pp)
                    bmp->data[p+i] = 255.0f;
        }
    }

    return 0;
}