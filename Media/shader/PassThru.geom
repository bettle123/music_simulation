#version 330 core 

//precision highp float;
layout (triangles) in;
layout (triangle_strip) out;
layout (max_vertices = 3) out;

in vec4 color[3];
out vec4 geomColor;
void main(void)
{ 
	for (int i = 0; i < gl_in.length(); i++) {
		gl_Position = gl_in[i] .gl_Position;
		geomColor = color[i];
		EmitVertex() ;
	}
	EndPrimitive() ;
}