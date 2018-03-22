//Reference GLSL code for the PICA200 vertex shader in the source
#version 330 core
layout (location = 0) in vec3 epos;
uniform mat4 projection;
uniform float expand;
uniform vec3 base;

float mix(float src1, float src2, float blendfactor)
{
	return (src1 * (1-blendfactor)) + (src2 * blendfactor);
}

void main()
{
	vec3 outpos = vec3(0);
	float realexpand = expand/100.0f; //expand is within 0 to 100, we want within 0 to 1
	outpos.x = mix(base.x, epos.x, realexpand);
	outpos.y = mix(base.y, epos.y, realexpand);

	gl_Position = projection * vec4(outpos, 1.0);
}