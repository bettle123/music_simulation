#version 330 core
// Fragment Shader – file "Gaussian.frag"

out vec4 out_Color;
in vec4 geomColor;

void main(void)
{
	const float size = 256.0f;
	const float sigma = 5.6f;
	float x = 2*gl_FragCoord.x / (size-1) - 1;
	float y = 2*(size - 1 - gl_FragCoord.y) / (size-1) -1;
	float r = -sigma * (x*x + y*y);
	float gaussian = exp(r);
	out_Color = geomColor;
	out_Color.a = gaussian;
	//out_Color.a = smoothstep(0.1,0.2,gaussian);
}