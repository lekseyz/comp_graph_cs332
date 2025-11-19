#define RAYGUI_IMPLEMENTATION
#include "raylib.h"
#include "raygui.h"
#include <vector>
#include <cmath>
#include <algorithm>

struct GHNode {
    Vector2 p;
    bool isInter;
    bool entry;
    bool visited;
    float alpha;
    GHNode* next;
    GHNode* prev;
    GHNode* neighbor;
    int pid;
};

static float Cross(Vector2 a, Vector2 b){ return a.x*b.y - a.y*b.x; }
static Vector2 Sub(Vector2 a, Vector2 b){ return {a.x-b.x, a.y-b.y}; }
static float Dot(Vector2 a, Vector2 b){ return a.x*b.x + a.y*b.y; }

static bool SegInter(Vector2 a, Vector2 b, Vector2 c, Vector2 d, float &t, float &u, Vector2 &p){
    Vector2 r = Sub(b,a);
    Vector2 s = Sub(d,c);

    float denom = Cross(r,s);
    float eps = 1e-8f;

    if (fabsf(denom) < eps) return false;
    Vector2 ac = Sub(c,a);

    t = Cross(ac,s)/denom;
    u = Cross(ac,r)/denom;

    if (t < -eps || t > 1+eps || u < -eps || u > 1+eps) return false;

    t = fminf(fmaxf(t,0.0f),1.0f);
    u = fminf(fmaxf(u,0.0f),1.0f);
    p = {a.x + t*r.x, a.y + t*r.y};
    return true;
}

static bool PointInPoly(const std::vector<Vector2>& poly, Vector2 q){
    bool c = false;
    int n = poly.size();

    for(int i = 0, j = n-1 ;i < n; j = i++){
        Vector2 pi=poly[i], pj=poly[j];
        bool cond=((pi.y>q.y)!=(pj.y>q.y)) && (q.x < (pj.x-pi.x)*(q.y-pi.y)/(pj.y-pi.y+0.0000001f)+pi.x);
        if(cond) c=!c;
    }
    return c;
}

static GHNode* MakeCycle(const std::vector<Vector2>& poly, int pid){
    if(poly.empty()) return nullptr;
    GHNode* first=nullptr;
    GHNode* prev=nullptr;

    for(size_t i = 0; i < poly.size(); ++i){
        GHNode* n = new GHNode();

        n->p=poly[i];
        n->isInter=false;
        n->entry=false;
        n->visited=false;
        n->alpha=-1.0f;
        n->neighbor=nullptr;
        n->pid=pid;
        n->prev=prev;

        if(prev) prev->next=n;
        else first=n;
        prev=n;
    }
    first->prev=prev;
    prev->next=first;
    return first;
}

static void InsertAfter(GHNode* at, GHNode* n){
    n->next=at->next;
    n->prev=at;
    at->next->prev=n;
    at->next=n;
}

static void InsertInEdge(GHNode* a0, GHNode* a1, GHNode* ins){
    GHNode* it=a0;
    while(it!=a1 && it->isInter && it->alpha<=ins->alpha) it=it->next;
    while(it!=a1 && !it->isInter) it=it->next;
    GHNode* pos=a0;
    for(GHNode* t=a0->next; ; t=t->next){
        if(t==a1 || (t->isInter && t->alpha>ins->alpha)){ pos=t->prev; break; }
        if(t==a0) { pos=a0; break; }
    }
    InsertAfter(pos,ins);
}

static void CollectNodes(GHNode* head, std::vector<GHNode*>& out){
    if(!head) return;
    GHNode* it=head;
    do{ out.push_back(it); it=it->next; }while(it!=head);
}

static std::vector<GHNode*> Intersections(GHNode* A, GHNode* B){
    std::vector<GHNode*> aNodes; CollectNodes(A,aNodes);
    std::vector<GHNode*> bNodes; CollectNodes(B,bNodes);

    for(size_t i = 0; i < aNodes.size(); ++i){
        GHNode* a0=aNodes[i];
        GHNode* a1=aNodes[(i+1)%aNodes.size()];

        for(size_t j=0;j<bNodes.size();++j){
            GHNode* b0=bNodes[j];
            GHNode* b1=bNodes[(j+1)%bNodes.size()];
    
            float ta, tb; Vector2 ip;

            if(SegInter(a0->p,a1->p,b0->p,b1->p,ta,tb,ip)){
                GHNode* na = new GHNode();
                na->p=ip; na->isInter=true; na->alpha=ta; na->visited=false; na->pid=a0->pid;

                GHNode* nb = new GHNode();
                nb->p=ip; nb->isInter=true; nb->alpha=tb; nb->visited=false; nb->pid=b0->pid;

                na->neighbor=nb; nb->neighbor=na;
                InsertInEdge(a0,a1,na);
                InsertInEdge(b0,b1,nb);
            }
        }
    }
    return {};
}

static void MarkEntry(GHNode* A, GHNode* B){
    std::vector<GHNode*> a; CollectNodes(A,a);
    std::vector<GHNode*> b; CollectNodes(B,b);
    std::vector<Vector2> aPoly, bPoly;

    for(auto* n:a) if(!n->isInter) aPoly.push_back(n->p);
    for(auto* n:b) if(!n->isInter) bPoly.push_back(n->p);

    bool inside = PointInPoly(bPoly, aPoly.empty()?A->p:aPoly[0]);
    GHNode* it=A;
    do{
        if(it->isInter && !it->visited){
            it->entry = !inside;
            it->neighbor->entry = inside;
            inside = !inside;
        }
        it=it->next;
    }while(it!=A);
}

static std::vector<std::vector<Vector2>> Traverse(GHNode* A){
    std::vector<std::vector<Vector2>> res;
    std::vector<GHNode*> a; CollectNodes(A,a);
    std::vector<GHNode*> inters;

    for(auto* n:a) if(n->isInter) inters.push_back(n);

    for(auto* n:inters){
        if(!n->visited && n->entry){
            std::vector<Vector2> poly;
            GHNode* start=n;
            GHNode* cur=n;

            do{
                poly.push_back(cur->p);
                if(cur->isInter){
                    cur->visited=true;
                    cur->neighbor->visited=true;
                    if(cur->entry) cur=cur->neighbor->next;
                    else cur=cur->neighbor->prev;
                }else{
                    cur=cur->next;
                }
            }while(cur!=start && cur!=nullptr);

            
            if(poly.size()>=3){
                std::vector<Vector2> clean;
                clean.reserve(poly.size());
                for(size_t i=0;i<poly.size();++i){
                    Vector2 p=poly[i];
                    if(i==0 || (fabsf(p.x-clean.back().x)>0.5f || fabsf(p.y-clean.back().y)>0.5f)) clean.push_back(p);
                }
                if(clean.size()>=3) res.push_back(clean);
            }
        }
    }
    return res;
}

static void FreeCycle(GHNode* head){
    if(!head) return;
    GHNode* it=head->next;
    while(it!=head){
        GHNode* n=it;
        it=it->next;
        delete n;
    }
    delete head;
}

static std::vector<std::vector<Vector2>> UnionGH(const std::vector<Vector2>& A, const std::vector<Vector2>& B){
    if(A.size()<3 && B.size()<3) return {};
    if(A.size()<3) return {B};
    if(B.size()<3) return {A};

    bool AinB = PointInPoly(B, A[0]);
    bool BinA = PointInPoly(A, B[0]);

    float eps=1e-6f;
    GHNode* a = MakeCycle(A,0);
    GHNode* b = MakeCycle(B,1);
    Intersections(a,b);

    bool hasInter=false;
    {
        std::vector<GHNode*> tmp; 
        CollectNodes(a,tmp);

        for(auto* n:tmp) if(n->isInter){ hasInter=true; break; }

        if(!hasInter){
            if(AinB && !BinA){ FreeCycle(a); FreeCycle(b); return {B}; }
            if(BinA && !AinB){ FreeCycle(a); FreeCycle(b); return {A}; }
            std::vector<std::vector<Vector2>> r; 
            
            r.push_back(A); r.push_back(B); FreeCycle(a); FreeCycle(b); 
            return r;
        }
    }
    MarkEntry(a,b);
    std::vector<std::vector<Vector2>> R = Traverse(a);
    if(R.empty()){
        if(AinB) R.push_back(B);
        else if(BinA) R.push_back(A);
        else { R.push_back(A); R.push_back(B); }
    }
    FreeCycle(a);
    FreeCycle(b);
    return R;
}

static void DrawPolyline(const std::vector<Vector2>& pts, bool closed, Color c, float thick){
    for(size_t i=0;i+1<pts.size();++i) DrawLineEx(pts[i],pts[i+1],thick,c);
    if(closed && pts.size()>=3) DrawLineEx(pts.back(),pts.front(),thick,c);
    for(auto& p:pts) DrawCircleV(p,4,c);
}

int main(){
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(1200, 800, "POLYGON UNION");
    SetTargetFPS(60);
    GuiLoadStyleDefault();
    int panelW=320;
    Rectangle canvas = {20,20,(float)(GetScreenWidth()-panelW-40), (float)(GetScreenHeight()-40)};
    enum Mode {MODE_IDLE, MODE_BUILD_A, MODE_BUILD_B};
    Mode mode=MODE_IDLE;
    std::vector<Vector2> polyA, polyB;
    std::vector<std::vector<Vector2>> unionPolys;
    bool hadUnion=false;

    while(!WindowShouldClose()){
        if(IsWindowResized()){
            canvas = {20,20,(float)(GetScreenWidth()-panelW-40),(float)(GetScreenHeight()-40)};
        }
        Vector2 m = GetMousePosition();
        bool inCanvas = CheckCollisionPointRec(m, canvas);
        if(inCanvas && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
            if(mode==MODE_BUILD_A){ polyA.push_back(m); hadUnion=false; unionPolys.clear(); }
            else if(mode==MODE_BUILD_B){ polyB.push_back(m); hadUnion=false; unionPolys.clear(); }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawRectangleRec(canvas, Color{245,245,245,255});
        DrawRectangleLinesEx(canvas,2,BLACK);

        if(!polyA.empty()) DrawPolyline(polyA, polyA.size()>=3, RED, 2.5f);
        if(!polyB.empty()) DrawPolyline(polyB, polyB.size()>=3, BLUE, 2.5f);

        if(hadUnion){
            for(auto& up: unionPolys){
                DrawPolyline(up, true, GREEN, 4.0f);
            }
        }

        DrawRectangle(GetScreenWidth()-panelW,0,panelW,GetScreenHeight(),Color{235,235,235,255});
        Rectangle r = { (float)GetScreenWidth()-panelW+20, 20, (float)panelW-40, 52 };
        if(GuiButton(r, "BUILD POLYGON A")){
            mode = MODE_BUILD_A;
        }
        r.y += 64;
        if(GuiButton(r, "BUILD POLYGON B")){
            mode = MODE_BUILD_B;
        }
        r.y += 64;
        if(GuiButton(r, "UNION")){
            unionPolys = UnionGH(polyA,polyB);
            hadUnion=true;
            mode=MODE_IDLE;
        }
        r.y += 64;
        if(GuiButton(r, "CLEAR")){
            polyA.clear();
            polyB.clear();
            unionPolys.clear();
            hadUnion=false;
            mode=MODE_IDLE;
        }
        r.y += 64;
        Rectangle lab = r; lab.height=40;
        const char* status = "MODE: IDLE";
        if(mode==MODE_BUILD_A) status="MODE: BUILD A";
        else if(mode==MODE_BUILD_B) status="MODE: BUILD B";
        GuiLabel(lab, status);

        EndDrawing();
    }
    CloseWindow();
    return 0;
}
