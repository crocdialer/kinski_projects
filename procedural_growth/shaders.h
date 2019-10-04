//
// Created by crocdialer on 9/21/19.
//

#pragma once

namespace kinski::gl
{
const char *g_lines_to_cuboids_geom =
        "#version 150\n"
        "\n"
        "// ------------------ Geometry Shader: Line -> Cuboid --------------------------------\n"
        "layout(lines) in;\n"
        "layout (triangle_strip, max_vertices = 20) out;\n"
        "\n"
        "struct matrix_struct_t\n"
        "{\n"
        "    mat4 model_view;\n"
        "    mat4 model_view_projection;\n"
        "    mat4 texture_matrix;\n"
        "    mat3 normal_matrix;\n"
        "};\n"
        "\n"
        "layout(std140) uniform MatrixBlock\n"
        "{\n"
        "    matrix_struct_t ubo;\n"
        "};\n"
        "\n"
        "uniform float u_cap_bias = 2;\n"
        "uniform float u_split_limit = 60.0;\n"
        "\n"
        "// placeholder for halfs of depth and height\n"
        "float depth2 = .5;\n"
        "float height2 = .5;\n"
        "\n"
        "// scratch space for our cuboid vertices\n"
        "vec3 v[8];\n"
        "\n"
        "// projected cuboid vertices\n"
        "vec4 vp[8];\n"
        "\n"
        "vec4 texCoords[4];\n"
        "\n"
        "// cubiod vertices in eye coords\n"
        "vec3 eye_vecs[8];\n"
        "\n"
        "in VertexData\n"
        "{\n"
        "    vec3 position;\n"
        "    vec3 normal;\n"
        "    vec4 color;\n"
        "    vec2 texCoord;\n"
        "    float pointSize;\n"
        "} vertex_in[2];\n"
        "\n"
        "out VertexData\n"
        "{\n"
        "  vec4 color;\n"
        "  vec4 texCoord;\n"
        "  vec3 normal;\n"
        "  vec3 eyeVec;\n"
        "} vertex_out;\n"
        "\n"
        "void create_box(in vec3 p0, in vec3 p1, in vec3 up_vec, in bool draw_caps)\n"
        "{\n"
        "    // same for all verts\n"
        "    depth2 = height2 = vertex_in[0].pointSize / 2.0;\n"
        "    vertex_out.color = vertex_in[0].color;\n"
        "\n"
        "    // basevectors for the cuboid\n"
        "    vec3 dx = normalize(p1 - p0);\n"
        "    vec3 dy = normalize(up_vec);\n"
        "    vec3 dz = cross(dx, dy);\n"
        "\n"
        "    //calc 8 vertices that define our cuboid\n"
        "    // front left bottom\n"
        "    v[0] = p0 + depth2 * dz - height2 * dy;\n"
        "    // front right bottom\n"
        "    v[1] = p1 + depth2 * dz - height2 * dy;\n"
        "    // back right bottom\n"
        "    v[2] = p1 - depth2 * dz - height2 * dy;\n"
        "    // back left bottom\n"
        "    v[3] = p0 - depth2 * dz - height2 * dy;\n"
        "    // front left top\n"
        "    v[4] = p0 + depth2 * dz + height2 * dy;\n"
        "    // front right top\n"
        "    v[5] = p1 + depth2 * dz + height2 * dy;\n"
        "    // back right top\n"
        "    v[6] = p1 - depth2 * dz + height2 * dy;\n"
        "    // back left top\n"
        "    v[7] = p0 - depth2 * dz + height2 * dy;\n"
        "\n"
        "    // calculate projected coords\n"
        "    for(int i = 0; i < 8; i++){ vp[i] = ubo.model_view_projection * vec4(v[i], 1); }\n"
        "\n"
        "    // calculate eye coords\n"
        "    for(int i = 0; i < 8; i++){ eye_vecs[i] = (ubo.model_view * vec4(v[i], 1)).xyz; }\n"
        "\n"
        "    // generate a triangle strip\n"
        "\n"
        "    // transform directions\n"
        "    dx = ubo.normal_matrix * dx;\n"
        "    dy = ubo.normal_matrix * dy;\n"
        "    dz = ubo.normal_matrix * dz;\n"
        "\n"
        "    ///////// mantle faces //////////\n"
        "\n"
        "    // front\n"
        "    vertex_out.normal = dz;\n"
        "    vertex_out.eyeVec = eye_vecs[0];\n"
        "    vertex_out.texCoord = texCoords[0];\n"
        "    gl_Position = vp[0];\n"
        "    EmitVertex();\n"
        "    vertex_out.eyeVec = eye_vecs[1];\n"
        "    vertex_out.texCoord = texCoords[1];\n"
        "    gl_Position = vp[1];\n"
        "    EmitVertex();\n"
        "    vertex_out.eyeVec = eye_vecs[4];\n"
        "    vertex_out.texCoord = texCoords[2];\n"
        "    gl_Position = vp[4];\n"
        "    EmitVertex();\n"
        "    vertex_out.eyeVec = eye_vecs[5];\n"
        "    vertex_out.texCoord = texCoords[3];\n"
        "    gl_Position = vp[5];\n"
        "    EmitVertex();\n"
        "    EndPrimitive();\n"
        "\n"
        "    // top\n"
        "    vertex_out.normal = dy;\n"
        "    vertex_out.eyeVec = eye_vecs[4];\n"
        "    vertex_out.texCoord = texCoords[2];\n"
        "    gl_Position = vp[4];\n"
        "    EmitVertex();\n"
        "    vertex_out.eyeVec = eye_vecs[5];\n"
        "    vertex_out.texCoord = texCoords[3];\n"
        "    gl_Position = vp[5];\n"
        "    EmitVertex();\n"
        "    vertex_out.eyeVec = eye_vecs[7];\n"
        "    vertex_out.texCoord = texCoords[0];\n"
        "    gl_Position = vp[7];\n"
        "    EmitVertex();\n"
        "    vertex_out.eyeVec = eye_vecs[6];\n"
        "    vertex_out.texCoord = texCoords[1];\n"
        "    gl_Position = vp[6];\n"
        "    EmitVertex();\n"
        "    EndPrimitive();\n"
        "\n"
        "    // back\n"
        "    vertex_out.normal = -dz;\n"
        "    vertex_out.eyeVec = eye_vecs[7];\n"
        "    vertex_out.texCoord = texCoords[0];\n"
        "    gl_Position = vp[7];\n"
        "    EmitVertex();\n"
        "    vertex_out.eyeVec = eye_vecs[6];\n"
        "    vertex_out.texCoord = texCoords[1];\n"
        "    gl_Position = vp[6];\n"
        "    EmitVertex();\n"
        "    vertex_out.eyeVec = eye_vecs[3];\n"
        "    vertex_out.texCoord = texCoords[2];\n"
        "    gl_Position = vp[3];\n"
        "    EmitVertex();\n"
        "    vertex_out.eyeVec = eye_vecs[2];\n"
        "    vertex_out.texCoord = texCoords[3];\n"
        "    gl_Position = vp[2];\n"
        "    EmitVertex();\n"
        "    EndPrimitive();\n"
        "\n"
        "    // bottom\n"
        "    vertex_out.normal = -dy;\n"
        "    vertex_out.eyeVec = eye_vecs[3];\n"
        "    vertex_out.texCoord = texCoords[2];\n"
        "    gl_Position = vp[3];\n"
        "    EmitVertex();\n"
        "    vertex_out.eyeVec = eye_vecs[2];\n"
        "    vertex_out.texCoord = texCoords[3];\n"
        "    gl_Position = vp[2];\n"
        "    EmitVertex();\n"
        "    vertex_out.eyeVec = eye_vecs[0];\n"
        "    vertex_out.texCoord = texCoords[0];\n"
        "    gl_Position = vp[0];\n"
        "    EmitVertex();\n"
        "    vertex_out.eyeVec = eye_vecs[1];\n"
        "    vertex_out.texCoord = texCoords[1];\n"
        "    gl_Position = vp[1];\n"
        "    EmitVertex();\n"
        "    EndPrimitive();\n"
        "\n"
        "    if(draw_caps)\n"
        "    {\n"
        "      // caps faces left\n"
        "      vertex_out.normal = -dx;\n"
        "      vertex_out.eyeVec = eye_vecs[3];\n"
        "      vertex_out.texCoord = texCoords[0];\n"
        "      gl_Position = vp[3];\n"
        "      EmitVertex();\n"
        "      vertex_out.eyeVec = eye_vecs[0];\n"
        "      vertex_out.texCoord = texCoords[0];\n"
        "      gl_Position = vp[0];\n"
        "      EmitVertex();\n"
        "      vertex_out.eyeVec = eye_vecs[7];\n"
        "      vertex_out.texCoord = texCoords[0];\n"
        "      gl_Position = vp[7];\n"
        "      EmitVertex();\n"
        "      vertex_out.eyeVec = eye_vecs[4];\n"
        "      vertex_out.texCoord = texCoords[0];\n"
        "      gl_Position = vp[4];\n"
        "      EmitVertex();\n"
        "      EndPrimitive();\n"
        "    }\n"
        "\n"
        "    // caps faces right\n"
        "    vertex_out.normal = dx;\n"
        "    vertex_out.eyeVec = eye_vecs[1];\n"
        "    vertex_out.texCoord = texCoords[0];\n"
        "    gl_Position = vp[1];\n"
        "    EmitVertex();\n"
        "    vertex_out.eyeVec = eye_vecs[2];\n"
        "    vertex_out.texCoord = texCoords[1];\n"
        "    gl_Position = vp[2];\n"
        "    EmitVertex();\n"
        "    vertex_out.eyeVec = eye_vecs[5];\n"
        "    vertex_out.texCoord = texCoords[2];\n"
        "    gl_Position = vp[5];\n"
        "    EmitVertex();\n"
        "    vertex_out.eyeVec = eye_vecs[6];\n"
        "    vertex_out.texCoord = texCoords[3];\n"
        "    gl_Position = vp[6];\n"
        "    EmitVertex();\n"
        "    EndPrimitive();\n"
        "}\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec3 p0 = vertex_in[0].position;\t// start of current segment\n"
        "    vec3 p1 = vertex_in[1].position;\t// end of current segment\n"
        "\n"
        "    // basevectors for the cuboid\n"
        "    vec3 line_seq = p1 - p0;\n"
        "    float line_length = length(line_seq);\n"
        "    vec3 line_dir = line_seq / line_length;\n"
        "\n"
        "    // texCoords\n"
        "    texCoords[0] = vec4(0, 0, 0, 1);\n"
        "    texCoords[1] = vec4(1, 0, 0, 1);\n"
        "    texCoords[2] = vec4(0, 1, 0, 1);\n"
        "    texCoords[3] = vec4(1, 1, 0, 1);\n"
        "\n"
        "    // one large cuboid without caps\n"
        "    if(line_length < u_split_limit)\n"
        "    {\n"
        "      create_box(p0 - line_dir * u_cap_bias,\n"
        "                 p1 + line_dir * u_cap_bias,\n"
        "                 vertex_in[0].normal,\n"
        "                 false);\n"
        "    }\n"
        "    // two smaller cuboid with caps\n"
        "    else\n"
        "    {\n"
        "      create_box(p0 - line_dir * u_cap_bias,\n"
        "                 p0 + line_dir * u_cap_bias,\n"
        "                 vertex_in[0].normal,\n"
        "                 true);\n"
        "\n"
        "      create_box(p1 - line_dir * u_cap_bias,\n"
        "                 p1 + line_dir * u_cap_bias,\n"
        "                 vertex_in[0].normal,\n"
        "                 true);\n"
        "    }\n"
        "}";
}