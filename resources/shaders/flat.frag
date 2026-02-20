#version 330 core
out vec4 FragColor;

in vec3 crntPos;
uniform vec3 camPos;

vec4 fog(vec4 color){
    vec3 toCamera = camPos-crntPos;
    float camDist2 = dot(toCamera,toCamera);
    //float distBias = 1-clamp(camDist2/120.f,0,1);
    float distBias = 1-clamp((camDist2*camDist2)/300,0,1);
    vec4 bg_col1 = vec4(0,0,0,1.0);
    vec4 bg_col2 = vec4(0.529,0.808,0.922,1.0);
    vec4 bg_col3 = vec4(1,1,1,1.0);
    return color*distBias+(1-distBias)*bg_col3;
}

in vec3 color;
void main()
{
  FragColor = vec4(color, 1.0f);
}
