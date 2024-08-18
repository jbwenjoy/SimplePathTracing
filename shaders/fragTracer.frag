#version 330 core

// ====== varyings ======
// vert shader 中修改varying的值，供 frag shader 使用，两处的声明必须一致

varying vec4 vertex_light_position;
varying vec4 v;

// ====== uniforms ======

//uniform vec2 mouse;
uniform float time;
uniform int numTriangles;
uniform int numBalls;
uniform int numWater; // 水被注释掉了
uniform vec4 vertecies[100];
uniform int triangles[100];
uniform vec4 ball_pos[20];     // positions
uniform float ball_radius[20]; // radii
uniform mat4 cameraTransform;

uniform samplerCube skybox;
uniform bool        skybox_enabled;

uniform sampler2D   prevFrame;
uniform bool        prevFrame_enabled;
uniform float       prevFrame_ratio;
uniform bool        softDiffuse;

uniform vec3 water        [80]; // 水被注释掉了
uniform vec3 water_normals[80]; // 水被注释掉了
uniform float turbulent_min;
uniform float turbulent_max;
uniform bool water_enabled; // 水被注释掉了
uniform bool model_enabled;
uniform bool portal_enabled;
uniform bool refract_enabled;
uniform int shadowSamples;


void main()
{    

}