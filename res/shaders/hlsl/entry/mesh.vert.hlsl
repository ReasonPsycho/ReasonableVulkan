#include "common/frame_ubo.hlsl"
#include "common/vertex_io.hlsl"
#include "vertex/mesh_vertex.hlsl"

VSOutput main(VSInput input)
{
    return VertexTransform(input);
}
