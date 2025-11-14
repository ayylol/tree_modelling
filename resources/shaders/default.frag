#version 330 core
out vec4 FragColor;

in vec3 Color;
in vec3 Normal;
in vec3 crntPos;

uniform vec3 camPos;

struct Light{
  vec3 dir;
  float diff_strength;
  float spec_strength;
  int specular_pow;
};

vec4 directionalLight()
{
  struct Light lights[7] = struct Light[7](
    Light(normalize(vec3(0,1.0,0.2)),0.8,0.2,2), 
    Light(normalize(vec3(0,-1.0,0.0)),0.6,0.0,2), 
    Light(normalize(vec3(0,1.0,-0.2)),0.7,0.1,2),
    Light(normalize(vec3(1.0,0,0)),0.5,0.1,2),
    Light(normalize(vec3(-1.0,0,0)),0.4,0.1,2),
    Light(normalize(vec3(0.0,0,1.0)),0.2,0.1,2),
    Light(normalize(vec3(0.0,0,-1.0)),0.2,0.1,2)
    );

  // Constants
  float ambient = 1.0f;
  float diffuse=0.0f;
  float specular=0.0f;
  for (int i = 0; i<lights.length();i++){
    // diffuse lighting
    vec3 normal = normalize(Normal);
    diffuse = diffuse + lights[i].diff_strength*max(dot(normal, lights[i].dir), 0.0f);
    // specular lighting
    vec3 viewDirection = normalize(camPos - crntPos);
    vec3 reflectionDirection = reflect(-lights[i].dir, normal);
    specular = specular + lights[i].spec_strength*pow(max(dot(viewDirection, reflectionDirection), 0.0f), lights[i].specular_pow);
  }

	return vec4(Color * (diffuse + ambient + specular),1.f);
}

vec4 fog(vec4 color){
    vec3 toCamera = camPos-crntPos;
    float camDist2 = dot(toCamera,toCamera);
    float distBias = 1-clamp(camDist2/120.f,0,1);
    vec4 bg_col1 = vec4(0,0,0,1.0);
    vec4 bg_col2 = vec4(0.529,0.808,0.922,1.0);
    return color*distBias+(1-distBias)*bg_col1;
}

void main()
{
  FragColor = fog(directionalLight());
}
