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
    
    float scale = 10.f;
    m_proto_lines = create_proto();
    m_proto_lines->set_scale(scale);
    
//    m_proto_triangles = create_proto_triangles(.07f);
//    m_proto_triangles->set_scale(10.f);
    setup_vario_map();
    
    auto aabb = m_proto_lines->aabb();
    
    for(int i = -2; i < 3; ++i)
    {
        auto m_line = m_proto_lines->copy();
        m_line->set_position(gl::vec3(i * 1.5f * aabb.width(), 0, 0));
        scene()->add_object(m_line);
        m_digits_lines.push_back(m_line);
        
//        auto m_triangle = m_proto_triangles->copy();
//        m_triangle->set_position(gl::vec3(i * 1.5f * aabb.width(), 0, 0));
//        scene()->add_object(m_triangle);
//        m_digits_triangles.push_back(m_triangle);
    }
    m_cursor_mesh = create_cursor();
    m_cursor_mesh->set_scale(scale);
    m_cursor_mesh->set_position(gl::vec3(0, aabb.min.y * 1.1f, 0));
    scene()->add_object(m_cursor_mesh);
    
    load_settings();
}

/////////////////////////////////////////////////////////////////

void VarioDisplay::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    m_cursor_mesh->position().x = m_digits_lines[std::min(m_current_index, 4)]->position().x;
}

/////////////////////////////////////////////////////////////////

void VarioDisplay::draw()
{
    gl::clear();
    gl::set_matrices(camera());
    if(*m_draw_grid){ gl::draw_grid(50, 50); }
    scene()->render(camera());
}

/////////////////////////////////////////////////////////////////

void VarioDisplay::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
    
    if(m_proto_lines){ m_proto_lines->materials()[1]->uniform("u_window_size", gl::window_dimension()); }
}

/////////////////////////////////////////////////////////////////

void VarioDisplay::key_press(const KeyEvent &e)
{
    if(e.isControlDown()){ ViewerApp::key_press(e); }
}

/////////////////////////////////////////////////////////////////

void VarioDisplay::key_release(const KeyEvent &e)
{
    if(e.isControlDown()){ ViewerApp::key_release(e); return;}
    
    switch(e.getCode())
    {
        case Key::_BACKSPACE:
            m_current_index = std::max(m_current_index - 1, 0);
            set_display(m_digits_lines[m_current_index], ' ');
            break;
        
        case Key::_DELETE:
            set_display(m_digits_lines[m_current_index], ' ');
            break;
            
        case Key::_LEFT:
            m_current_index = std::max(m_current_index - 1, 0);
            break;
            
        case Key::_RIGHT:
            m_current_index = std::min(m_current_index + 1, 4);
            break;
            
        default:
            if((m_current_index < 5) && set_display(m_digits_lines[m_current_index], e.getChar()))
            { m_current_index = std::min(m_current_index + 1, 5); }
            break;
    }
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
    geom->compute_aabb();
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
    active_mat->uniform("u_line_thickness", 11.f);
    active_mat->set_line_width(11.f);
    active_mat->uniform("u_window_size", gl::window_dimension());
    active_mat->set_diffuse(gl::COLOR_ORANGE);
    ret->materials().push_back(active_mat);
    
    return ret;
}

/////////////////////////////////////////////////////////////////

gl::MeshPtr VarioDisplay::create_proto_triangles(float line_width)
{
    auto proto = create_proto();
    auto ret = gl::Mesh::create();
    ret->geometry()->set_primitive_type(GL_TRIANGLE_STRIP);
    ret->material()->set_culling(gl::Material::CULL_NONE);
    ret->entries().clear();
    
    const auto& src_verts = proto->geometry()->vertices();
    const auto& src_indices = proto->geometry()->indices();
    
    auto& dst_verts = ret->geometry()->vertices();
    auto& dst_indices = ret->geometry()->indices();
    
    uint32_t i = 0;
    
    for(auto &e : proto->entries())
    {
        auto v0 = src_verts[src_indices[e.base_index]], v1 = src_verts[src_indices[e.base_index + 1]];
        gl::vec3 orth = line_width * gl::normalize(glm::cross(v1 - v0, gl::Z_AXIS));
        gl::vec3 tmp_verts[4] = {v0 + orth, v0 - orth, v1 + orth, v1 - orth};
        uint32_t tmp_indices[4] = {i, i + 1, i + 2, i + 3};
        dst_verts.insert(dst_verts.end(), tmp_verts, tmp_verts + 4);
        dst_indices.insert(dst_indices.end(), tmp_indices, tmp_indices + 4);
        
        gl::Mesh::Entry tmp_entry;
        tmp_entry.num_indices = 4;
        tmp_entry.base_index = i;
        tmp_entry.enabled = false;
        ret->entries().push_back(tmp_entry);
        i += 4;
    }
    ret->geometry()->colors().resize(dst_verts.size(), gl::COLOR_WHITE);
    ret->geometry()->compute_aabb();
    
    // active material
    auto active_mat = gl::Material::create();
    active_mat->set_diffuse(gl::COLOR_ORANGE);
    active_mat->set_culling(gl::Material::CULL_NONE);
    ret->materials().push_back(active_mat);
    
    return ret;
}

/////////////////////////////////////////////////////////////////

gl::MeshPtr VarioDisplay::create_cursor()
{
    float w = 0.5f, h = 0.2f;
    auto geom = gl::Geometry::create();
    geom->set_primitive_type(GL_LINE_STRIP);
    geom->vertices() = {gl::vec3(-w/2, -h, 0), gl::vec3(0), gl::vec3(w/2, -h, 0)};
    geom->indices() = {0, 1, 2};
    geom->colors().resize(3, gl::COLOR_WHITE);
    auto ret = gl::Mesh::create(geom, m_proto_lines->materials()[1]);
    return ret;
}

/////////////////////////////////////////////////////////////////

bool VarioDisplay::set_display(gl::MeshPtr the_vario_mesh, int the_value)
{
    the_value = tolower(the_value);
    
    // look up digit in map
    auto iter = m_vario_map.find(the_value);
    
    if(iter != m_vario_map.end())
    {
        for(auto &e : the_vario_mesh->entries()){ e.material_index = 0; }
        for(int index : iter->second){ the_vario_mesh->entries()[index].material_index = 1; }
        return true;
    }
    LOG_WARNING << "digit: " << static_cast<char>(the_value) << " not found";
    return false;
}

/////////////////////////////////////////////////////////////////

bool VarioDisplay::set_display_triangles(gl::MeshPtr the_vario_mesh, int the_value)
{
    the_value = tolower(the_value);
    
    // look up digit in map
    auto iter = m_vario_map.find(the_value);
    
    if(iter != m_vario_map.end())
    {
        for(auto &e : the_vario_mesh->entries()){ e.enabled = false; }
        
        for(int index : iter->second)
        {
            the_vario_mesh->entries()[index].material_index = 1;
            the_vario_mesh->entries()[index].enabled = true;
        }
        return true;
    }
    LOG_WARNING << "digit: " << static_cast<char>(the_value) << " not found";
    return false;
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
    
    m_vario_map['-'] = {6, 7};
    m_vario_map['+'] = {3, 6, 7, 10};
    m_vario_map['%'] = {1, 2, 4, 6, 7, 9, 11, 12};
    
    float sum = 0;
    for(const auto &p : m_vario_map){ sum += p.second.size(); }
    sum /= m_vario_map.size();
    LOG_INFO << "average num_elements: " << to_string(sum, 2);
}
