static float3 lightPos = float3(1.2f, 1.0f, 2.0f);
static float3 lightColor = float3(1.0f, 1.0f, 1.0f);
static float3 viewPos = float3(2.0f, 2.0f, 2.0f);

struct VSInput
{
    float3 inPosition;
    float3 inColor;
    float3 inNormal;
    float2 inTexCoord;
}

struct UniformBuffer
{
    float4x4 model;
    float4x4 view;
    float4x4 proj;
}
ConstantBuffer<UniformBuffer> ubo;

struct VSOutput
{
    float4 pos : SV_Position;
    float3 color;
    float3 norm;
    float2 fragTexCoord;
}

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    VSOutput output;
    output.pos = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(input.inPosition, 1.0))));
    output.color = input.inColor;
    // todo: need transpose, inverse
    output.norm = mul(ubo.model , float4(input.inNormal, 1.0f)).xyz;
    output.fragTexCoord = input.inTexCoord;
    return output;
}

Sampler2D texture;

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target
{
    float3 norm = normalize(vertIn.norm);
    float3 lightDir = normalize(lightPos - vertIn.pos.xyz);
    float diff = max(dot(norm, lightDir), 0.0);
    float3 diffuse = diff * lightColor;

    float ambientStrength = 0.1;
	float3 ambient = ambientStrength * lightColor;

    float specularStrength = 0.5;
    float3 viewDir = normalize(viewPos - vertIn.pos.xyz);
    float3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	float3 specular = specularStrength * spec * lightColor;

    float3 objectColor = texture.Sample(vertIn.fragTexCoord).xyz;
	float3 result = (ambient + diffuse + specular) * objectColor;

    return float4(result, 1.0f);
}