#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 color;
out vec3 crntPos;

uniform mat4 cam;

void main()
{
  crntPos = aPos;
  gl_Position = cam*vec4(aPos, 1.0);
  color = aColor;
}
