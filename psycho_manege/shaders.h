//
// Created by crocdialer on 9/21/19.
//

#pragma once

namespace kinski::gl
{

const char *unlit_displace_vert =
        "#version 330\n"
        "\n"
        "#define DIFFUSE 0\n"
        "#define DISPLACE 1\n"
        "\n"
        "uniform mat4 u_modelViewProjectionMatrix;\n"
        "uniform mat4 u_textureMatrix;\n"
        "\n"
        "uniform sampler2D u_sampler_2D[2];\n"
        "uniform float u_displace_factor = 0.f;\n"
        "\n"
        "layout(location = 0) in vec4 a_vertex; \n"
        "layout(location = 1) in vec3 a_normal; \n"
        "layout(location = 2) in vec4 a_texCoord;\n"
        "layout(location = 3) in vec4 a_color; \n"
        "\n"
        "out VertexData\n"
        "{ \n"
        "  vec4 color;\n"
        "  vec2 texCoord;\n"
        "} vertex_out;\n"
        "\n"
        "void main() \n"
        "{\n"
        "  vertex_out.color = a_color;\n"
        "  vertex_out.texCoord = (u_textureMatrix * a_texCoord).xy;\n"
        "  float displace = (2.0 * texture(u_sampler_2D[DISPLACE], vertex_out.texCoord.st).x) - 1.0;\n"
        "  vec4 displace_vert = a_vertex + vec4(a_normal * u_displace_factor * displace, 0.f); \n"
        "  gl_Position = u_modelViewProjectionMatrix * displace_vert; \n"
        "}";

const char *unlit_skin_vert =
        "#version 330\n"
        "\n"
        "uniform mat4 u_modelViewProjectionMatrix;\n"
        "uniform mat4 u_textureMatrix;\n"
        "uniform mat4 u_bones[110];\n"
        "\n"
        "layout(location = 0) in vec4 a_vertex; \n"
        "layout(location = 2) in vec4 a_texCoord;\n"
        "layout(location = 3) in vec4 a_color; \n"
        "layout(location = 6) in ivec4 a_boneIds; \n"
        "layout(location = 7) in vec4 a_boneWeights;\n"
        "\n"
        "out VertexData\n"
        "{ \n"
        "  vec4 color;\n"
        "  vec2 texCoord;\n"
        "} vertex_out;\n"
        "\n"
        "void main() \n"
        "{\n"
        "  vertex_out.color = a_color;\n"
        "  vertex_out.texCoord = (u_textureMatrix * a_texCoord).xy;\n"
        "  vec4 newVertex = vec4(0); \n"
        "  \n"
        "  for (int i = 0; i < 4; i++)\n"
        "  {\n"
        "    newVertex += u_bones[a_boneIds[i]] * a_vertex * a_boneWeights[i]; \n"
        "  }\n"
        "  gl_Position = u_modelViewProjectionMatrix * vec4(newVertex.xyz, 1.0); \n"
        "}";

const char *unlit_skin_displace_vert =
        "#version 330\n"
        "\n"
        "#define DIFFUSE 0\n"
        "#define DISPLACE 1\n"
        "\n"
        "uniform mat4 u_modelViewProjectionMatrix;\n"
        "uniform mat4 u_textureMatrix;\n"
        "uniform mat4 u_bones[110];\n"
        "\n"
        "// vertex displacement\n"
        "uniform sampler2D u_sampler_2D[2];\n"
        "uniform float u_displace_factor = 0.f;\n"
        "\n"
        "layout(location = 0) in vec4 a_vertex; \n"
        "layout(location = 1) in vec3 a_normal; \n"
        "layout(location = 2) in vec4 a_texCoord;\n"
        "layout(location = 3) in vec4 a_color; \n"
        "layout(location = 6) in ivec4 a_boneIds; \n"
        "layout(location = 7) in vec4 a_boneWeights;\n"
        "\n"
        "out VertexData\n"
        "{ \n"
        "  vec4 color;\n"
        "  vec2 texCoord;\n"
        "} vertex_out;\n"
        "\n"
        "void main() \n"
        "{\n"
        "  vertex_out.color = a_color;\n"
        "  vertex_out.texCoord = (u_textureMatrix * a_texCoord).xy;\n"
        "  vec4 newVertex = vec4(0); \n"
        "  vec3 newNormal = vec3(0); \n"
        "  \n"
        "  for (int i = 0; i < 4; i++)\n"
        "  {\n"
        "    newVertex += u_bones[a_boneIds[i]] * a_vertex * a_boneWeights[i]; \n"
        "    newNormal += (u_bones[a_boneIds[i]] * vec4(a_normal, 0.0) * a_boneWeights[i]).xyz; \n"
        "  }\n"
        "  newVertex = vec4(newVertex.xyz, 1.0);\n"
        "\n"
        "  float displace = (2.0 * texture(u_sampler_2D[DISPLACE], vertex_out.texCoord.st).x) - 1.0;\n"
        "  vec4 displace_vert = newVertex + vec4(newNormal * u_displace_factor * displace, 0.f); \n"
        "  gl_Position = u_modelViewProjectionMatrix * displace_vert; \n"
        "}";
}
