#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec4 fragPosEye;
in vec4 fragPosLightSpace;

out vec4 fColor;

//matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;

//lighting
uniform vec3 lightPos1;
uniform vec3 lightDir;
uniform vec3 lightColor;

// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

//components
vec3 ambient;
vec3 ambientP;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 diffuseP;
vec3 specular;
vec3 specularP;
float specularStrength = 0.5f;


float ambientPoint = 0.5f;
float specularStrengthPoint = 0.5f;
float shininessPoint = 32.0f;

float constant = 1.0f;
float linear = 0.0045f;
float quadratic = 0.0075f;

uniform vec3 pointLightPosition;
uniform vec3 pointLightPosition2;
uniform vec3 pointLightPosition3;
uniform vec3 pointLightColor;

uniform float fogDensity;

void computeDirLight()
{

    //compute eye space coordinates
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);

    //normalize light direction
    vec3 lightDirN = vec3(normalize(view * vec4(lightDir, 0.0f)));


    //compute view direction (in eye coordinates, the viewer is situated at the origin
    vec3 viewDir = normalize(- fPosEye.xyz);

    //compute ambient light
    ambient = ambientStrength * lightColor;

    //compute diffuse light
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;

    //compute specular light
    vec3 reflectDir = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
    specular = specularStrength * specCoeff * lightColor;
}

void computePointLight(vec4 lightPosEye, vec3 pointLightColor)
{
	vec3 cameraPosEye = vec3(0.0f);
	vec3 normalEye = normalize(normalMatrix * fNormal);
	vec3 lightDirN = normalize(lightPosEye.xyz - fragPosEye.xyz);
	vec3 viewDirN = normalize(cameraPosEye - fragPosEye.xyz);
	ambientP += ambientPoint * pointLightColor;
	diffuseP += max(dot(normalEye, lightDirN), 0.0f) * pointLightColor;
	vec3 halfVector = normalize(lightDirN + viewDirN);
	vec3 reflection = reflect(-lightDirN, normalEye);
	float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), shininessPoint);
	specularP += specularStrengthPoint * specCoeff * pointLightColor;
	float distance = length(lightPosEye.xyz - fragPosEye.xyz);
	float att = 2.0f / (constant + linear * distance + quadratic * distance * distance);
	
	ambientP *= att;
	diffuseP *= att;
	specularP *= att;
}


float computeShadow()
{	
	// perform perspective divide
    vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    if(normalizedCoords.z > 1.0f)
        return 0.0f;
    
	// Transform to [0,1] range
    normalizedCoords = normalizedCoords * 0.5f + 0.5f;
   
   // Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, normalizedCoords.xy).r;    
   
   // Get depth of current fragment from light's perspective
    float currentDepth = normalizedCoords.z;
   
   // Check whether current frag pos is in shadow
    float bias = 0.005f;
    float shadow = currentDepth - bias> closestDepth  ? 1.0f : 0.0f;

    return shadow;	
}

float computeFog()
{

 float fragmentDistance = length(fragPosEye);
 float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));

 return clamp(fogFactor, 0.0f, 1.0f);
}

void main() 
{
    computeDirLight();
	
	ambientP = vec3(0.0f);
	diffuseP = vec3(0.0f);
	specularP = vec3(0.0f);

	vec4 lightPosEye = view * vec4(pointLightPosition, 1.0f);
	//vec4 lightPosEye2 = view * vec4(pointLightPosition2, 1.0f);
	//vec4 lightPosEye3 = view * vec4(pointLightPosition3, 1.0f);
	computePointLight(lightPosEye, pointLightColor);
	//computePointLight(lightPosEye2, pointLightColor);
	//computePointLight(lightPosEye3, pointLightColor);

	//modulate with shadow
	float shadow = computeShadow();
	diffuse = (1.0 - shadow) * diffuse;
	specular = (1.0 - shadow) * specular;

	ambient += ambientP;
	diffuse += diffuseP;
	specular += specularP;

    //compute final vertex color
    vec3 color = min((ambient + diffuse) * texture(diffuseTexture, fTexCoords).rgb + specular * texture(specularTexture, fTexCoords).rgb, 1.0f);

	float fogFactor = computeFog();
	vec3 fogColor = vec3(0.5f, 0.5f, 0.5f);	color = fogColor * (1 - fogFactor) + color * fogFactor;


    fColor = vec4(color, 1.0f);
}
