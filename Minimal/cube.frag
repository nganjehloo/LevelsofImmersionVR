#version 330 core

in vec2 TexCoords;
uniform sampler2D textureSampler;
out vec3 color;

void main()
{
	color = texture(textureSampler, TexCoords).rgb;
}