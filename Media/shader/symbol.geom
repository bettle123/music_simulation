#version 330 core 
// Fragment Shader – file "symbol.geom"
// precision highp float;
layout (points) in;
layout (triangle_strip) out;
layout (max_vertices = 4) out;

in vec4 color[1];
out vec4 brushColor;
out vec2 texCoord;
void main(void)
{
	const float delta = 0.12f;
	brushColor = color[0];
	vec4 center = gl_in[0] .gl_Position;
	float left = center.x - delta;
	float right = center.x + delta;
	float bottom = center.y - delta;
	float top = center.y + delta;
	gl_Position = vec4(left, bottom, center.z, center.w);
	texCoord.x = 0;
	texCoord.y = 0;
	EmitVertex() ;
	gl_Position = vec4(right, bottom, center.z, center.w);
	texCoord.x = 1;
	EmitVertex() ;
	gl_Position = vec4(left, top, center.z, center.w);
	texCoord.x = 0;
	texCoord.y = 1;
	EmitVertex() ;
	gl_Position = vec4(right, top, center.z, center.w);
	texCoord.x = 1;
	EmitVertex() ;
	EndPrimitive() ;
}