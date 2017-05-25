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
    setup_vario_map();
    
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
    
    if(m_proto_mesh){ m_proto_mesh->materials()[1]->uniform("u_window_size", gl::window_dimension()); }
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
    set_display(m_proto_mesh, e.getChar());
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
        3, 4, 4, 5, 3, 6, 4, 6, 4, 7, 4, 8, 5, 8, 6, 8
    };
    
    geom->set_primitive_type(GL_LINES);
    gl::MeshPtr ret = gl::Mesh::create(geom, gl::Material::create());
    
    gl::Mesh::Entry e;
    e.base_vertex = 0;
    e.num_vertices = 9;
    e.num_indices = 2;
    
    ret->entries().clear();
    
    for(int i = 0; i < 14; ++i)
    {
        ret->entries().push_back(e);
        e.base_index += 2;
    }
    
    // active material
    auto active_mat = gl::Material::create(gl::ShaderType::LINES_2D);
    active_mat->uniform("u_line_thickness", 8.f);
//    active_mat->uniform("u_window_size", gl::window_dimension());
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
        for(int index : iter->second){ the_vario_mesh->entries()[index].material_index = 1; }
    }
    else{ LOG_WARNING << "digit: " << static_cast<char>(the_value) << " not found"; }
}

/////////////////////////////////////////////////////////////////

void VarioDisplay::setup_vario_map()
{
    m_vario_map[' '] = {};
    m_vario_map['a'] = {0, 1, 5, 6, 7, 8, 12};
    m_vario_map['b'] = {0, 3, 5, 7, 10, 12, 13};
    m_vario_map['c'] = {0, 1, 8, 13};
    m_vario_map['d'] = {0, 3, 5, 10, 12, 13};
    m_vario_map['e'] = {0, 1, 6, 8, 13};
    m_vario_map['f'] = {0, 1, 6, 8};
    m_vario_map['g'] = {0, 1, 7, 8, 12, 13};
    m_vario_map['h'] = {1, 5, 6, 7, 8, 12};
    m_vario_map['i'] = {0, 3, 10, 13};
    m_vario_map['j'] = {0, 5, 11, 12};
    m_vario_map['k'] = {1, 4, 6, 8, 11};
    m_vario_map['l'] = {1, 8, 13};
    m_vario_map['m'] = {1, 2, 4, 5, 8, 12};
    m_vario_map['n'] = {1, 2, 5, 8, 11, 12};
    m_vario_map['o'] = {0, 1, 5, 8, 12, 13};
    m_vario_map['p'] = {0, 1, 5, 6, 7, 8};
    m_vario_map['q'] = {0, 1, 5, 8, 11, 12, 13};
    m_vario_map['r'] = {0, 1, 5, 6, 7, 8, 11};
    m_vario_map['s'] = {0, 1, 6, 7, 12, 13};
    m_vario_map['t'] = {0, 3, 10};
    m_vario_map['u'] = {1, 5, 8, 12, 13};
    m_vario_map['v'] = {1, 4, 8, 9};
    m_vario_map['w'] = {1, 5, 8, 9, 11, 12};
    m_vario_map['x'] = {2, 4, 9, 11};
    m_vario_map['y'] = {2, 4, 9};
    m_vario_map['z'] = {0, 4, 9, 13};
    
    m_vario_map['0'] = {0, 1, 4, 5, 8, 9, 12, 13};
    m_vario_map['1'] = {3, 10};
    m_vario_map['2'] = {0, 5, 7, 9, 13};
    m_vario_map['3'] = {0, 4, 7, 12, 13};
    m_vario_map['4'] = {1, 3, 6, 7, 10};
    m_vario_map['5'] = {0, 1, 6, 11, 13};
    m_vario_map['6'] = {0, 1, 6, 7, 8, 12, 13};
    m_vario_map['7'] = {0, 4, 9};
    m_vario_map['8'] = {0, 1, 5, 6, 7, 8, 12, 13};
    m_vario_map['9'] = {0, 1, 5, 6, 7, 12, 13};
}
