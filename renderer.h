#pragma once

// ============================================================================
//  renderer.h - OpenGL resource wrappers (RAII) and the low-level renderer.
//  ShaderProgram owns the linked GLSL program together with its uniform
//  locations, GpuMesh owns one VAO/VBO/EBO triplet and Renderer bundles the
//  shader, the unit-cube mesh and the procedural car mesh.
// ============================================================================

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include "car_model.h"
#include "vehicle_models.h"

inline constexpr const char* kVertexShaderSource = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
layout(location=3) in float aMat;
uniform mat4 uMVP;
uniform mat4 uModel;
out vec3 vNormal;
out vec2 vUV;
out float vMat;
out vec3 vWorldPos;
void main() {
    vNormal = mat3(uModel) * aNormal;
    vUV = aUV;
    vMat = aMat;
    vWorldPos = vec3(uModel * vec4(aPos, 1.0));
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

inline constexpr const char* kFragmentShaderSource = R"(
#version 330 core
in vec3 vNormal;
in vec2 vUV;
in float vMat;
in vec3 vWorldPos;
out vec4 FragColor;
uniform vec3 uColor;   // base body / paint color (BODY and PAINT materials)
uniform vec3 uLight;
uniform vec3 uCamPos;

vec3 matColor(float m) {
    int id = int(m + 0.5);
    if (id == 0 || id == 15) return uColor;            // BODY, PAINT
    if (id == 1)  return vec3(0.45, 0.55, 0.62);       // GLASS
    if (id == 2)  return vec3(0.08, 0.08, 0.09);       // RUBBER
    if (id == 3)  return vec3(0.65, 0.66, 0.68);       // ALLOY
    if (id == 4)  return vec3(0.82, 0.83, 0.85);       // CHROME
    if (id == 5)  return vec3(0.03, 0.03, 0.035);      // BLACK
    if (id == 6)  return vec3(1.00, 0.95, 0.75);       // LAMP_HEAD
    if (id == 7)  return vec3(0.75, 0.10, 0.10);       // LAMP_TAIL
    if (id == 8)  return vec3(0.16, 0.14, 0.13);       // CABIN
    if (id == 9)  return vec3(0.95, 0.95, 0.95);       // SIGN
    if (id == 10) return vec3(0.10, 0.30, 0.95);       // BEACON_B
    if (id == 11) return vec3(1.00, 0.65, 0.05);       // BEACON_A
    if (id == 12) return vec3(0.95, 0.10, 0.10);       // BEACON_R
    if (id == 13) return vec3(0.85, 0.08, 0.08);       // BRAKE
    if (id == 14) return vec3(0.22, 0.20, 0.20);       // FABRIC
    if (id == 16) return vec3(0.18, 0.18, 0.20);       // SPOKE
    return uColor;
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLight);
    vec3 V = normalize(uCamPos - vWorldPos);

    // 1. Direct ceiling directional light
    float diff1 = max(dot(N, L), 0.0);

    // 2. Secondary indoor cross light (diffused reflection from opposite walls)
    vec3 L2 = normalize(vec3(-L.x, 0.5, -L.z));
    float diff2 = max(dot(N, L2), 0.0) * 0.32;

    // 3. Ground bounce ambient from bright epoxy floor:
    // Underside of pipes/beams receives strong bounce light, preventing pitch-black / flat look.
    float groundBounce = mix(0.46, 0.58, N.y * 0.5 + 0.5);

    float diffuse = diff1 * 0.54 + diff2 + groundBounce * 0.38;

    // 4. Blinn-Phong specular highlight for glossy painted pipes, metal conduit and stoppers
    vec3 H = normalize(L + V);
    float specVal = pow(max(dot(N, H), 0.0), 32.0) * 0.40;

    // 5. Linear neon ceiling tube reflection strip
    vec3 H2 = normalize(vec3(0.0, 1.0, 0.0) + V);
    float neonSpec = pow(max(dot(N, H2), 0.0), 20.0) * 0.22;

    vec3 baseCol = matColor(vMat);
    vec3 litColor = baseCol * diffuse + vec3(1.0, 0.98, 0.94) * (specVal + neonSpec);

    FragColor = vec4(litColor, 1.0);
}
)";

// ============================================================================
//  Advanced car shader: PBR-ish shading with procedural body details,
//  text atlas for the roof sign and number plate, two-pass glass.
//  All body-detail checks run in car-local space; the CPU passes the
//  camera and light already transformed into the car's local frame so the
//  procedural coordinates (door lines, lamps, grille) stay correct even
//  when the car is translated / yawed in the world.
// ============================================================================
// GPU-cost notes (v3.3.1 optim):
// - fbm reduced from 4 to 2 octaves
// - tree-band noise removed from skyEnv (was 2 extra noise samples + atan)
// - sun specular uses lower exponents (128 / 8 instead of 900 / 10)
// - still produces a usable sky reflection for car paint / glass
inline constexpr const char* kCarCommonShaderSource = R"(
const float PI = 3.14159265;
uniform vec3 uSun;
float hash(vec2 p){ p=fract(p*vec2(123.34,456.21)); p+=dot(p,p+45.32); return fract(p.x*p.y); }
float vnoise(vec2 p){ vec2 i=floor(p), f=fract(p); f=f*f*(3.-2.*f);
  return mix(mix(hash(i),hash(i+vec2(1,0)),f.x), mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x), f.y); }
float fbm(vec2 p){ float s=0., a=.5; for(int i=0;i<2;i++){ s+=a*vnoise(p); p*=2.03; a*=.5; } return s; }
const vec3 HOR = vec3(0.72,0.81,0.90);
vec3 skyBase(vec3 d){ vec3 zen=vec3(0.15,0.33,0.72); return mix(HOR, zen, pow(clamp(d.y,0.,1.),0.45)); }
vec3 skyEnv(vec3 d, float rough){
  vec3 c;
  if(d.y>=0.){
    c = skyBase(d);
    // Cheap cloud layer (2-octave fbm only)
    vec2 uv = d.xz/(d.y+0.25)*1.8;
    float cl = smoothstep(0.50,0.90,fbm(uv+7.));
    c = mix(c, vec3(1.), cl*0.55);
    float s = max(dot(d,uSun),0.);
    // Soft sun disk + halo (avoid pow(...,900))
    float sunDisk = s*s; sunDisk = sunDisk*sunDisk; sunDisk = sunDisk*sunDisk; // ~s^16
    c += vec3(1.,.9,.7)*(sunDisk*sunDisk*12.*(1.-rough) + pow(s,8.)*0.10);
    // Horizon darkening instead of procedural trees
    c = mix(c * 0.55, c, smoothstep(0.0, 0.12 + rough*0.08, d.y));
    c = mix(c, skyBase(d), rough*rough);
  } else {
    c = mix(HOR, vec3(0.13,0.14,0.13), smoothstep(0.,-0.2,d.y));
  }
  return c;
}
vec3 aces(vec3 x){ return clamp((x*(2.51*x+0.03))/(x*(2.43*x+0.59)+0.14),0.,1.); }
)";

inline constexpr const char* kCarVertexShaderSource = R"(
layout(location=0) in vec3 aPos; layout(location=1) in vec3 aNrm; layout(location=2) in vec2 aUV; layout(location=3) in float aMat;
uniform mat4 uMVP;
out vec3 vPos; out vec3 vNrm; out vec2 vUV; flat out int vMat;
void main(){ vPos=aPos; vNrm=aNrm; vUV=aUV; vMat=int(aMat+0.5); gl_Position=uMVP*vec4(aPos,1.); }
)";

inline constexpr const char* kCarFragmentShaderSource = R"(
in vec3 vPos; in vec3 vNrm; in vec2 vUV; flat in int vMat;
out vec4 frag;
uniform vec3 uCam; uniform vec3 uBaseColor; uniform int uPass; uniform float uExposure;
uniform sampler2D uTex;

struct Surf { vec3 albedo; float metal; float rough; float coat; vec3 emis; };
Surf mk(vec3 a,float m,float r,float c){ Surf s; s.albedo=a; s.metal=m; s.rough=r; s.coat=c; s.emis=vec3(0.); return s; }
Surf paint(){ return mk(uBaseColor,0.9,0.36,1.); }
Surf plastic(vec3 c,float r){ return mk(c,0.,r,0.); }
Surf metalS(vec3 c,float r){ return mk(c,1.,r,0.); }

vec3 envBRDF(vec3 F0,float rough,float nv){
  const vec4 c0=vec4(-1.,-.0275,-.572,.022), c1=vec4(1.,.0425,1.04,-.04);
  vec4 r=rough*c0+c1; float a004=min(r.x*r.x,exp2(-9.28*nv))*r.x+r.y;
  vec2 AB=vec2(-1.04,1.04)*a004+r.zw; return F0*AB.x+AB.y;
}
vec3 shade(Surf s, vec3 N, vec3 V, float ao){
  vec3 F0=mix(vec3(0.04),s.albedo,s.metal), diff=s.albedo*(1.-s.metal);
  vec3 L=uSun, H=normalize(L+V);
  float nl=max(dot(N,L),0.), nv=max(dot(N,V),1e-3), nh=max(dot(N,H),0.), vh=max(dot(V,H),0.);
  float a=max(s.rough*s.rough,0.03), a2=a*a, dd=nh*nh*(a2-1.)+1.;
  float D=a2/(PI*dd*dd);
  vec3 F=F0+(1.-F0)*pow(1.-vh,5.);
  float k=(s.rough+1.)*(s.rough+1.)/8.;
  float G=(nl/(nl*(1.-k)+k))*(nv/(nv*(1.-k)+k));
  vec3 spec=D*F*G/(4.*nl*nv+1e-3);
  vec3 sun=vec3(1.,.95,.85)*3.2;
  vec3 col=(diff/PI+spec)*sun*nl;
  vec3 amb=mix(vec3(0.16,0.17,0.16), vec3(0.40,0.52,0.78), N.y*0.5+0.5);
  col+=diff*amb*ao;
  vec3 R=reflect(-V,N);
  col+=skyEnv(R,s.rough)*envBRDF(F0,s.rough,nv)*mix(0.4,1.,ao);
  if(s.coat>0.){
    float Fc=0.04+0.96*pow(1.-nv,5.);
    vec3 cs=skyEnv(R,0.04)*mix(0.4,1.,ao);
    // Soft coat highlight (avoid pow(...,600))
    float rh=max(dot(R,L),0.); rh=rh*rh; rh=rh*rh; rh=rh*rh; // ~rh^8
    cs+=sun*rh*rh*2.5;
    col=col*(1.-s.coat*Fc)+cs*Fc*s.coat;
  }
  return col+s.emis;
}

float winMargin(vec3 p, vec2 uv){
  float x=p.x, m;
  if(uv.x<0.){
    float mt=-uv.x-0.055;
    float mw=min(x-0.14,0.84-x), mr=min(x+1.44,-0.90-x);
    m=min(mt,max(mw,mr));
  } else {
    float ms=min(uv.x-0.055,uv.y-0.035);
    float mf=min(x-0.035,0.80-x), mb=min(x+1.26,-0.095-x);
    m=min(ms,max(mf,mb));
  }
  return m;
}
float archD(vec3 p){
  if(abs(p.z)<0.50) return 1e9;
  return min(length(p.xy-vec2(1.275,0.30)), length(p.xy-vec2(-1.275,0.30)));
}
float atlas(float u,float up,float row0){
  return texture(uTex, vec2(u, row0+(1.-up)*0.5)).r;
}

Surf bodySurf(vec3 p, vec3 n, vec2 uv, float m, float ad){
  Surf s=paint();
  float x=p.x, y=p.y, z=abs(p.z);
  bool sideF=abs(n.z)>0.55, frontF=n.x>0.30&&x>1.70, rearF=n.x<-0.30&&x<-1.70;
  if(n.y<-0.55) return plastic(vec3(0.015),0.8);
  if(m>-0.02) return plastic(vec3(0.01),0.45);
  if(ad<0.368) return plastic(vec3(0.02),0.6);
  if(sideF&&y<0.935&&y>0.27){
    float k=y-0.6;
    float d=min(min(abs(x-(0.72+0.02*k)),abs(x+0.02)),abs(x-(-0.90-0.02*k)));
    if(d<0.0024) return plastic(vec3(0.02),0.6);
  }
  if(sideF&&y<0.25&&x>-0.88&&x<0.74) return plastic(vec3(0.025),0.55);
  if(sideF){
    vec2 q=vec2(x-0.21,y-0.855), q2=vec2(x+0.68,y-0.855);
    if((abs(q.x)<0.075&&abs(q.y)<0.011)||(abs(q2.x)<0.075&&abs(q2.y)<0.011)) return metalS(vec3(0.75),0.2);
    float r=length(vec2(x+0.50,y-0.60));
    if(r<0.095) return r>0.074 ? metalS(vec3(0.95,0.62,0.10),0.3) : plastic(vec3(0.03,0.12,0.45),0.35);
  }
  if(n.y>0.6&&x>1.0&&x<2.05&&abs(z-0.715)<0.0022) return plastic(vec3(0.02),0.6);
  if(n.y>0.6&&abs(x-0.985)<0.002&&z<0.71)         return plastic(vec3(0.02),0.6);
  if(frontF){
    float t=clamp((z-0.30)/0.54,0.,1.);
    float yc=0.69-0.02*t, hh=0.024+0.040*t;
    if(z>0.30&&z<0.84&&abs(y-yc)<hh){
      float proj=length(vec2((z-0.50)/0.075,(y-0.665)/0.045));
      vec3 c=mix(vec3(0.05,0.055,0.07),vec3(0.38,0.40,0.44),smoothstep(0.7,1.3,proj));
      if(abs(y-yc)>hh-0.006||z<0.32) c=vec3(0.02);
      return mk(c,0.7,0.10,1.);
    }
    if(z<0.28&&y>0.635&&y<0.715){
      if(length(vec2(z/0.05,(y-0.675)/0.028))<1.) return metalS(vec3(0.85),0.1);
      if(abs(y-0.69)<0.004) return metalS(vec3(0.8),0.15);
      return plastic(vec3(0.015),0.5);
    }
    if(z<0.235&&y>0.395&&y<0.505){
      float u=0.5-p.z/0.47, up=(y-0.395)/0.11;
      float t2=atlas(u,up,0.5);
      bool border=z>0.225||y<0.402||y>0.498;
      return plastic(border?vec3(0.02):mix(vec3(0.92),vec3(0.02),t2),0.4);
    }
    if(z<0.52&&y<0.375) return plastic(vec3(0.02),0.5);
    float fr=length(vec2(z-0.62,y-0.36));
    if(fr<0.045) return mk(vec3(0.9,0.92,0.95),0.5,0.1,1.);
    if(fr<0.065) return plastic(vec3(0.02),0.5);
  }
  if(rearF){
    float t=clamp((z-0.28)/0.56,0.,1.);
    float hh=0.05+0.035*t;
    if(z>0.28&&z<0.84&&abs(y-0.79)<hh){
      vec3 c=vec3(0.55,0.02,0.03);
      if(z<0.46&&y<0.78) c=vec3(0.85,0.85,0.8);
      Surf l=mk(c,0.2,0.1,1.); l.emis=c*0.08; return l;
    }
    if(z<0.235&&y>0.56&&y<0.66){
      float u=0.5+p.z/0.47, up=(y-0.56)/0.10;
      float t2=atlas(u,up,0.5);
      bool border=z>0.225||y<0.567||y>0.653;
      return plastic(border?vec3(0.02):mix(vec3(0.92),vec3(0.02),t2),0.4);
    }
    if(y<0.47) return plastic(vec3(0.02),0.5);
  }
  return s;
}

void main(){
  vec3 P=vPos, N=normalize(vNrm), V=normalize(uCam-P);
  bool ff=gl_FrontFacing;
  if(uPass==1&&vMat!=0) discard;
  Surf s; float ao=1.; vec3 col;

  if(vMat==0){
    float m=winMargin(P,vUV);
    bool glass=m>0.;
    if(uPass==1){
      if(!glass) discard;
      float nv=abs(dot(N,V)); vec3 Ng=ff?N:-N;
      vec3 R=reflect(-V,Ng);
      float F=0.04+0.96*pow(1.-nv,5.);
      vec3 c=skyEnv(R,0.02)*(F+0.10)+vec3(0.004);
      float gs=max(dot(R,uSun),0.); gs=gs*gs; gs=gs*gs; gs=gs*gs; // ~gs^8
      c+=vec3(1.,.9,.7)*gs*gs*3.0;
      frag=vec4(pow(aces(c*uExposure),vec3(1./2.2)),clamp(0.40+F*0.6,0.,1.));
      return;
    }
    if(glass) discard;
    float ad=archD(P);
    if(ad<0.356) discard;
    if(!ff){
      vec3 c=P.y>1.0?vec3(0.40,0.38,0.33):vec3(0.045,0.045,0.05);
      frag=vec4(pow(aces(c*0.7*uExposure),vec3(1./2.2)),1.); return;
    }
    s=bodySurf(P,N,vUV,m,ad);
    ao=mix(0.35,1.,smoothstep(0.15,0.7,P.y));
    col=shade(s,N,V,ao);
  } else {
    if(!ff) N=-N;
    if(vMat==2){ s=plastic(vec3(0.025),0.9); float g=abs(fract(vUV.y/(2.*PI)*70.)-0.5); if(vUV.x>0.295&&g<0.08) s.albedo=vec3(0.008); }
    else if(vMat==3) s=metalS(vec3(0.78,0.79,0.82),0.28);
    else if(vMat==16){
      float r=vUV.x;
      if(r>0.065&&r<0.186){
        float seg=2.*PI/5.; float a=mod(vUV.y+0.35,seg)-seg*0.5;
        float w=mix(0.016,0.040,smoothstep(0.07,0.18,r));
        if(abs(a)*r>w) discard;
      }
      s=metalS(vec3(0.78,0.79,0.82),0.25); if(r<0.022) s=plastic(vec3(0.03),0.5);
    }
    else if(vMat==4) s=metalS(vec3(0.9),0.06);
    else if(vMat==5) s=plastic(vec3(0.02),0.5);
    else if(vMat==8) s=plastic(vec3(0.045),0.7);
    else if(vMat==13) s=mk(vec3(0.35,0.35,0.37),0.9,0.5,0.);
    else if(vMat==14) s=plastic(vec3(0.36,0.33,0.27),0.95);
    else if(vMat==15) s=paint();
    else if(vMat==9){
      s=plastic(vec3(0.92),0.45);
      if(abs(N.x)>0.6){ float t=texture(uTex,vec2(vUV.x,(1.-vUV.y)*0.5)).r; s.albedo=mix(vec3(0.92),vec3(0.02,0.02,0.05),t); }
    }
    else if(vMat==10){ s=mk(vec3(0.05,0.2,0.95),0.,0.12,1.); s.emis=vec3(0.02,0.08,0.4); }
    else if(vMat==11){ s=mk(vec3(1.,0.55,0.02),0.,0.12,1.); s.emis=vec3(0.4,0.18,0.); }
    else if(vMat==12){ s=mk(vec3(0.9,0.04,0.03),0.,0.12,1.); s.emis=vec3(0.35,0.02,0.01); }
    else s=plastic(vec3(0.5),0.5);
    ao=mix(0.3,1.,smoothstep(0.05,0.6,P.y));
    col=shade(s,N,V,ao);
  }
  frag=vec4(pow(aces(col*uExposure),vec3(1./2.2)),1.);
}
)";

// Owns a linked GLSL program and caches its uniform locations.
class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram() { destroy(); }
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    bool build(const char* vertexSource, const char* fragmentSource);
    bool buildCarShader(const char* common, const char* vertexSource, const char* fragmentSource);

    void use() const { glUseProgram(id_); }
    GLuint id() const { return id_; }
    GLint uniformLocation(const char* name) const { return glGetUniformLocation(id_, name); }

    void setMVP(const glm::mat4& matrix) const {
        glUniformMatrix4fv(uMVP_, 1, GL_FALSE, glm::value_ptr(matrix));
    }
    void setModel(const glm::mat4& matrix) const {
        glUniformMatrix4fv(uModel_, 1, GL_FALSE, glm::value_ptr(matrix));
    }
    void setColor(const glm::vec3& color) const {
        glUniform3fv(uColor_, 1, glm::value_ptr(color));
    }
    void setLight(const glm::vec3& direction) const {
        glUniform3fv(uLight_, 1, glm::value_ptr(direction));
    }
    void setCamPos(const glm::vec3& camPos) const {
        if (uCamPos_ != -1) {
            glUniform3fv(uCamPos_, 1, glm::value_ptr(camPos));
        }
    }

    void destroy() {
        if (id_ != 0) glDeleteProgram(id_);
        id_ = 0;
        uMVP_ = uModel_ = uColor_ = uLight_ = uCamPos_ = -1;
    }

private:
    static GLuint compile(GLenum type, const char* source);

    GLuint id_ = 0;
    GLint uMVP_ = -1;
    GLint uModel_ = -1;
    GLint uColor_ = -1;
    GLint uLight_ = -1;
    GLint uCamPos_ = -1;
};

inline GLuint ShaderProgram::compile(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE) {
        char log[2048] = {};
        glGetShaderInfoLog(shader, static_cast<GLsizei>(sizeof(log)), nullptr, log);
        std::fprintf(stderr, "Shader compilation failed: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

inline bool ShaderProgram::build(const char* vertexSource, const char* fragmentSource) {
    destroy();
    const GLuint vertexShader = compile(GL_VERTEX_SHADER, vertexSource);
    const GLuint fragmentShader = compile(GL_FRAGMENT_SHADER, fragmentSource);
    if (vertexShader == 0 || fragmentShader == 0) {
        if (vertexShader != 0) glDeleteShader(vertexShader);
        if (fragmentShader != 0) glDeleteShader(fragmentShader);
        return false;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success != GL_TRUE) {
        char log[2048] = {};
        glGetProgramInfoLog(program, static_cast<GLsizei>(sizeof(log)), nullptr, log);
        std::fprintf(stderr, "Shader program linking failed: %s\n", log);
        glDeleteProgram(program);
        return false;
    }

    id_ = program;
    uMVP_ = glGetUniformLocation(id_, "uMVP");
    uModel_ = glGetUniformLocation(id_, "uModel");
    uColor_ = glGetUniformLocation(id_, "uColor");
    uLight_ = glGetUniformLocation(id_, "uLight");
    uCamPos_ = glGetUniformLocation(id_, "uCamPos");
    return true;
}

// Builds the advanced car program. The common block (sky / noise helpers) is
// prepended to the fragment shader only; the vertex shader is standalone.
// A `#version 330 core` directive is prepended to both stages.
inline bool ShaderProgram::buildCarShader(const char* common, const char* vertexSource,
                                          const char* fragmentSource) {
    destroy();
    const std::string version = "#version 330 core\n";
    const std::string vs = version + vertexSource;
    const std::string fs = version + common + fragmentSource;
    const GLuint vertexShader = compile(GL_VERTEX_SHADER, vs.c_str());
    const GLuint fragmentShader = compile(GL_FRAGMENT_SHADER, fs.c_str());
    if (vertexShader == 0 || fragmentShader == 0) {
        if (vertexShader != 0) glDeleteShader(vertexShader);
        if (fragmentShader != 0) glDeleteShader(fragmentShader);
        return false;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success != GL_TRUE) {
        char log[2048] = {};
        glGetProgramInfoLog(program, static_cast<GLsizei>(sizeof(log)), nullptr, log);
        std::fprintf(stderr, "Car shader program linking failed: %s\n", log);
        glDeleteProgram(program);
        return false;
    }

    id_ = program;
    return true;
}

// Owns the VAO/VBO/EBO triplet of a single indexed mesh.
class GpuMesh {
public:
    GpuMesh() = default;
    ~GpuMesh() { destroy(); }
    GpuMesh(const GpuMesh&) = delete;
    GpuMesh& operator=(const GpuMesh&) = delete;

    void upload(const car::Mesh& mesh) {
        destroy();
        indexCount_ = static_cast<GLsizei>(mesh.i.size());
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glGenBuffers(1, &ebo_);
        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(mesh.v.size() * sizeof(car::Vertex)),
                     mesh.v.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(mesh.i.size() * sizeof(unsigned)),
                     mesh.i.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(car::Vertex),
                              reinterpret_cast<void*>(offsetof(car::Vertex, p)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(car::Vertex),
                              reinterpret_cast<void*>(offsetof(car::Vertex, n)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(car::Vertex),
                              reinterpret_cast<void*>(offsetof(car::Vertex, uv)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(car::Vertex),
                              reinterpret_cast<void*>(offsetof(car::Vertex, mat)));
        glBindVertexArray(0);
    }

    void draw() const {
        glBindVertexArray(vao_);
        glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    }

    GLsizei indexCount() const { return indexCount_; }

    void destroy() {
        if (ebo_ != 0) glDeleteBuffers(1, &ebo_);
        if (vbo_ != 0) glDeleteBuffers(1, &vbo_);
        if (vao_ != 0) glDeleteVertexArrays(1, &vao_);
        vao_ = vbo_ = ebo_ = 0;
        indexCount_ = 0;
    }

private:
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ebo_ = 0;
    GLsizei indexCount_ = 0;
};

// Unit cube: 1 x 1 footprint centred on the origin, extruded from y = 0 to y = 1.
// 24 vertices with distinct flat normals per face for realistic 3D volume shading.
inline car::Mesh makeUnitBoxMesh() {
    car::Mesh mesh;
    struct FaceDef {
        glm::vec3 normal;
        glm::vec3 corners[4];
    };
    const FaceDef faces[6] = {
        // Top (+Y)
        { {0.0f, 1.0f, 0.0f}, {{-0.5f, 1.0f,  0.5f}, { 0.5f, 1.0f,  0.5f}, { 0.5f, 1.0f, -0.5f}, {-0.5f, 1.0f, -0.5f}} },
        // Bottom (-Y)
        { {0.0f, -1.0f, 0.0f}, {{-0.5f, 0.0f, -0.5f}, { 0.5f, 0.0f, -0.5f}, { 0.5f, 0.0f,  0.5f}, {-0.5f, 0.0f,  0.5f}} },
        // Front (+Z)
        { {0.0f, 0.0f, 1.0f}, {{-0.5f, 0.0f,  0.5f}, { 0.5f, 0.0f,  0.5f}, { 0.5f, 1.0f,  0.5f}, {-0.5f, 1.0f,  0.5f}} },
        // Back (-Z)
        { {0.0f, 0.0f, -1.0f}, {{ 0.5f, 0.0f, -0.5f}, {-0.5f, 0.0f, -0.5f}, {-0.5f, 1.0f, -0.5f}, { 0.5f, 1.0f, -0.5f}} },
        // Right (+X)
        { {1.0f, 0.0f, 0.0f}, {{ 0.5f, 0.0f,  0.5f}, { 0.5f, 0.0f, -0.5f}, { 0.5f, 1.0f, -0.5f}, { 0.5f, 1.0f,  0.5f}} },
        // Left (-X)
        { {-1.0f, 0.0f, 0.0f}, {{-0.5f, 0.0f, -0.5f}, {-0.5f, 0.0f,  0.5f}, {-0.5f, 1.0f,  0.5f}, {-0.5f, 1.0f, -0.5f}} }
    };

    for (const auto& f : faces) {
        const unsigned base = static_cast<unsigned>(mesh.v.size());
        for (int i = 0; i < 4; ++i) {
            mesh.v.push_back({f.corners[i], f.normal, glm::vec2(0.0f), 0.0f});
        }
        mesh.i.push_back(base + 0);
        mesh.i.push_back(base + 1);
        mesh.i.push_back(base + 2);
        mesh.i.push_back(base + 0);
        mesh.i.push_back(base + 2);
        mesh.i.push_back(base + 3);
    }
    return mesh;
}

// Unit cylinder: along Y axis from y = 0 to y = 1, diameter = 1.0 (radius = 0.5).
// Smooth cylindrical normals around the radius give realistic 3D curved sheen and depth.
inline car::Mesh makeUnitCylinderMesh(int slices = 16) {
    car::Mesh mesh;
    const float radius = 0.5f;
    const float pi = 3.14159265358979323846f;

    // Side vertices: 2 rows (bottom y=0, top y=1)
    for (int i = 0; i <= slices; ++i) {
        const float theta = static_cast<float>(i) * (2.0f * pi / static_cast<float>(slices));
        const float cosT = std::cos(theta);
        const float sinT = std::sin(theta);
        const glm::vec3 normal(cosT, 0.0f, sinT);

        // Bottom vertex
        mesh.v.push_back({glm::vec3(radius * cosT, 0.0f, radius * sinT), normal, glm::vec2(static_cast<float>(i) / slices, 0.0f), 0.0f});
        // Top vertex
        mesh.v.push_back({glm::vec3(radius * cosT, 1.0f, radius * sinT), normal, glm::vec2(static_cast<float>(i) / slices, 1.0f), 0.0f});
    }

    // Side indices
    for (int i = 0; i < slices; ++i) {
        const unsigned b0 = i * 2;
        const unsigned t0 = b0 + 1;
        const unsigned b1 = (i + 1) * 2;
        const unsigned t1 = b1 + 1;

        mesh.i.push_back(b0);
        mesh.i.push_back(b1);
        mesh.i.push_back(t1);

        mesh.i.push_back(b0);
        mesh.i.push_back(t1);
        mesh.i.push_back(t0);
    }

    // Bottom cap (normal points -Y)
    const unsigned botCenterIdx = static_cast<unsigned>(mesh.v.size());
    mesh.v.push_back({glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.5f, 0.5f), 0.0f});
    const unsigned botRimStart = static_cast<unsigned>(mesh.v.size());
    for (int i = 0; i <= slices; ++i) {
        const float theta = static_cast<float>(i) * (2.0f * pi / static_cast<float>(slices));
        mesh.v.push_back({glm::vec3(radius * std::cos(theta), 0.0f, radius * std::sin(theta)),
                          glm::vec3(0.0f, -1.0f, 0.0f),
                          glm::vec2(0.5f + 0.5f * std::cos(theta), 0.5f + 0.5f * std::sin(theta)), 0.0f});
    }
    for (int i = 0; i < slices; ++i) {
        mesh.i.push_back(botCenterIdx);
        mesh.i.push_back(botRimStart + i + 1);
        mesh.i.push_back(botRimStart + i);
    }

    // Top cap (normal points +Y)
    const unsigned topCenterIdx = static_cast<unsigned>(mesh.v.size());
    mesh.v.push_back({glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.5f, 0.5f), 0.0f});
    const unsigned topRimStart = static_cast<unsigned>(mesh.v.size());
    for (int i = 0; i <= slices; ++i) {
        const float theta = static_cast<float>(i) * (2.0f * pi / static_cast<float>(slices));
        mesh.v.push_back({glm::vec3(radius * std::cos(theta), 1.0f, radius * std::sin(theta)),
                          glm::vec3(0.0f, 1.0f, 0.0f),
                          glm::vec2(0.5f + 0.5f * std::cos(theta), 0.5f + 0.5f * std::sin(theta)), 0.0f});
    }
    for (int i = 0; i < slices; ++i) {
        mesh.i.push_back(topCenterIdx);
        mesh.i.push_back(topRimStart + i);
        mesh.i.push_back(topRimStart + i + 1);
    }

    return mesh;
}

// Flat unit quad on the XZ plane (y = 0), normal +Y. No thickness and no side
// faces, so it always reads as a painted-on decal rather than a raised strip.
inline car::Mesh makeUnitXZQuadMesh() {
    car::Mesh mesh;
    const glm::vec3 n(0.0f, 1.0f, 0.0f);
    mesh.v.push_back({{-0.5f, 0.0f, -0.5f}, n, {0.0f, 0.0f}, 0.0f});
    mesh.v.push_back({{ 0.5f, 0.0f, -0.5f}, n, {1.0f, 0.0f}, 0.0f});
    mesh.v.push_back({{ 0.5f, 0.0f,  0.5f}, n, {1.0f, 1.0f}, 0.0f});
    mesh.v.push_back({{-0.5f, 0.0f,  0.5f}, n, {0.0f, 1.0f}, 0.0f});
    mesh.i.push_back(0); mesh.i.push_back(1); mesh.i.push_back(2);
    mesh.i.push_back(0); mesh.i.push_back(2); mesh.i.push_back(3);
    return mesh;
}

// Low-level drawing front end shared by the scene and the HUD.
// Supports optional box batching: consecutive drawBox calls with the same
// colour are merged into a single dynamic mesh submission, cutting draw-call
// count from hundreds/thousands down to a handful per frame.
class Renderer {
public:
    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool initialise() {
        if (!program_.build(kVertexShaderSource, kFragmentShaderSource)) return false;
        if (!carProgram_.buildCarShader(kCarCommonShaderSource, kCarVertexShaderSource,
                                        kCarFragmentShaderSource)) {
            program_.destroy();
            return false;
        }
        createTextAtlas();
        boxMesh_.upload(makeUnitBoxMesh());
        cylinderMesh_.upload(makeUnitCylinderMesh(16));
        quadMesh_.upload(makeUnitXZQuadMesh());
        carMesh_.upload(car::buildCar());

        // Dynamic batch buffers for merged box draws.
        glGenVertexArrays(1, &batchVao_);
        glGenBuffers(1, &batchVbo_);
        glGenBuffers(1, &batchEbo_);
        glBindVertexArray(batchVao_);
        glBindBuffer(GL_ARRAY_BUFFER, batchVbo_);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batchEbo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(car::Vertex),
                              reinterpret_cast<void*>(offsetof(car::Vertex, p)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(car::Vertex),
                              reinterpret_cast<void*>(offsetof(car::Vertex, n)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(car::Vertex),
                              reinterpret_cast<void*>(offsetof(car::Vertex, uv)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(car::Vertex),
                              reinterpret_cast<void*>(offsetof(car::Vertex, mat)));
        glBindVertexArray(0);

        uCarMVP_ = carProgram_.uniformLocation("uMVP");
        uCarCam_ = carProgram_.uniformLocation("uCam");
        uCarSun_ = carProgram_.uniformLocation("uSun");
        uCarBase_ = carProgram_.uniformLocation("uBaseColor");
        uCarPass_ = carProgram_.uniformLocation("uPass");
        uCarExp_ = carProgram_.uniformLocation("uExposure");
        uCarTex_ = carProgram_.uniformLocation("uTex");
        return true;
    }

    void shutdown() {
        flushBatch();
        if (batchEbo_ != 0) glDeleteBuffers(1, &batchEbo_);
        if (batchVbo_ != 0) glDeleteBuffers(1, &batchVbo_);
        if (batchVao_ != 0) glDeleteVertexArrays(1, &batchVao_);
        batchVao_ = batchVbo_ = batchEbo_ = 0;
        boxMesh_.destroy();
        cylinderMesh_.destroy();
        quadMesh_.destroy();
        carMesh_.destroy();
        program_.destroy();
        carProgram_.destroy();
        if (textAtlas_ != 0) { glDeleteTextures(1, &textAtlas_); textAtlas_ = 0; }
    }

    void beginFrame(const glm::mat4& viewProjection, const glm::vec3& lightDirection,
                    const glm::vec3& cameraPosition) {
        flushBatch();
        program_.use();
        program_.setLight(lightDirection);
        program_.setCamPos(cameraPosition);
        viewProjection_ = viewProjection;
        lightDir_ = glm::normalize(lightDirection);
        cameraPos_ = cameraPosition;
        batchingEnabled_ = true;
        lastColorValid_ = false;
    }

    // Call at the end of the 3D scene (before HUD) to submit any remaining
    // batched boxes. Batching stays enabled so the HUD also benefits.
    void endScene() {
        flushBatch();
    }

    void setViewProjection(const glm::mat4& viewProjection) {
        flushBatch();
        viewProjection_ = viewProjection;
    }
    const glm::mat4& viewProjection() const { return viewProjection_; }

    void draw(const GpuMesh& mesh, const glm::mat4& model, const glm::vec3& color) const {
        flushBatch();
        program_.use();
        program_.setMVP(viewProjection_ * model);
        program_.setModel(model);
        program_.setColor(color);
        mesh.draw();
        lastColorValid_ = false;
    }

    void drawBox(const glm::vec3& bottomCenter, const glm::vec3& size,
                 const glm::vec3& color) const {
        if (batchingEnabled_) {
            appendBoxToBatch(bottomCenter, size, color);
            return;
        }
        const glm::mat4 model = glm::translate(glm::mat4(1.0f), bottomCenter) *
                                glm::scale(glm::mat4(1.0f), size);
        program_.use();
        program_.setMVP(viewProjection_ * model);
        program_.setModel(model);
        if (!lastColorValid_ || lastColor_ != color) {
            program_.setColor(color);
            lastColor_ = color;
            lastColorValid_ = true;
        }
        boxMesh_.draw();
    }

    void drawOrientedBox(const glm::vec3& center, const glm::vec3& size, float rotationZ,
                         const glm::vec3& color) const {
        flushBatch();
        const glm::mat4 model = glm::translate(glm::mat4(1.0f), center) *
                                glm::rotate(glm::mat4(1.0f), rotationZ, glm::vec3(0.0f, 0.0f, 1.0f)) *
                                glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -size.y * 0.5f, 0.0f)) *
                                glm::scale(glm::mat4(1.0f), size);
        program_.use();
        program_.setMVP(viewProjection_ * model);
        program_.setModel(model);
        program_.setColor(color);
        boxMesh_.draw();
        lastColorValid_ = false;
    }

    // Flat rectangle lying on the XZ plane at the given Y, used for painted-on
    // floor markings. A single quad with no side faces means it never reads as
    // a raised strip. `center` is the centre point; `sizeXZ` is (widthX, widthZ).
    void drawFlatXZ(const glm::vec3& center, const glm::vec2& sizeXZ,
                    const glm::vec3& color) const {
        flushBatch();
        const glm::mat4 model =
            glm::translate(glm::mat4(1.0f), center) *
            glm::scale(glm::mat4(1.0f), glm::vec3(sizeXZ.x, 1.0f, sizeXZ.y));
        program_.use();
        program_.setMVP(viewProjection_ * model);
        program_.setModel(model);
        program_.setColor(color);
        quadMesh_.draw();
        lastColorValid_ = false;
    }

    // Flat rectangle on the XZ plane rotated around Y (e.g. angled arrows).
    void drawFlatXZRotated(const glm::vec3& center, const glm::vec2& sizeXZ,
                           float yaw, const glm::vec3& color) const {
        flushBatch();
        const glm::mat4 model =
            glm::translate(glm::mat4(1.0f), center) *
            glm::rotate(glm::mat4(1.0f), yaw, glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(sizeXZ.x, 1.0f, sizeXZ.y));
        program_.use();
        program_.setMVP(viewProjection_ * model);
        program_.setModel(model);
        program_.setColor(color);
        quadMesh_.draw();
        lastColorValid_ = false;
    }

    const GpuMesh& quadMesh() const { return quadMesh_; }

    // Draws the car with the advanced PBR-ish shader. The camera and light are
    // transformed into the car's local frame so the procedural body details
    // (door lines, lamps, grille, sign text) stay aligned with the mesh.
    // Two passes: opaque surfaces first, then the glass with blending.
    void drawCar(const glm::mat4& model, const glm::vec3& color) const {
        flushBatch();
        // Yaw-only cars: model is T * R_y, so the 3x3 is a pure rotation and
        // its inverse equals its transpose – cheaper than glm::inverse.
        const glm::mat3 modelRot = glm::mat3(model);
        const glm::mat3 invRot = glm::transpose(modelRot);
        const glm::vec3 localCam = invRot * (cameraPos_ - glm::vec3(model[3]));
        const glm::vec3 localSun = glm::normalize(invRot * lightDir_);

        carProgram_.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textAtlas_);
        glUniform1i(uCarTex_, 0);
        glUniformMatrix4fv(uCarMVP_, 1, GL_FALSE, glm::value_ptr(viewProjection_ * model));
        glUniform3fv(uCarCam_, 1, glm::value_ptr(localCam));
        glUniform3fv(uCarSun_, 1, glm::value_ptr(localSun));
        glUniform3fv(uCarBase_, 1, glm::value_ptr(color));
        glUniform1f(uCarExp_, 0.95f);

        // Opaque pass.
        glUniform1i(uCarPass_, 0);
        carMesh_.draw();

        // Glass pass (blended).
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glUniform1i(uCarPass_, 1);
        carMesh_.draw();
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        program_.use();
        lastColorValid_ = false;
    }

    // Draws any car type. `model` is T * R_y like for drawCar(); `bodyColor`
    // paints the body panels (the Vios shader and the part tables both use it).
    void drawVehicle(vehicle_models::Model type, const glm::mat4& model,
                     const glm::vec3& bodyColor) const {
        const std::vector<vehicle_models::Part>& parts = vehicle_models::partsFor(type);
        if (parts.empty()) {
            drawCar(model, bodyColor);   // Toyota Vios: dedicated lofted mesh
            return;
        }
        for (const vehicle_models::Part& part : parts) {
            const glm::vec3& color = part.useBodyColor ? bodyColor : part.color;
            if (part.kind == vehicle_models::PartKind::Cylinder) {
                drawPipe(glm::vec3(model * glm::vec4(part.a, 1.0f)),
                         glm::vec3(model * glm::vec4(part.b, 1.0f)), part.radius, color);
                continue;
            }
            const glm::mat4 partModel =
                model * glm::translate(glm::mat4(1.0f), part.a) *
                glm::rotate(glm::mat4(1.0f), part.tiltZ, glm::vec3(0.0f, 0.0f, 1.0f)) *
                glm::scale(glm::mat4(1.0f), part.b);
            draw(boxMesh_, partModel, color);
        }
    }

    // Draws a 3D cylindrical pipe between two arbitrary world points.
    void drawPipe(const glm::vec3& start, const glm::vec3& end, float radius,
                  const glm::vec3& color) const {
        flushBatch();
        const glm::vec3 dir = end - start;
        const float len = glm::length(dir);
        if (len < 1e-4f) return;

        const glm::vec3 unitDir = dir / len;
        const glm::vec3 up(0.0f, 1.0f, 0.0f);

        glm::mat4 model(1.0f);
        model = glm::translate(model, start);

        const float dotVal = glm::dot(up, unitDir);
        if (std::abs(dotVal - 1.0f) < 1e-5f) {
            // Collinear with +Y
        } else if (std::abs(dotVal + 1.0f) < 1e-5f) {
            // Collinear with -Y
            model = glm::rotate(model, glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
        } else {
            const glm::vec3 rotAxis = glm::normalize(glm::cross(up, unitDir));
            const float angle = std::acos(glm::clamp(dotVal, -1.0f, 1.0f));
            model = glm::rotate(model, angle, rotAxis);
        }

        // Unit cylinder has diameter = 1.0 (radius = 0.5) and height = 1.0
        model = glm::scale(model, glm::vec3(radius * 2.0f, len, radius * 2.0f));

        program_.use();
        program_.setMVP(viewProjection_ * model);
        program_.setModel(model);
        program_.setColor(color);
        cylinderMesh_.draw();
        lastColorValid_ = false;
    }

    // Convenient pipe along X axis, centered at `center` with length `length`
    void drawPipeX(const glm::vec3& center, float length, float radius, const glm::vec3& color) const {
        drawPipe(center - glm::vec3(length * 0.5f, 0.0f, 0.0f),
                 center + glm::vec3(length * 0.5f, 0.0f, 0.0f), radius, color);
    }

    // Convenient pipe along Y axis, from `bottomCenter` with height `height`
    void drawPipeY(const glm::vec3& bottomCenter, float height, float radius, const glm::vec3& color) const {
        drawPipe(bottomCenter, bottomCenter + glm::vec3(0.0f, height, 0.0f), radius, color);
    }

    // Convenient pipe along Z axis, centered at `center` with length `length`
    void drawPipeZ(const glm::vec3& center, float length, float radius, const glm::vec3& color) const {
        drawPipe(center - glm::vec3(0.0f, 0.0f, length * 0.5f),
                 center + glm::vec3(0.0f, 0.0f, length * 0.5f), radius, color);
    }

    const GpuMesh& boxMesh() const { return boxMesh_; }
    const GpuMesh& cylinderMesh() const { return cylinderMesh_; }
    const GpuMesh& carMesh() const { return carMesh_; }

private:
    void appendBoxToBatch(const glm::vec3& bottomCenter, const glm::vec3& size,
                          const glm::vec3& color) const {
        // Flush when colour changes so we keep one uniform set per batch.
        if (hasBatchColor_ && batchColor_ != color) {
            flushBatch();
        }
        batchColor_ = color;
        hasBatchColor_ = true;

        const float hx = size.x * 0.5f;
        const float hy = size.y;
        const float hz = size.z * 0.5f;
        const float cx = bottomCenter.x;
        const float by = bottomCenter.y;
        const float cz = bottomCenter.z;

        struct BatchFace {
            glm::vec3 normal;
            glm::vec3 p[4];
        };

        const BatchFace faces[6] = {
            // Top (+Y)
            { {0.0f, 1.0f, 0.0f}, {{cx - hx, by + hy, cz + hz}, {cx + hx, by + hy, cz + hz}, {cx + hx, by + hy, cz - hz}, {cx - hx, by + hy, cz - hz}} },
            // Bottom (-Y)
            { {0.0f, -1.0f, 0.0f}, {{cx - hx, by, cz - hz}, {cx + hx, by, cz - hz}, {cx + hx, by, cz + hz}, {cx - hx, by, cz + hz}} },
            // Front (+Z)
            { {0.0f, 0.0f, 1.0f}, {{cx - hx, by, cz + hz}, {cx + hx, by, cz + hz}, {cx + hx, by + hy, cz + hz}, {cx - hx, by + hy, cz + hz}} },
            // Back (-Z)
            { {0.0f, 0.0f, -1.0f}, {{cx + hx, by, cz - hz}, {cx - hx, by, cz - hz}, {cx - hx, by + hy, cz - hz}, {cx + hx, by + hy, cz - hz}} },
            // Right (+X)
            { {1.0f, 0.0f, 0.0f}, {{cx + hx, by, cz + hz}, {cx + hx, by, cz - hz}, {cx + hx, by + hy, cz - hz}, {cx + hx, by + hy, cz + hz}} },
            // Left (-X)
            { {-1.0f, 0.0f, 0.0f}, {{cx - hx, by, cz - hz}, {cx - hx, by, cz + hz}, {cx - hx, by + hy, cz + hz}, {cx - hx, by + hy, cz - hz}} }
        };

        for (const auto& f : faces) {
            const unsigned base = static_cast<unsigned>(batchVerts_.size());
            for (int i = 0; i < 4; ++i) {
                car::Vertex v;
                v.p = f.p[i];
                v.n = f.normal;
                v.uv = glm::vec2(0.0f);
                v.mat = 0.0f;
                batchVerts_.push_back(v);
            }
            batchIndices_.push_back(base + 0);
            batchIndices_.push_back(base + 1);
            batchIndices_.push_back(base + 2);
            batchIndices_.push_back(base + 0);
            batchIndices_.push_back(base + 2);
            batchIndices_.push_back(base + 3);
        }

        // Cap batch size to avoid huge uploads on pathological frames.
        if (batchIndices_.size() > 45000) {
            flushBatch();
        }
    }

    void flushBatch() const {
        if (batchVerts_.empty()) {
            hasBatchColor_ = false;
            return;
        }
        program_.use();
        // Identity model: vertices are already in world space.
        const glm::mat4 identity(1.0f);
        program_.setMVP(viewProjection_);
        program_.setModel(identity);
        program_.setColor(batchColor_);

        glBindVertexArray(batchVao_);
        glBindBuffer(GL_ARRAY_BUFFER, batchVbo_);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(batchVerts_.size() * sizeof(car::Vertex)),
                     batchVerts_.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batchEbo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(batchIndices_.size() * sizeof(unsigned)),
                     batchIndices_.data(), GL_DYNAMIC_DRAW);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(batchIndices_.size()),
                       GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);

        batchVerts_.clear();
        batchIndices_.clear();
        hasBatchColor_ = false;
        lastColorValid_ = false;
    }

    // 5x7 bitmap font used to bake the roof sign and number plate into an R8
    // texture atlas (top half = sign, bottom half = number plate).
    static const char* glyph(char c) {
        switch (c) {
        case 'D': return "####." "#...#" "#...#" "#...#" "#...#" "#...#" "####.";
        case 'R': return "####." "#...#" "#...#" "####." "#.#.." "#..#." "#...#";
        case 'I': return "#####" "..#.." "..#.." "..#.." "..#.." "..#.." "#####";
        case 'V': return "#...#" "#...#" "#...#" "#...#" "#...#" ".#.#." "..#..";
        case 'N': return "#...#" "##..#" "#.#.#" "#..##" "#...#" "#...#" "#...#";
        case 'G': return ".###." "#...#" "#...." "#.###" "#...#" "#...#" ".###.";
        case 'L': return "#...." "#...." "#...." "#...." "#...." "#...." "#####";
        case 'C': return ".###." "#...#" "#...." "#...." "#...." "#...#" ".###.";
        case 'E': return "#####" "#...." "#...." "####." "#...." "#...." "#####";
        case 'S': return ".####" "#...." "#...." ".###." "....#" "....#" "####.";
        case 'T': return "#####" "..#.." "..#.." "..#.." "..#.." "..#.." "..#..";
        case 'Z': return "#####" "....#" "...#." "..#.." ".#..." "#...." "#####";
        case '0': return ".###." "#...#" "#..##" "#.#.#" "##..#" "#...#" ".###.";
        case '2': return ".###." "#...#" "....#" "...#." "..#.." ".#..." "#####";
        case '3': return "####." "....#" "....#" ".###." "....#" "....#" "####.";
        case '5': return "#####" "#...." "####." "....#" "....#" "#...#" ".###.";
        case '6': return ".###." "#...." "#...." "####." "#...#" "#...#" ".###.";
        case '-': return "....." "....." "....." "#####" "....." "....." ".....";
        default:  return "....." "....." "....." "....." "....." "....." ".....";
        }
    }

    void createTextAtlas() {
        const int W = 128, H = 64;
        std::vector<unsigned char> atlas(static_cast<size_t>(W * H), 0);
        auto drawText = [&](const char* txt, int x0, int y0, int sc) {
            for (const char* p = txt; *p; p++, x0 += 6 * sc) {
                const char* g = glyph(*p);
                for (int r = 0; r < 7; r++)
                    for (int c = 0; c < 5; c++)
                        if (g[r * 5 + c] == '#')
                            for (int dy = 0; dy < sc; dy++)
                                for (int dx = 0; dx < sc; dx++)
                                    atlas[(y0 + r * sc + dy) * W + x0 + c * sc + dx] = 255;
            }
        };
        drawText("DRIVING LICENSE TEST", 4, 12, 1);
        drawText("30Z-5226", 17, 32 + 9, 2);

        glGenTextures(1, &textAtlas_);
        glBindTexture(GL_TEXTURE_2D, textAtlas_);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, W, H, 0, GL_RED, GL_UNSIGNED_BYTE, atlas.data());
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    ShaderProgram program_;
    ShaderProgram carProgram_;
    GpuMesh boxMesh_;
    GpuMesh cylinderMesh_;
    GpuMesh quadMesh_;
    GpuMesh carMesh_;
    GLuint textAtlas_ = 0;
    glm::mat4 viewProjection_{1.0f};
    glm::vec3 lightDir_{0.0f, 1.0f, 0.0f};
    glm::vec3 cameraPos_{0.0f};
    GLint uCarMVP_ = -1;
    GLint uCarCam_ = -1;
    GLint uCarSun_ = -1;
    GLint uCarBase_ = -1;
    GLint uCarPass_ = -1;
    GLint uCarExp_ = -1;
    GLint uCarTex_ = -1;

    // Dynamic box-batch state (world-space vertices, one draw per colour run).
    // Mutable so const draw paths (scene helpers) can accumulate/flush batches.
    GLuint batchVao_ = 0;
    GLuint batchVbo_ = 0;
    GLuint batchEbo_ = 0;
    mutable std::vector<car::Vertex> batchVerts_;
    mutable std::vector<unsigned> batchIndices_;
    mutable glm::vec3 batchColor_{1.0f};
    mutable bool hasBatchColor_ = false;
    bool batchingEnabled_ = false;
    mutable glm::vec3 lastColor_{1.0f};
    mutable bool lastColorValid_ = false;
};
