static float2 positions[3] = float2[](
    float2(0.0, -0.5),
    float2(0.5, 0.5),
    float2(-0.5, 0.5)
);

static float3 colors[3] = float3[](
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0)
);

struct VertexOutput 
{
    float3 color;
    float4 sv_position : SV_Position;  // The built-in annotation SV_Position functions as the output.   
}

[shader("vertex")]
VertexOutput vertMain(uint vid : SV_VertexID)  //  The built-in SV_VertexID annotated variable in the parameters contains the index of the current vertex.
{
    VertexOutput output;
    output.sv_position = float4(positions[vid], 0.0, 1.0);
    output.color = colors[vid];
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target  //  You have to specify your own output variable for each framebuffer where the SV_TARGET annotation specifies the index of the framebuffer.
{
    float3 color = inVert.color;
    return float4(color, 1.0);
}