#version 410 core

in vec3 v_vertexColors;
in vec3 v_vertexNormals;
in vec2 v_texCoords; // Receive texture coordinates
in vec3 FragPos;

out vec4 color;

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

uniform PointLight pointLight;
uniform vec3 u_CameraPosition;
uniform sampler2D u_diffuseMap; // Texture sampler for the diffuse map
uniform sampler2D u_bumpMap;    // Texture sampler for the bump map
uniform sampler2D u_specularMap;// Texture sampler for the specular map

void main()
{
    vec3 norm = normalize(v_vertexNormals);

    // If a normal map is present, it would affect the 'norm' here.
    // This would require additional logic to apply the normal map.

    // vec3 lightDir = normalize(pointLight.position - FragPos);
    // float diff = max(dot(norm, lightDir), 0.0);

    // Texture sampling
    vec3 sampledDiffuse = texture(u_diffuseMap, v_texCoords).rgb;
    vec3 sampledSpecular = texture(u_specularMap, v_texCoords).rgb;

    // Calculate ambient, diffuse, and specular components
    // vec3 ambient = pointLight.ambient * sampledDiffuse;
    // vec3 diffuse = diff * pointLight.diffuse * sampledDiffuse;

    vec3 viewDir = normalize(u_CameraPosition - FragPos);
    // vec3 reflectDir = reflect(-lightDir, norm);
    // float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    // vec3 specular = spec * pointLight.specular * sampledSpecular;

    // float distance = length(pointLight.position - FragPos);
    // float attenuation = 1.0 / (pointLight.constant + pointLight.linear * distance + pointLight.quadratic * distance * distance);

    // vec3 result = (ambient + attenuation * (diffuse + specular));
    vec3 result = sampledDiffuse + sampledSpecular;

    color = vec4(result, 1.0f);
    // color = vec4(v_texCoords, 0.0, 1.0); // This will show UVs as red-green color
    // color = vec4(1.0, 0.0, 0.0, 1.0); // Red color

    // color = vec4(sampledDiffuse, 1.0);
}
