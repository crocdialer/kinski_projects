#version 330

#define DIFFUSE 0
#define DISPLACE 1

uniform int u_numTextures;
uniform sampler2D u_sampler_2D[2];

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
  if(u_numTextures > 1) 
    texColors *= texture(u_sampler_2D[DIFFUSE], vertex_in.texCoord.st); 
  fragData = u_material.diffuse * texColors; 
}
