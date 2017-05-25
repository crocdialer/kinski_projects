//
//  VarioDisplay.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "VarioDisplay.hpp"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void VarioDisplay::setup()
{
    ViewerApp::setup();
    observe_properties();
    add_tweakbar_for_component(shared_from_this());
    
    m_proto_mesh = create_proto();
    m_proto_mesh->set_scale(10.f);
    load_settings();
}

/////////////////////////////////////////////////////////////////

void VarioDisplay::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void VarioDisplay::draw()
{
    gl::set_matrices(camera());
    if(*m_draw_grid){ gl::draw_grid(50, 50); }
    
    gl::mult_matrix(gl::MODEL_VIEW_MATRIX, m_proto_mesh->global_transform());
    gl::draw_mesh(m_proto_mesh);
}

/////////////////////////////////////////////////////////////////

void VarioDisplay::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void VarioDisplay::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);
}

/////////////////////////////////////////////////////////////////

void VarioDisplay::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void VarioDisplay::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);
}

/////////////////////////////////////////////////////////////////

void VarioDisplay::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);
}

/////////////////////////////////////////////////////////////////

void VarioDisplay::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void VarioDisplay::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void VarioDisplay::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void VarioDisplay::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void VarioDisplay::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void VarioDisplay::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
}

/////////////////////////////////////////////////////////////////

void VarioDisplay::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void VarioDisplay::teardown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void VarioDisplay::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
}

/////////////////////////////////////////////////////////////////

gl::MeshPtr VarioDisplay::create_proto()
{
    auto geom = gl::Geometry::create();
    auto &verts = geom->vertices();
    verts =
    {
        vec3(-.5f, 1.f, 0), vec3(0, 1.f, 0), vec3(.5f, 1.f, 0),
        vec3(-.5f, 0, 0), vec3(0, 0, 0), vec3(.5f, 0, 0),
        vec3(-.5f, -1.f, 0), vec3(0, -1.f, 0), vec3(.5f, -1.f, 0)
    };
    geom->colors().resize(verts.size(), gl::COLOR_WHITE);
    auto &indices = geom->indices();
    indices =
    {
        0, 2, 0, 3, 0, 4, 1, 4, 2, 4, 2, 5,
        3, 5, 3, 6, 4, 6, 4, 7, 4, 8, 5, 8, 6, 8
    };
    
    geom->set_primitive_type(GL_LINES);
    gl::MeshPtr ret = gl::Mesh::create(geom, gl::Material::create());
    
    gl::Mesh::Entry e;
    e.base_vertex = 0;
    e.num_vertices = 9;
    e.num_indices = 2;
    
    ret->entries().clear();
    
    for(int i = 0; i < 13; ++i)
    {
        ret->entries().push_back(e);
        e.base_index += 2;
    }
    
    // active material
    auto active_mat = gl::Material::create();
    active_mat->set_diffuse(gl::COLOR_ORANGE);
    ret->materials().push_back(active_mat);
    
    return ret;
}

/////////////////////////////////////////////////////////////////

void VarioDisplay::set_display(gl::MeshPtr the_vario_mesh, int the_value)
{
    the_value = tolower(the_value);
    
    for(auto &e : the_vario_mesh->entries()){ e.material_index = 0; }
    
    // look up digit in map
    auto iter = m_vario_map.find(the_value);
    
    if(iter != m_vario_map.end())
    {
        
    }
    
}
