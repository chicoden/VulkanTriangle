static float2 positions[] = {
    {-0.5,  0.5},
    { 0.5,  0.5},
    { 0.0, -0.5}
};

static float3 colors[] = {
    {1.0, 0.0, 0.0},
    {0.0, 1.0, 0.0},
    {0.0, 0.0, 1.0}
};

struct VertexAttribs {
    float3 color;
};

struct OutVertex {
    float4 position : SV_Position;
    VertexAttribs attribs : VertexAttribs;
};

[shader("vertex")]
OutVertex vertex_main(uint vertex_index : SV_VulkanVertexID) {
    OutVertex out;
    out.position = float4(positions[vertex_index], 0.0, 1.0);
    out.attribs.color = colors[vertex_index];
    return out;
}

[shader("fragment")]
float4 fragment_main(VertexAttribs vertex_attribs : VertexAttribs) : SV_Target {
    return float4(vertex_attribs.color, 1.0);
}