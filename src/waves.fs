#version 330 core

out vec4 FragColor;

struct PointLight {
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
	
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct DirLight {
    vec3 direction;
	
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
  
    float constant;
    float linear;
    float quadratic;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;       
};

#define NUM_OF_POINT_LIGHTS 1

in vec3 FragPos;
in vec3 Normal;
//in vec2 TexCoords;

uniform vec3 color;
uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[NUM_OF_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform float shininess;

uniform bool useLighting;

uniform samplerCube skybox;
uniform float skyboxBlendAmount;

uniform bool showFoam;
uniform float foamThreshold;
uniform float foamIntensity;

vec3 currentColor;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
float GetFoam(vec3 normal);

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    //// phase 1: directional lighting
    //vec3 result = CalcDirLight(dirLight, norm, viewDir);
    //// phase 2: point lights
    //for(int i = 0; i < NUM_OF_POINT_LIGHTS; i++)
    //    result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);    
    //// phase 3: spot light
    //result += CalcSpotLight(spotLight, norm, FragPos, viewDir); 

    vec3 incidence = normalize(FragPos - viewPos);
    vec3 reflection = reflect(incidence , normalize(Normal));
    vec4 skyColor = texture(skybox, reflection); 

    vec4 blendedColor = (1.0 - skyboxBlendAmount) * vec4(color, 1.0) + skyColor * skyboxBlendAmount;

    currentColor = blendedColor.rgb;
    if (showFoam){
        float foam = GetFoam(normalize(Normal));
        currentColor = (1.0 - foam) * currentColor + vec3(1.0) * foam;
    }

    if (useLighting){
        vec3 result =  vec3(0.0);
        for(int i = 0; i < NUM_OF_POINT_LIGHTS; i++)
            //result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
        //result += CalcSpotLight(spotLight, norm, FragPos, viewDir); 
        result += CalcDirLight(dirLight, norm, viewDir);

        FragColor = vec4(result, 1.0);
    }
    else{
        FragColor = vec4(currentColor, 1.0);
    }
    //norm = (norm + vec3(1.0)) / 2.0;
    //FragColor = vec4(norm.x, norm.z, norm.y, 1.0);
}

// calculates the color when using a directional light.
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

    // fresnel
    float normalreflection = 0.04; 
    float cos = max(0.0, dot(viewDir, normal));
    float fresnel = normalreflection  + (1.0 - normalreflection) * pow(1.0 - cos, 5.0);

    // combine results
    vec3 ambient = light.ambient * currentColor;
    vec3 diffuse = light.diffuse * diff * currentColor;
    vec3 specular = light.specular * spec * currentColor * fresnel;
    return (ambient + diffuse + specular);
}

// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // combine results
    vec3 ambient = light.ambient * currentColor;
    vec3 diffuse = light.diffuse * diff * currentColor;
    vec3 specular = light.specular * spec * currentColor;
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

// calculates the color when using a spot light.
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    // combine results
    vec3 ambient = light.ambient * currentColor;
    vec3 diffuse = light.diffuse * diff * currentColor;
    vec3 specular = light.specular * spec * currentColor;
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
}

float GetFoam(vec3 normal)
{
    float slope = 1.0 - dot(normal, vec3(0.0, 1.0, 0.0));
    float foam = smoothstep(foamThreshold, 1.0, slope) * foamIntensity;
    return foam;
}