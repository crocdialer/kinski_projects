//  Terraner.cpp
//
//  created by crocdialer on 21/05/18.

#include "Terraner.hpp"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void Terraner::setup()
{
    ViewerApp::setup();
    register_property(m_noise_scale);
    register_property(m_displace_factor);
    observe_properties();

    // update procedural noise texture
    auto height_tex = m_noise.simplex(0.f);
    textures()[TEXTURE_HEIGHT] = height_tex;

    m_terrain = gl::Mesh::create(gl::Geometry::create_plane(50, 50, 100, 100),
                                 gl::Material::create(gl::ShaderType::UNLIT_DISPLACE));
    m_terrain->set_rotation(-glm::pi<float>() / 2.f, 0.f, 0.f);
    m_terrain->add_tag("terrain");
    m_terrain->material()->set_culling(gl::Material::CULL_NONE);
    m_terrain->material()->add_texture(height_tex, gl::Texture::Usage::DISPLACEMENT);
    scene()->add_object(m_terrain);

    load_settings();
}

/////////////////////////////////////////////////////////////////

void Terraner::update(float timeDelta)
{
    ViewerApp::update(timeDelta);

    // construct ImGui window for this frame
    if(display_gui())
    {
        gui::draw_component_ui(shared_from_this());
    }
}

/////////////////////////////////////////////////////////////////

void Terraner::draw()
{
    gl::clear();
    gl::set_matrices(camera());
    if(*m_draw_grid){ gl::draw_grid(50, 50); }

    scene()->render(camera());
}

/////////////////////////////////////////////////////////////////

void Terraner::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void Terraner::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);
}

/////////////////////////////////////////////////////////////////

void Terraner::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void Terraner::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);
}

/////////////////////////////////////////////////////////////////

void Terraner::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);
}

/////////////////////////////////////////////////////////////////

void Terraner::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void Terraner::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void Terraner::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void Terraner::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void Terraner::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void Terraner::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
}

/////////////////////////////////////////////////////////////////

void Terraner::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void Terraner::teardown()
{
    BaseApp::teardown();
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void Terraner::update_property(const Property::ConstPtr &the_property)
{
    ViewerApp::update_property(the_property);

    if(the_property == m_noise_scale)
    {
        m_noise.set_scale(*m_noise_scale);
    }
    else if(the_property == m_displace_factor)
    {
        m_terrain->material()->uniform("u_displace_factor", *m_displace_factor);
    }
}
