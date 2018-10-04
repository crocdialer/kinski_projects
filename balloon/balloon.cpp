//
//  balloon.cpp
//  gl
//
//  Created by Fabian on 03/10/18.
//
//

#include "balloon.hpp"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void balloon::setup()
{
    ViewerApp::setup();
    register_property(m_background_asset_dir);
    register_property(m_max_num_balloons);
    register_property(m_float_speed);
    register_property(m_parallax_factor);
    observe_properties();


    load_settings();
}

/////////////////////////////////////////////////////////////////

void balloon::update(float timeDelta)
{
    ViewerApp::update(timeDelta);

    // construct ImGui window for this frame
    if(display_gui())
    {
        gui::draw_component_ui(shared_from_this());
    }

    // animate bg textures
    float factor = *m_float_speed;
    gl::Texture* bg_textures[] = {&textures()[TEXTURE_BG_01], &textures()[TEXTURE_BG_02]};

    for(auto t : bg_textures)
    {
        t->set_uvw_offset(t->uvw_offset() + gl::Y_AXIS * timeDelta * factor);
        factor /= *m_parallax_factor;
    }
}

/////////////////////////////////////////////////////////////////

void balloon::draw()
{
    gl::clear();

    // render our 2D scene
//    m_2d_scene->render(gui_camera());

    // background textures
    gl::draw_texture(textures()[TEXTURE_BG_02], gl::window_dimension());
    gl::draw_texture(textures()[TEXTURE_BG_01], gl::window_dimension());

    auto sprite_size = glm::vec2(80, 160);
    gl::draw_quad(sprite_size, gl::COLOR_OLIVE, (gl::window_dimension() - sprite_size) / 2.f, false);

}

/////////////////////////////////////////////////////////////////

void balloon::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void balloon::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);
}

/////////////////////////////////////////////////////////////////

void balloon::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void balloon::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);
}

/////////////////////////////////////////////////////////////////

void balloon::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);
}

/////////////////////////////////////////////////////////////////

void balloon::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void balloon::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void balloon::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void balloon::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void balloon::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void balloon::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
}

/////////////////////////////////////////////////////////////////

void balloon::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ *m_background_asset_dir = f; }
}

/////////////////////////////////////////////////////////////////

void balloon::teardown()
{
    BaseApp::teardown();
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void balloon::update_property(const Property::ConstPtr &the_property)
{
    ViewerApp::update_property(the_property);

    if(the_property == m_background_asset_dir)
    {
        m_2d_scene->clear();

        auto bg_image_paths = fs::get_directory_entries(fs::join_paths(*m_background_asset_dir, "background"),
                                                     fs::FileType::IMAGE, true);

        if(!bg_image_paths.empty())
        {
            async_load_texture(bg_image_paths[0], [this](gl::Texture t)
            {
                textures()[TEXTURE_BG_01] = t;
            }, true, true);
        }
        if(bg_image_paths.size() > 1)
        {
            async_load_texture(bg_image_paths[1], [this](gl::Texture t)
            {
                textures()[TEXTURE_BG_02] = t;
            }, true, true);
        }
    }
}
