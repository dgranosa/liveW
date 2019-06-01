#version 430

in vec2 TexCoords;
out vec4 color;

uniform sampler2D text;
uniform vec3 textColor;

void main()
{
    float p = texture(text, TexCoords).r;

    vec4 sampled = vec4(1.0, 1.0, 1.0, p);

    color = vec4(textColor, 1.0) * sampled;
}
