#version 410 core

in vec3 textureCoordinates;
out vec4 color;

uniform samplerCube skybox;
uniform float fogDensity;

void main()
{
	if (fogDensity == 0)
		color = texture(skybox, textureCoordinates);
	else
		color = vec4(0.5f, 0.5f, 0.5f, 1.0f);
}
