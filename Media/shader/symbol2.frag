#version 330 core
// Fragment Shader – file "symbol2.frag"

out vec4 out_Color;
in vec4 brushColor;
in vec2 texCoord;
uniform sampler2D brushTexture;

void main(void)
{
	//const float alpha = 0.6f;
	vec4 intensity = texture(brushTexture, texCoord.st);
	out_Color = intensity;
	//out_Color.a *= alpha;
}