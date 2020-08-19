#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;
  
struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 direction;
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;

    float cutOff;
    float outerCutOff;
};

struct PointLight {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};
#define NR_POINT_LIGHTS 3   

uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform vec3 viewPos;
uniform Material material;

vec3 CalcAmbient(vec3 ambient);
vec3 CalcDiffuse(vec3 diffuse, vec3 lightDir, vec3 normal);
vec3 CalcSpecular(vec3 specular, vec3 lightDir, vec3 normal, vec3 viewDir);
float CalcAttenuation(vec3 position, vec3 fragPos, float constant, float linear, float quadratic);
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir); 
vec3 CalSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);


void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    //direct lightning
    vec3 result = CalcDirLight(dirLight, norm, viewDir);
    //pointlights
    for (int i = 0; i < NR_POINT_LIGHTS; i++)
    {
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
    }
    //spotlight
    result += CalSpotLight(spotLight, norm, FragPos, viewDir);

    //combined
    FragColor = vec4(result, 1.0);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);

    vec3 ambient = CalcAmbient(light.ambient);
    vec3 diffuse = CalcDiffuse(light.diffuse, lightDir, normal);
    vec3 specular = CalcSpecular(light.specular, lightDir, normal, viewDir);
    
    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);

    vec3 ambient = CalcAmbient(light.ambient);
    vec3 diffuse = CalcDiffuse(light.diffuse, lightDir, normal);
    vec3 specular = CalcSpecular(light.specular, lightDir, normal, viewDir);
    float attenuation = 
        CalcAttenuation(light.position, fragPos, light.constant, light.linear, light.quadratic);

    return ((ambient + diffuse + specular) * attenuation);
}

vec3 CalSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);

    vec3 ambient = CalcAmbient(light.ambient);
    vec3 diffuse = CalcDiffuse(light.diffuse, lightDir, normal);
    vec3 specular = CalcSpecular(light.specular, lightDir, normal, viewDir);
    float attenuation = 
        CalcAttenuation(light.position, fragPos, light.constant, light.linear, light.quadratic);

    //light cone
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    diffuse *= intensity;
    specular *= intensity;

    return ((ambient + diffuse + specular) * attenuation);
}

vec3 CalcAmbient(vec3 ambient)
{
    return (ambient * vec3(texture(material.diffuse, TexCoords)));
}

vec3 CalcDiffuse(vec3 diffuse, vec3 lightDir, vec3 normal)
{
    float diff = max(dot(normal, lightDir), 0.0);
    return (diff * diffuse * vec3(texture(material.diffuse, TexCoords)));
}

vec3 CalcSpecular(vec3 specular, vec3 lightDir, vec3 normal, vec3 viewDir)
{
    vec3 halfwayDir = normalize(lightDir + viewDir);
    vec3 reflectDir = reflect(-lightDir, normal);

    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    //float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    return (spec * specular * vec3(texture(material.specular, TexCoords)));
}

float CalcAttenuation(vec3 position, vec3 fragPos, float constant, float linear, float quadratic)
{
    float distance = length(position - fragPos);
    return (1.0 / (constant + linear * distance + quadratic * (distance * distance)));
}