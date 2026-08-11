// Vertex shader
cbuffer vs_cb : register(b0)
{
    float4x4 VP;
};

struct vertex_out
{
    float4 position : SV_Position;
    float3 color : VT_Color;
};

vertex_out v_shader(const float3 position : KL_Position, const float3 color : KL_Color)
{
    vertex_out data;
    data.position = mul(float4(position, 1), VP);
    data.color = color;
    return data;
}

// Pixel shader
float4 p_shader(const vertex_out data) : SV_Target
{
    return float4(data.color, 1);
}
