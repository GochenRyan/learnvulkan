/*
    [[...]] is the attribute syntax of Slang, used to pass target-specific decorative information to the compiler/backend
    vk::binding(binding: int, set: int = 0) indicates binding this shader parameter to the Vulkan descriptor set/binding.
*/
[[vk::binding(0, 1)]] Sampler2D samplerColor;
[[vk::binding(1, 1)]] Sampler2D samplerNormalMap;
[[vk::binding(2, 1)]] Sampler2D samplerMetallicMap;
[[vk::binding(3, 1)]] Sampler2D samplerRoughnessMap;

struct VSInput
{
    float4 Pos;
    float3 Normal;
    float3 Tangent;
    float3 Color;
    float2 UV0;
    float2 UV1;
    float2 UV2;
    float2 UV3;
    float2 UV4;
    float2 UV5;
    float2 UV6;
    float2 UV7;
}

struct VSOutput
{
    float4 Pos : SV_Position;
    float3 Normal;
    float2 UV;
    float3 Color;
    float3 WorldPos;
    // The Tangent (tangent line) is a direction vector at a vertex, used together with the normal to define the "tangent space" (TBN)
    float3 Tangent;
};

struct FSOutput
{
    float4 Position;
    float4 Normal;
    float4 Albedo;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4x4 view;
    // instancePos are typically arrays that store the translation (or other data for each instance) of each instance - that is, the shift vectors of each instance during instanced rendering (or other information stored in w).
    float4 instancePos[3];
}

[[vk::binding(0, 0)]] ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput vertMain(VSInput input, uint InstanceIndex: SV_InstanceID)
{
    VSOutput output;
    float4 tmpPos = input.Pos + ubo.instancePos[InstanceIndex];

    output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, tmpPos)));
    
    output.UV = input.UV0;

    // Vertex position in world space
    output.WorldPos = mul(ubo.model, tmpPos).xyz;

    // Normal in world space
    output.Normal = normalize(input.Normal);
    output.Tangent = normalize(input.Tangent);

    // Currently just vertex color
    output.Color = input.Color;
    return output;
}

[shader("fragment")]
FSOutput fragMain(VSOutput input)
{
    FSOutput output;
    output.Position = float4(input.WorldPos, 1.0);

    // Calculate normal in tangent space
    float3 N = normalize(input.Normal);
    float3 T = normalize(input.Tangent);
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);

    float3 tNorm = mul(normalize(samplerNormalMap.Sample(input.UV).xyz * 2.0 - float3(1.0, 1.0, 1.0)), TBN);
    output.Normal = float4(tNorm, 1.0);

    output.Albedo = samplerColor.Sample(input.UV);
    return output;
}