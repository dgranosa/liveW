#version 430
layout (location = 0) in vec4 vertex;
out vec2 TexCoords;

void main()
{
    vec4 pos = vec4(vertex.xy, 0.0, 1.0);
    gl_Position = pos;
    TexCoords = vertex.zw;
}
