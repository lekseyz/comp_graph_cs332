#version 330
in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform vec3 camPos;
uniform vec3 camTarget;
uniform vec3 lightPos1;
uniform vec3 lightPos2;
uniform float wallMode;
uniform float objMirror;
uniform float objTrans;

const int MAX_STEPS = 100;
const float MAX_DIST = 100.0;
const float SURF_DIST = 0.01;

float sdBox(vec3 p, vec3 b) {
    vec3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float sdSphere(vec3 p, float s) {
    return length(p) - s;
}

vec2 GetDist(vec3 p) {
    float room = -sdBox(p, vec3(4.0));
    float sphere = sdSphere(p - vec3(-1.5, -2.5, 1.0), 1.3);
    float box = sdBox(p - vec3(1.5, -3.0, -1.5), vec3(1.0));
    
    float d = min(room, min(sphere, box));
    
    float id = 0.0; 
    if (d == room) id = 1.0;
    else if (d == sphere) id = 2.0;
    else if (d == box) id = 3.0;
    
    return vec2(d, id);
}

vec3 GetNormal(vec3 p) {
    float d = GetDist(p).x;
    vec2 e = vec2(0.001, 0);
    vec3 n = d - vec3(GetDist(p-e.xyy).x, GetDist(p-e.yxy).x, GetDist(p-e.yyx).x);
    return normalize(n);
}

float GetLight(vec3 p, vec3 n, vec3 lp) {
    vec3 l = normalize(lp - p);
    float dif = clamp(dot(n, l), 0.0, 1.0);
    float d = RayMarch(p + n * SURF_DIST * 2.0, l).x;
    if (d < length(lp - p)) dif *= 0.1;
    return dif;
}

vec3 RayMarch(vec3 ro, vec3 rd) {
    float dO = 0.0;
    float id = 0.0;
    for(int i=0; i<MAX_STEPS; i++) {
        vec3 p = ro + rd * dO;
        vec2 dS = GetDist(p);
        dO += dS.x;
        id = dS.y;
        if(dO > MAX_DIST || abs(dS.x) < SURF_DIST) break;
    }
    return vec3(dO, id, 0.0);
}

vec3 GetMaterialColor(float id, vec3 p, vec3 n) {
    if (id == 1.0) {
        if (abs(n.x - 1.0) < 0.1) return vec3(1.0, 0.0, 0.0);
        if (abs(n.x + 1.0) < 0.1) return vec3(0.0, 1.0, 0.0);
        return vec3(1.0);
    }
    if (id == 2.0) return vec3(0.0, 0.5, 1.0);
    if (id == 3.0) return vec3(1.0, 0.8, 0.0);
    return vec3(0.0);
}

void main() {
    vec2 uv = (fragTexCoord - 0.5) * resolution / resolution.y;
    vec3 ro = camPos;
    vec3 f = normalize(camTarget - ro);
    vec3 r = normalize(cross(vec3(0,1,0), f));
    vec3 u = cross(f, r);
    vec3 rd = normalize(f + uv.x * r + uv.y * u);

    vec3 col = vec3(0.0);
    vec3 attenuation = vec3(1.0);

    for (int bounce = 0; bounce < 3; bounce++) {
        vec3 d = RayMarch(ro, rd);
        float t = d.x;
        float id = d.y;

        if (t > MAX_DIST) break;

        vec3 p = ro + rd * t;
        vec3 n = GetNormal(p);
        vec3 matCol = GetMaterialColor(id, p, n);
        
        float dif1 = GetLight(p, n, lightPos1);
        float dif2 = GetLight(p, n, lightPos2);
        vec3 directLight = matCol * (dif1 + dif2) * 0.5;
        
        bool isMirrorWall = (id == 1.0 && (
            (wallMode == 1.0 && n.z > 0.5) || 
            (wallMode == 2.0 && n.x > 0.5) || 
            (wallMode == 3.0 && n.x < -0.5) ||
            (wallMode == 4.0 && n.y < -0.5)
        ));

        bool isReflective = (id > 1.5 && objMirror > 0.5) || isMirrorWall;
        bool isTransparent = (id > 1.5 && objTrans > 0.5);

        if (isTransparent) {
            col += directLight * attenuation * 0.2;
            ro = p - n * SURF_DIST * 2.0; 
            attenuation *= 0.9;
        } else if (isReflective) {
            col += directLight * attenuation * 0.1;
            ro = p + n * SURF_DIST * 2.0;
            rd = reflect(rd, n);
            attenuation *= 0.8;
            continue;
        } else {
            col += directLight * attenuation;
            break;
        }
    }
    
    finalColor = vec4(pow(col, vec3(0.4545)), 1.0);
}