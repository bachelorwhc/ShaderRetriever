#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout(location=0) in vec3 in_pos;
layout(location=1) in vec3 in_normal;
layout(location=2) in vec2 in_uv;

layout(push_constant) uniform MVP {
    mat4 world;
	mat4 view;
	mat4 project;
} mvp;

layout (location=0) out vec4 out_normal;
layout (location=1) out vec4 out_pos;
layout (location=2) out vec2 out_uv;
layout (location=3) out mat4 out_world;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main() 
{
	out_uv = in_uv;
	out_pos = vec4(in_pos, 1.0f);
	out_world = mvp.world;
	mat4 normal_IT = transpose(inverse(mvp.world));
	out_normal = normalize(normal_IT * vec4(in_normal, 1.0f));
	gl_Position = mvp.project * mvp.view * mvp.world * vec4(in_pos.xyz, 1.0);
}
