struct VData
{
    float4 position : SV_Position;
    float3 color : VS_Color;
};

float4x4 VP;

VData v_shader(float3 position : KL_Position, float3 color : KL_Color)
{
    VData data;
    data.position = mul(float4(position, 1.0f), VP);
    data.color = color;
    return data;
}

float4 p_shader(VData data) : SV_Target
{
    return float4(data.color, 1.0f);
}
