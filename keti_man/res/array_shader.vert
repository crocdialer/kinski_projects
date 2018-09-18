#version 410 core

struct matrix_struct_t
{
    mat4 model_view;
    mat4 model_view_projection;
    mat4 texture_matrix;
    mat3 normal_matrix;
};

layout(std140) uniform MatrixBlock
{
    matrix_struct_t ubo;
};

in vec4 a_vertex;
in vec4 a_texCoord;
in vec4 a_color;

out VertexData{
   vec4 color;
   vec2 texCoord;
} vertex_out;

void main()
{
   vertex_out.color = a_color;
   vertex_out.texCoord =  (ubo.texture_matrix * a_texCoord).xy;
   gl_Position = ubo.model_view_projection * a_vertex;
}
