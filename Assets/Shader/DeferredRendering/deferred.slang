/*
    [[...]] is the attribute syntax of Slang, used to pass target-specific decorative information to the compiler/backend
    vk::binding(binding: int, set: int = 0) indicates binding this shader parameter to the Vulkan descriptor set/binding.
*/
[[vk::binding(0, 0)]] Sampler2D samplerPosition;
[[vk::binding(1, 0)]] Sampler2D samplerNormal;
[[vk::binding(2, 0)]] Sampler2D smaplerAlbedo;

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

[[vk::binding(3, 0)]] ConstantBuffer<UBO> ubo;

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

[shader("fragment")]
float4 fragMain(VSOutput input)
{
    float3 fragPos = samplerPosition.Sample(input.UV).rgb;
    float3 normal = samplerNormal.Sample(input.UV).rgb;
    float4 albedo = smaplerAlbedo.Sample(input.UV);

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
                fragColor.rgb = albedo.aaa;
                break;
        }
        return float4(fragColor, 1.0);
    }

    #define lightCount 6
    #define ambient 0.0

    // Ambient part
    fragColor = albedo.rgb * ambient;

    for (int i = 0; i < lightCount; ++i)
    {
        // Vector to light
        float3 L = ubo.lights[i].position.xyz - fragPos;
        // Distance from light to fragment position
        float dist = length(L);

        // Viewer to fragment
        float3 V = ubo.viewPos.xyz - fragPos;
        V = normalize(V);

        // Light to fragment
        L = normalize(L);

        // Attenuation
        float atten = ubo.lights[i].radius / (pow(dist, 2.0) + 1.0);

        // Diffuse part
        float3 N = normalize(normal);
        float NdotL = max(0.0, dot(N, L));
        float3 diff = ubo.lights[i].color * albedo.rgb * NdotL * atten;

        // Specular part
        // Specular map values are stored in alpha of albedo mrt
        float3 R = reflect(-L, N);
        float NdotR = max(0.0, dot(R, V));
        float3 spec = ubo.lights[i].color * albedo.a * pow(NdotR, 16.0) * atten;

        fragColor += diff + spec;
    }

    return float4(fragColor, 1.0);
}