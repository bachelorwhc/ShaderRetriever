#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#define MAX_LIGHT 16

struct Light {
	vec4 position;
	vec4 color;
	vec3 atten_factor;
	float intensity;
};

layout(set=1, binding=0) uniform Scene {
	int light_size;
} scene;

layout(set=1, binding=1) uniform Lights {
	Light lights[MAX_LIGHT];
};

layout(set=1, binding=2) uniform sampler2D sampler_color;
layout(location=0) in vec4 in_normal;
layout(location=1) in vec4 in_pos;
layout(location=2) in vec2 in_uv;
layout(location=3) in mat4 in_world;
layout(location=0) out vec4 frag_color;

void main() 
{
	vec4 diffuse = texture(sampler_color, in_uv, 0.0f);
	vec4 world_pos = in_world * in_pos;
	for(int i = 0; i < scene.light_size; ++i) {
		vec4 diff_vec = lights[i].position - world_pos;
		float distance = length(diff_vec);
		vec4 light_dir = normalize(diff_vec);
		float s = max(dot(in_normal, light_dir), 0.0);
		float atten = lights[i].atten_factor.x * distance * distance + lights[i].atten_factor.y * distance;
		diffuse += lights[i].intensity * s * lights[i].color / atten;
	}
	frag_color = diffuse;
}