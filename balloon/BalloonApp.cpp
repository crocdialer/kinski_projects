//
//  balloon.cpp
//  gl
//
//  Created by Fabian on 03/10/18.
//
//

#include "BalloonApp.hpp"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void BalloonApp::setup()
{
    ViewerApp::setup();

    // load larger glyphs
    fonts()[1].load(fonts()[0].path(), 72);

    register_property(m_asset_dir);
    register_property(m_max_num_balloons);
    register_property(m_sprite_size);
    register_property(m_balloon_noise_intensity);
    register_property(m_balloon_noise_speed);
    register_property(m_float_speed);
    register_property(m_parallax_factor);
    observe_properties();


    load_settings();
}

/////////////////////////////////////////////////////////////////

void BalloonApp::update(float timeDelta)
{
    ViewerApp::update(timeDelta);

    // construct ImGui window for this frame
    if(display_gui())
    {
        gui::draw_component_ui(shared_from_this());
    }

    // animate bg textures
    float factor = *m_float_speed;

    for(auto &t : m_bg_textures)
    {
        t.set_uvw_offset(t.uvw_offset() + gl::Y_AXIS * timeDelta * factor);
        factor /= *m_parallax_factor;
    }
}

/////////////////////////////////////////////////////////////////

void BalloonApp::draw()
{
    gl::clear();

    // background textures
    auto rev_it = m_bg_textures.rbegin();
    for(;rev_it != m_bg_textures.rend(); ++rev_it){ gl::draw_texture(*rev_it, gl::window_dimension()); }

    // balloon sprite
    glm::vec2 pos_offset = glm::vec2(glm::simplex(glm::vec2(get_application_time() * m_balloon_noise_speed->value().x, 0.f)),
                                     glm::simplex(glm::vec2(get_application_time() * m_balloon_noise_speed->value().y, 0.2f)));
    pos_offset *= m_balloon_noise_intensity->value();
    auto pos = pos_offset + (gl::window_dimension() - m_sprite_size->value()) / 2.f;
    gl::draw_texture(m_balloon_texture, *m_sprite_size, pos);

    gl::draw_text_2D(to_string(m_current_num_balloons) + " / " + to_string(m_max_num_balloons->value()),
                     fonts()[1], gl::COLOR_WHITE, pos);

}

/////////////////////////////////////////////////////////////////

void BalloonApp::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void BalloonApp::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);

    switch(e.code())
    {
        case Key::_X:
            explode_balloon();
            break;
        default:
            break;
    }
}

/////////////////////////////////////////////////////////////////

void BalloonApp::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void BalloonApp::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);
}

/////////////////////////////////////////////////////////////////

void BalloonApp::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);
}

/////////////////////////////////////////////////////////////////

void BalloonApp::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void BalloonApp::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void BalloonApp::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void BalloonApp::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void BalloonApp::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void BalloonApp::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
}

/////////////////////////////////////////////////////////////////

void BalloonApp::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ *m_asset_dir = f; }
}

/////////////////////////////////////////////////////////////////

void BalloonApp::teardown()
{
    BaseApp::teardown();
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void BalloonApp::update_property(const Property::ConstPtr &the_property)
{
    ViewerApp::update_property(the_property);

    if(the_property == m_asset_dir)
    {
        m_bg_textures.clear();

        // retrieve background assets
        auto bg_image_paths = fs::get_directory_entries(fs::join_paths(*m_asset_dir, "background"),
                                                        fs::FileType::IMAGE, true);
        auto num_bg_images = bg_image_paths.size();

        for(const auto &p : bg_image_paths)
        {
            async_load_texture(p, [this, num_bg_images](gl::Texture t)
            {
                m_bg_textures.push_back(t);
                if(m_bg_textures.size() == num_bg_images){ create_scene(); }
            }, true, true);
        }

        // retrieve balloon assets
        auto balloon_image_paths = fs::get_directory_entries(fs::join_paths(*m_asset_dir, "balloon"),
                                                             fs::FileType::IMAGE, true);

        if(!balloon_image_paths.empty())
        {
            async_load_texture(balloon_image_paths[0], [this](gl::Texture t)
            {
                m_balloon_texture = t;
            }, true, true);
        }
    }
    else if(the_property == m_max_num_balloons)
    {
        m_current_num_balloons = *m_max_num_balloons;
    }
}

gl::MeshPtr BalloonApp::create_sprite_mesh(const gl::Texture &t)
{
    auto mat = gl::Material::create();
    mat->add_texture(t);
    mat->set_blending();
    gl::MeshPtr ret = gl::Mesh::create(gl::Geometry::create_plane(t.width(), t.height()), mat);
    return ret;
}

void BalloonApp::create_scene()
{
    scene()->clear();
    float z_val = 0.f;

    for(auto &t : m_bg_textures)
    {
        auto m = create_sprite_mesh(t);
        m->set_position(glm::vec3(t.width() / 2.f, t.height() / 2.f, z_val));
        scene()->add_object(m);
        z_val -= .1f;
    }
}

void BalloonApp::explode_balloon()
{
    LOG_DEBUG << "explode_balloon: " << m_current_num_balloons << " -> " << (m_current_num_balloons - 1);
    m_current_num_balloons--;

    if(!m_current_num_balloons)
    {
        LOG_DEBUG << "crash!";
        m_current_num_balloons = *m_max_num_balloons;
    }
}
