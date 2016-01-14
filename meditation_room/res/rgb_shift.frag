#version 330

uniform int u_numTextures;
uniform sampler2D u_sampler_2D[1];

//! shift amount in normalized tex coords [0, 1]
uniform float u_shift_amount = 0.05f;

//! shift angle
uniform float u_shift_angle = 0.f;

#define PI 3.1415926536
const float angle_inc = 2.0 * PI / 3.f;

struct Material
{
  vec4 diffuse;
  vec4 ambient;
  vec4 specular;
  vec4 emission;
  vec4 point_vals;// (size, constant_att, linear_att, quad_att)
  float shinyness;
};

layout(std140) uniform MaterialBlock
{
  Material u_material;
};

in VertexData
{
  vec4 color;
  vec2 texCoord;
} vertex_in;

out vec4 fragData;

void main()
{
  vec4 texColors = vertex_in.color;

  float angle = u_shift_angle;

  for(int i = 0; i < 3; i++, angle += angle_inc)
  {
    vec2 coord = vertex_in.texCoord.st + vec2(sin(angle), cos(angle)) * u_shift_amount;
    texColors[i] *= texture(u_sampler_2D[0], coord)[i];
  }
  fragData = u_material.diffuse * texColors;
}
