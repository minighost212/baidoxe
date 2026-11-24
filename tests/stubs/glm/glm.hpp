#pragma once
#include <cmath>
namespace glm {
struct vec2 {
    float x=0, y=0;
    vec2() = default;
    vec2(float a, float b):x(a),y(b){}
    vec2 operator/(float s) const { return {x/s,y/s}; }
};
inline vec2 operator-(const vec2&a,const vec2&b){return {a.x-b.x,a.y-b.y};}
inline float length(const vec2&v){return std::sqrt(v.x*v.x+v.y*v.y);}
struct vec3 {
    float x=0,y=0,z=0;
    vec3() = default;
    explicit vec3(float v):x(v),y(v),z(v){}
    vec3(float a,float b,float c):x(a),y(b),z(c){}
    vec3 operator+(const vec3&o) const{return {x+o.x,y+o.y,z+o.z};}
    vec3 operator-(const vec3&o) const{return {x-o.x,y-o.y,z-o.z};}
    vec3& operator+=(const vec3&o){x+=o.x;y+=o.y;z+=o.z;return *this;}
};
inline vec3 operator*(const vec3&v,float s){return {v.x*s,v.y*s,v.z*s};}
inline vec3 operator*(float s,const vec3&v){return v*s;}
inline float clamp(float v,float lo,float hi){return v<lo?lo:(v>hi?hi:v);}
}
