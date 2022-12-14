#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNorm;

out vec3 Normal;
out vec3 Color;
out vec3 crntPos;

uniform mat4 cam;

void main()
{
  Color = aColor;
  Normal = aNorm;
  crntPos = aPos;
  gl_Position = cam*vec4(aPos, 1.0);
}
