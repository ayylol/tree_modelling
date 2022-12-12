#version 330 core
out vec4 FragColor;

in vec3 Color;
in vec3 Normal;
in vec3 crntPos;

uniform vec3 camPos;

vec4 directionalLight()
{
	// ambient lighting
	float ambient = 0.20f;

	// diffuse lighting
	vec3 normal = normalize(Normal);
	vec3 lightDirection = normalize(vec3(1.0f, 1.0f, 0.0f));
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	// specular lighting
	float specularLight = 0.50f;
	vec3 viewDirection = normalize(camPos - crntPos);
	vec3 reflectionDirection = reflect(-lightDirection, normal);
	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
	float specular = specAmount * specularLight;

	return vec4(Color * (diffuse + ambient + specular),1.f);
}

void main()
{
  FragColor = directionalLight();
}
