#version 330 core
out vec4 FragColor;

in vec3 Color;
in vec3 Normal;
in vec3 crntPos;

uniform vec3 camPos;

vec4 directionalLight()
{
	vec3 lightDirection = normalize(vec3(0,1.0,0.2));

	// ambient lighting
	float ambient = 0.8f;

	// diffuse lighting
  float diffuseStrength = 0.9;
	vec3 normal = normalize(Normal);
	float diffuse = diffuseStrength*max(dot(normal, lightDirection), 0.0f);

	// specular lighting
	float specularStrength = 0.2;
	vec3 viewDirection = normalize(camPos - crntPos);
	vec3 reflectionDirection = reflect(-lightDirection, normal);
	float specular = specularStrength*pow(max(dot(viewDirection, reflectionDirection), 0.0f), 2);


	return vec4(Color * (diffuse + ambient + specular),1.f);
}

vec4 fog(vec4 color){
    vec3 toCamera = camPos-crntPos;
    float camDist2 = dot(toCamera,toCamera);
    float distBias = 1-clamp(camDist2/120.f,0,1);
    return color*distBias+(1-distBias)*vec4(0.529,0.808,0.922,1.0);
}

void main()
{
  FragColor = fog(directionalLight());
}
