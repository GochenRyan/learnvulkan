/*
    [[...]] is the attribute syntax of Slang, used to pass target-specific decorative information to the compiler/backend
    vk::binding(binding: int, set: int = 0) indicates binding this shader parameter to the Vulkan descriptor set/binding.
*/
[[vk::binding(0, 0)]] Sampler2D samplerPosition;
[[vk::binding(1, 0)]] Sampler2D samplerNormal;
[[vk::binding(2, 0)]] Sampler2D samplerAlbedo;
[[vk::binding(3, 0)]] Sampler2D samplerMetallic;
[[vk::binding(4, 0)]] Sampler2D samplerRoughness;
[[vk::binding(5, 0)]] Sampler2D samplerEmissive;
[[vk::binding(6, 0)]] Sampler2D samplerAO;
[[vk::binding(7, 0)]] Sampler2D samplerORM;

struct Light {
    float4 position;
    float3 color;
    float radius;
};

struct UBO
{
    Light lights[6];
    float4 viewPos;
    int displayDebugTarget;
};

[[vk::binding(8, 0)]] ConstantBuffer<UBO> ubo;

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
};

[shader("vertex")]
VSOutput vertMain(uint VertexIndex: SV_VertexID)
{
    VSOutput output;
    /*
        fullscreen triangle:
            The coordinates of the three vertices (UV being 0 or 2) are generated using the bit operation of VertexIndex, 
            and then these values are linearly transformed into the NDC space to obtain the three vertices (-1,-1),(3,-1), and (-1,3) - the triangle formed by these three vertices covers the entire screen. 
            This way, without using vertex buffering, full-screen triangles can be generated in the vertex shader by SV_VertexID (or VertexIndex).
        vkCmdDraw(cmd, 3, 1, 0, 0) -> VertexIndex sequence is 0, 1, 2 (commonly used for full-screen triangulation)
    */
    output.UV = float2((VertexIndex << 1) & 2, VertexIndex & 2);
    output.Pos = float4(output.UV * 2.0f - 1.0f, 0.0f, 1.0f);
    return output;
}

static const float PI = 3.14159265359;

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.00000001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float demon = NdotV * (1.0 - k) + k;
    return nom / demon;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 frenelUnreal(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(2.0, (-5.55473 * cosTheta - 6098316) * cosTheta);
}

[shader("fragment")]
float4 fragMain(VSOutput input)
{
    float3 fragPos = samplerPosition.Sample(input.UV).rgb;
    float3 normal = samplerNormal.Sample(input.UV).rgb;
    float3 albedo = pow(samplerAlbedo.Sample(input.UV).rgb, float3(2.2));
    float3 emissive = samplerEmissive.Sample(input.UV).rgb;
#if defined(USE_ORM)
    float ao = samplerORM.Sample(input.UV).r;
    float roughness = samplerORM.Sample(input.UV).g;
    float metallic = samplerORM.Sample(input.UV).b;
#else
    float ao = samplerAO.Sample(input.UV).r;
    float roughness = samplerRoughness.Sample(input.UV).r;
    float metallic = samplerMetallic.Sample(input.UV).r;
#endif
    
    float3 fragColor;

    // Debug display
    if (ubo.displayDebugTarget > 0)
    {
        switch (ubo.displayDebugTarget)
        {
            case 1:
                fragColor.rgb = fragPos;
                break;
            case 2:
                fragColor.rgb = normal;
                break;
            case 3:
                fragColor.rgb = albedo.rgb;
                break;
            case 4:
                fragColor.rgb = float3(ao, ao, ao);
                break;
        }
        return float4(fragColor, 1.0);
    }

    #define lightCount 6

    float3 N = normalize(normal);

    float3 V = ubo.viewPos.xyz - fragPos;
    V = normalize(V);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    float3 F0 = float3(0.04);
    F0 = lerp(F0, albedo.rgb, metallic);

    float3 Lo = float3(0.0);
    for (int i = 0; i < lightCount; ++i)
    {
        // calculate per-light radiance
        float3 L = normalize(ubo.lights[i].position.xyz - fragPos);
        float3 H = normalize(V + L);
        float distance = length(ubo.lights[i].position.xyz - fragPos);
        float attenuation = 1.0 / (distance * distance);
        float3 radiance = ubo.lights[i].position.xyz * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);
        // float3 F = frenelUnreal(clamp(dot(H, V), 0.0, 1.0), F0);

        float3 nominator = NDF * G * F;
        float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        float3 specular = nominator / max(denominator, 0.001);

        // kS is equal to Fresnel
        float3 kS = F;
        float3 kD = float3(1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);

        Lo += (kD * albedo.rgb / PI + specular) * radiance * NdotL;
    }

    float3 ambient = float3(0.03f) * albedo.rgb * ao;
    float3 color = ambient + Lo + emissive;

    // HDR tonemapping
    color = color / (color + float3(1.0));
    // gamma correct
    fragColor = pow(color, float3(1.0 / 2.2));

    return float4(fragColor, 1.0);
}