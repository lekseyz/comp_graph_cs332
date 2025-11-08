#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <cfloat>
#include <cstdio>
#include <climits>
#include <cstring>
#include <vector>

#pragma region layout
#define WINDOW_WIDTH  800
#define WINDOW_HIGHT  600

#define DRAW_BOX_WIDTH  520
#define DRAW_BOX_HIGHT  600

#define ELEMENTS_X  (DRAW_BOX_WIDTH + 10)
#define ELEMENTS_W  (WINDOW_WIDTH - DRAW_BOX_WIDTH - 20)
#define ELEMENTS_H  44
#define PAD         6
#pragma endregion

#pragma region misc
static constexpr float EPS = 1e-5f;

static inline Vector2 V2(float x, float y) { return Vector2{ x, y }; }
static inline Vector2 Add(Vector2 a, Vector2 b){ return Vector2Add(a,b); }
static inline Vector2 Sub(Vector2 a, Vector2 b){ return Vector2Subtract(a,b); }
static inline Vector2 Mul(Vector2 a, float s){ return Vector2Scale(a, s); }
static inline float  Len(Vector2 a){ return Vector2Length(a); }
static inline float  Dot(Vector2 a, Vector2 b){ return Vector2DotProduct(a,b); }

static inline float CrossV(Vector2 a, Vector2 b){ return a.x*b.y - a.y*b.x; }
static inline bool  ApproxEq(float a, float b, float e=EPS){ return std::fabs(a-b) <= e; }
static inline bool  ApproxEqPt(Vector2 a, Vector2 b, float e=EPS){ return std::fabs(a.x-b.x)<=e && std::fabs(a.y-b.y)<=e; }
#pragma endregion

static inline float Orient(Vector2 a, Vector2 b, Vector2 c) { return CrossV(Sub(b,a), Sub(c,a)); }

static void RemoveCollinear(std::vector<Vector2>& poly){
    if(poly.size() < 3) return;
    std::vector<Vector2> out;
    out.reserve(poly.size());
    for(size_t i=0;i<poly.size();++i){
        Vector2 prev = poly[(i + poly.size() - 1) % poly.size()];
        Vector2 curr = poly[i];
        Vector2 next = poly[(i + 1) % poly.size()];
        float cr = CrossV(Sub(curr, prev), Sub(next, curr));
        if(std::fabs(cr) > 1e-6f) out.push_back(curr);
    }
    if(out.size() >= 3) poly.swap(out);
}

static float SignedArea(const std::vector<Vector2>& poly){
    double A = 0.0;
    size_t n = poly.size();
    for(size_t i=0;i<n;i++){
        Vector2 a = poly[i];
        Vector2 b = poly[(i+1)%n];
        A += (double)a.x*b.y - (double)a.y*b.x;
    }
    return (float)(A*0.5);
}

static void EnsureCCW(std::vector<Vector2>& poly){
    if(poly.size() < 3) return;
    Vector2 c{0,0};
    for(auto& p: poly) { c = Vector2Add(c, p); }
    c = Vector2Scale(c, 1.0f/(float)poly.size());
    std::sort(poly.begin(), poly.end(), [&](const Vector2& A, const Vector2& B){
        float angA = atan2f(A.y - c.y, A.x - c.x);
        float angB = atan2f(B.y - c.y, B.x - c.x);
        return angA < angB;
    });
    if(SignedArea(poly) < 0) std::reverse(poly.begin(), poly.end());
}

static bool IsConvexCCW(const std::vector<Vector2>& poly){
    if(poly.size() < 3) return false;
    float sign = 0;
    size_t n = poly.size();
    for(size_t i=0;i<n;i++){
        Vector2 a = poly[i];
        Vector2 b = poly[(i+1)%n];
        Vector2 c = poly[(i+2)%n];
        float o = Orient(a,b,c);
        if(std::fabs(o) < 1e-6f) continue;
        if(sign == 0) sign = (o>0)? 1.0f : -1.0f;
        else if(o*sign < 0) return false;
    }
    return SignedArea(poly) > 0;
}

static bool PointInConvex(const std::vector<Vector2>& poly, Vector2 p){
    if(poly.size() < 3) return false;
    for(size_t i=0;i<poly.size();++i){
        Vector2 a = poly[i];
        Vector2 b = poly[(i+1)%poly.size()];
        if(Orient(a,b,p) < -1e-6f) return false;
    }
    return true;
}

static bool SegmentIntersect(Vector2 A, Vector2 B, Vector2 C, Vector2 D,
                             float& t, float& u, Vector2& P)
{
    Vector2 r = Sub(B,A);
    Vector2 s = Sub(D,C);
    float rxs = CrossV(r,s);
    if(std::fabs(rxs) < 1e-8f){
        return false; 
    }
    t = CrossV(Sub(C,A), s) / rxs;
    u = CrossV(Sub(C,A), r) / rxs;
    if(t < -1e-6f || t > 1+1e-6f || u < -1e-6f || u > 1+1e-6f) return false;
    t = std::clamp(t, 0.0f, 1.0f);
    u = std::clamp(u, 0.0f, 1.0f);
    P = Add(A, Mul(r, t));
    return true;
}

// ------------------- DRAW POLYGON -------------------
static void FillPolygonScanline(Image* img, Color color, const std::vector<Vector2>& poly)
{
    if(poly.size() < 3) return;

    int xmin = INT_MAX, xmax = INT_MIN;
    int ymin = INT_MAX, ymax = INT_MIN;

    for(auto&p: poly){
        xmin = std::min(xmin, (int)std::floor(p.x));
        xmax = std::max(xmax, (int)std::ceil (p.x));
        ymin = std::min(ymin, (int)std::floor(p.y));
        ymax = std::max(ymax, (int)std::ceil (p.y));
    }

    xmin = std::max(xmin, 0);
    ymin = std::max(ymin, 0);
    xmax = std::min(xmax, (int)img->width-1);
    ymax = std::min(ymax, (int)img->height-1);

    std::vector<int> inter; inter.reserve(poly.size());

    for(int y=ymin; y<=ymax; ++y){
        inter.clear();
        for(size_t i=0;i<poly.size();++i){
            Vector2 v1 = poly[i];
            Vector2 v2 = poly[(i+1)%poly.size()];
            if(std::fabs(v1.y - v2.y) < 1e-6f) continue;
            float yminEdge = std::min(v1.y, v2.y);
            float ymaxEdge = std::max(v1.y, v2.y);
            if(y >= (int)std::floor(yminEdge) && y < (int)std::ceil(ymaxEdge)){
                float x = v1.x + (y - v1.y) * (v2.x - v1.x) / (v2.y - v1.y);
                inter.push_back((int)std::round(x));
            }
        }
        std::sort(inter.begin(), inter.end());
        for(size_t j=0;j+1<inter.size(); j+=2){
            int x1 = std::max(0, std::min(inter[j],     (int)img->width-1));
            int x2 = std::max(0, std::min(inter[j + 1], (int)img->width-1));
            if(x2 < x1) std::swap(x1,x2);
            for(int x=x1; x<=x2; ++x) ImageDrawPixel(img, x, y, color);
        }
    }
}

static void StrokePolygon(Image* img, Color /*col*/, const std::vector<Vector2>& poly){
    if(poly.size() < 2) return;
    for(size_t i=0;i<poly.size();++i){
        Vector2 a = poly[i];
        ImageDrawCircle(img, (int)std::round(a.x), (int)std::round(a.y), 2, BLACK);
    }
}


struct Edge { Vector2 a,b; };

static std::vector<Edge> SubdivideByIntersections(const std::vector<Vector2>& P,
                                                  const std::vector<Vector2>& Q)
{
    std::vector<Edge> out;
    if(P.size()<2) return out;
    for(size_t i=0;i<P.size();++i){
        Vector2 A = P[i];
        Vector2 B = P[(i+1)%P.size()];
        std::vector<std::pair<float,Vector2>> cuts;
        cuts.push_back({0.0f, A});
        cuts.push_back({1.0f, B});

        for(size_t j=0;j<Q.size();++j){
            Vector2 C = Q[j];
            Vector2 D = Q[(j+1)%Q.size()];
            float t,u; Vector2 X;
            if(SegmentIntersect(A,B,C,D,t,u,X)){
                bool dup = false;
                for(auto&pr: cuts){
                    if(std::fabs(pr.first - t) < 1e-5f || ApproxEqPt(pr.second, X))
                    { dup = true; break; }
                }
                if(!dup) cuts.push_back({t, X});
            }
        }

        std::sort(cuts.begin(), cuts.end(),
                  [](auto&L, auto&R){ return L.first < R.first; });

        for(size_t k=0;k+1<cuts.size();++k){
            Vector2 S = cuts[k].second;
            Vector2 E = cuts[k+1].second;
            if(Len(Sub(E,S)) > 1e-5f) out.push_back({S,E});
        }
    }
    return out;
}

static bool MidpointInside(const std::vector<Vector2>& poly, const Edge& e){
    Vector2 m = Vector2Scale(Vector2Add(e.a, e.b), 0.5f);
    return PointInConvex(poly, m);
}

static std::vector<std::vector<Vector2>> BuildCycles(const std::vector<Edge>& edges){
    std::vector<std::vector<Vector2>> cycles;
    std::vector<char> used(edges.size(), 0);

    auto findNext = [&](Vector2 cur)->int{
        for(size_t i=0;i<edges.size();++i){
            if(used[i]) continue;
            if(ApproxEqPt(edges[i].a, cur)) return (int)i;
        }
        return -1;
    };

    for(size_t i=0;i<edges.size();++i){
        if(used[i]) continue;
        std::vector<Vector2> loop;
        loop.push_back(edges[i].a);
        loop.push_back(edges[i].b);
        used[i] = 1;

        Vector2 start = edges[i].a;
        Vector2 cur   = edges[i].b;

        int guard = 0;
        while(!ApproxEqPt(cur, start) && guard < 10000){
            int j = findNext(cur);
            if(j<0) break;
            used[j] = 1;
            loop.push_back(edges[j].b);
            cur = edges[j].b;
            guard++;
        }

        if(loop.size() >= 3){
            if(ApproxEqPt(loop.front(), loop.back())) loop.pop_back();
            RemoveCollinear(loop);
            if(loop.size() >= 3){
                if(SignedArea(loop) < 0) std::reverse(loop.begin(), loop.end());
                cycles.push_back(loop);
            }
        }
    }
    return cycles;
}

static float crossXZ(const Vector2& a, const Vector2& b, const Vector2& c){
    return CrossV(Sub(b,a), Sub(c,a));
}

static std::vector<Vector2> ConvexHull(std::vector<Vector2> pts){
    if(pts.size()<=1) return pts;

    auto lessXY = [](const Vector2& A, const Vector2& B){
        if(!ApproxEq(A.x, B.x)) return A.x < B.x;
        return A.y < B.y - EPS;
    };
    std::sort(pts.begin(), pts.end(), lessXY);
    std::vector<Vector2> filtered;
    filtered.reserve(pts.size());
    for(auto &p: pts){
        if(filtered.empty() || !ApproxEqPt(filtered.back(), p)) filtered.push_back(p);
    }
    pts.swap(filtered);
    if(pts.size()<=2) return pts;

    std::vector<Vector2> H;
    for(auto &p: pts){
        while(H.size()>=2 && crossXZ(H[H.size()-2], H.back(), p) <= 0) H.pop_back();
        H.push_back(p);
    }
    size_t t = H.size()+1;
    for(int i=(int)pts.size()-2; i>=0; --i){
        Vector2 p = pts[i];
        while(H.size()>=t && crossXZ(H[H.size()-2], H.back(), p) <= 0) H.pop_back();
        H.push_back(p);
    }
    if(!H.empty()) H.pop_back();
    if(SignedArea(H) < 0) std::reverse(H.begin(), H.end());
    RemoveCollinear(H);
    return H;
}


static std::vector<Vector2> UnionConvexPolygonsOne(const std::vector<Vector2>& A, const std::vector<Vector2>& B)
{
    if(A.size()<3 && B.size()<3) return {};
    if(A.size()<3) return B;
    if(B.size()<3) return A;

    std::vector<Vector2> P = A, Q = B;
    EnsureCCW(P); RemoveCollinear(P);
    EnsureCCW(Q); RemoveCollinear(Q);

    if(!IsConvexCCW(P) || !IsConvexCCW(Q)){
        std::vector<Vector2> all = P; all.insert(all.end(), Q.begin(), Q.end());
        return ConvexHull(std::move(all));
    }

    if(PointInConvex(P, Q[0]) && PointInConvex(P, Q[Q.size()/2])) return P;
    if(PointInConvex(Q, P[0]) && PointInConvex(Q, P[P.size()/2])) return Q;

    std::vector<Edge> segP = SubdivideByIntersections(P, Q);
    std::vector<Edge> segQ = SubdivideByIntersections(Q, P);

    std::vector<Edge> boundary; boundary.reserve(segP.size()+segQ.size());
    for(auto&e: segP) if(!MidpointInside(Q, e)) boundary.push_back(e);
    for(auto&e: segQ) if(!MidpointInside(P, e)) boundary.push_back(e);

    auto cycles = BuildCycles(boundary);

    if(cycles.empty()){
        std::vector<Vector2> all = P; all.insert(all.end(), Q.begin(), Q.end());
        return ConvexHull(std::move(all));
    }

    if(cycles.size()==1){
        return cycles[0];
    }

    
    std::vector<Vector2> allPts;
    for(auto &c : cycles) allPts.insert(allPts.end(), c.begin(), c.end());
    
    allPts.insert(allPts.end(), P.begin(), P.end());
    allPts.insert(allPts.end(), Q.begin(), Q.end());
    return ConvexHull(std::move(allPts));
}

// ------------------- main -------------------
enum class Mode { DRAW_A, DRAW_B, IDLE };

int main(){
    InitWindow(WINDOW_WIDTH, WINDOW_HIGHT, "UNION OF CONVEX POLYGONS");
    SetTargetFPS(60);

    Rectangle panel  = {0,0, (float)DRAW_BOX_WIDTH, (float)DRAW_BOX_HIGHT};
    Rectangle canvas = {1,1, (float)DRAW_BOX_WIDTH-2, (float)DRAW_BOX_HIGHT-2};

    Image    canvasImage   = GenImageColor((int)canvas.width, (int)canvas.height, WHITE);
    Texture2D canvasTexture = LoadTextureFromImage(canvasImage);

    std::vector<Vector2> polyA, polyB;
    std::vector<Vector2> unionPoly; // ALWAYS ONE POLYGON
    Mode mode = Mode::IDLE;

    bool showMsg = false;
    char msgText[512] = {0};

    auto DrawGridToImage = [&](Image& img){
        for(int x=0; x<canvas.width; x+=40){
            for(int y=0; y<canvas.height; ++y)
                ImageDrawPixel(&img, x, y, (Color){235,235,235,255});
        }
        for(int y=0; y<canvas.height; y+=40){
            for(int x=0; x<canvas.width; ++x)
                ImageDrawPixel(&img, x, y, (Color){235,235,235,255});
        }
    };

    auto RedrawAll = [&](){
        UnloadImage(canvasImage);
        canvasImage = GenImageColor((int)canvas.width, (int)canvas.height, WHITE);
        DrawGridToImage(canvasImage);

        // If union exists — SHOW ONLY ONE GREEN POLYGON (as required)
        if(!unionPoly.empty()){
            FillPolygonScanline(&canvasImage, (Color){40,160,40,160}, unionPoly);
            StrokePolygon(&canvasImage, DARKGREEN, unionPoly);
        } else {
            // Otherwise show inputs
            if(polyA.size() >= 3){
                std::vector<Vector2> tmp = polyA; EnsureCCW(tmp); RemoveCollinear(tmp);
                FillPolygonScanline(&canvasImage, (Color){80,120,255,90}, tmp);
                StrokePolygon(&canvasImage, BLUE, tmp);
            } else {
                for(auto &p : polyA)
                    ImageDrawCircle(&canvasImage, (int)p.x, (int)p.y, 3, BLUE);
            }

            if(polyB.size() >= 3){
                std::vector<Vector2> tmp = polyB; EnsureCCW(tmp); RemoveCollinear(tmp);
                FillPolygonScanline(&canvasImage, (Color){255,160,60,90}, tmp);
                StrokePolygon(&canvasImage, ORANGE, tmp);
            } else {
                for(auto &p : polyB)
                    ImageDrawCircle(&canvasImage, (int)p.x, (int)p.y, 3, ORANGE);
            }
        }

        UpdateTexture(canvasTexture, canvasImage.data);
    };

    RedrawAll();

    while(!WindowShouldClose()){
        BeginDrawing();
        ClearBackground(RAYWHITE);

        const char* stateText = (mode==Mode::DRAW_A)? "POLYGON A MODE" :
                                (mode==Mode::DRAW_B)? "POLYGON B MODE" :
                                                      "CHOOSE AN OPTION:";
        GuiLabel((Rectangle){ELEMENTS_X, PAD, ELEMENTS_W, ELEMENTS_H}, stateText);

        if(GuiButton((Rectangle){ELEMENTS_X, PAD*2+ELEMENTS_H*1, ELEMENTS_W, ELEMENTS_H}, "POLYGON A MODE")){
            mode = Mode::DRAW_A;
            unionPoly.clear();
            RedrawAll();
        }
        if(GuiButton((Rectangle){ELEMENTS_X, PAD*3+ELEMENTS_H*2, ELEMENTS_W, ELEMENTS_H}, "POLYGON B MODE")){
            mode = Mode::DRAW_B;
            unionPoly.clear();
            RedrawAll();
        }
        if(GuiButton((Rectangle){ELEMENTS_X, PAD*4+ELEMENTS_H*3, ELEMENTS_W, ELEMENTS_H}, "FINISH")){
            std::vector<Vector2>* cur = (mode==Mode::DRAW_A? &polyA : (mode==Mode::DRAW_B? &polyB : nullptr));
            if(cur && cur->size()>=3){
                EnsureCCW(*cur);
                RemoveCollinear(*cur);
                if(!IsConvexCCW(*cur)){
                    snprintf(msgText, sizeof(msgText), "INCORRECT POLYGON (MUST BE CONVEX CCW)");
                    showMsg = true;
                }
                unionPoly.clear();
                RedrawAll();
            }
        }
        if(GuiButton((Rectangle){ELEMENTS_X, PAD*5+ELEMENTS_H*4, ELEMENTS_W, ELEMENTS_H}, "UNION POLYGONS")){
            if(polyA.size()<3 || polyB.size()<3){
                snprintf(msgText, sizeof(msgText), "ONE OF THE POLYGONS IS INCOMPLETE");
                showMsg = true;
            } else {
                unionPoly = UnionConvexPolygonsOne(polyA, polyB);
                RemoveCollinear(unionPoly);
                EnsureCCW(unionPoly);
                RedrawAll();
                if(!unionPoly.empty())
                    snprintf(msgText, sizeof(msgText), "UNION: ONE POLYGON. AREA: %.2f",
                             std::fabs(SignedArea(unionPoly)));
                else
                    snprintf(msgText, sizeof(msgText), "UNION FAILED");
                showMsg = true;
            }
        }
        if(GuiButton((Rectangle){ELEMENTS_X, PAD*6+ELEMENTS_H*5, ELEMENTS_W, ELEMENTS_H}, "CLEAN")){
            polyA.clear(); polyB.clear(); unionPoly.clear();
            mode = Mode::IDLE;
            RedrawAll();
        }

        // Add points
        Vector2 mp = GetMousePosition();
        if(CheckCollisionPointRec(mp, panel)){
            Vector2 inner = { mp.x - panel.x, mp.y - panel.y };
            if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                if(mode==Mode::DRAW_A){
                    polyA.push_back(inner); unionPoly.clear(); RedrawAll();
                } else if(mode==Mode::DRAW_B){
                    polyB.push_back(inner); unionPoly.clear(); RedrawAll();
                }
            }
            if(IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)){
                std::vector<Vector2>* cur = (mode==Mode::DRAW_A? &polyA : (mode==Mode::DRAW_B? &polyB : nullptr));
                if(cur && cur->size()>=3){
                    EnsureCCW(*cur);
                    RemoveCollinear(*cur);
                    if(!IsConvexCCW(*cur)){
                        snprintf(msgText, sizeof(msgText), "INCORRECT POLYGON (MUST BE CONVEX CCW)");
                        showMsg = true;
                    }
                    unionPoly.clear();
                    RedrawAll();
                }
            }
        }

        GuiPanel(panel, NULL);
        DrawTexture(canvasTexture, (int)panel.x, (int)panel.y, WHITE);

        if(showMsg){
            int r = GuiMessageBox((Rectangle){WINDOW_WIDTH/2-200, WINDOW_HIGHT/2-100, 400, 200},
                                  "RESULT", msgText, "OK");
            if(r >= 0) showMsg = false;
        }

        EndDrawing();
    }

    UnloadTexture(canvasTexture);
    UnloadImage(canvasImage);
    CloseWindow();
    return 0;
}
