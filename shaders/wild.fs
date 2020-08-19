#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D tex;
uniform mat4 shear;


void main()
{
    FragColor = texture(tex, TexCoords) * shear * 1.5;
}