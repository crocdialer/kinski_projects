//
//  balloon.cpp
//  gl
//
//  Created by Fabian on 03/10/18.
//
//

#include "gl_post_process/Blur.hpp"
#include "BalloonApp.hpp"

using namespace std;
using namespace kinski;
using namespace glm;


const std::string g_parallax_bg_tag = "background_parallax";

/////////////////////////////////////////////////////////////////

void BalloonApp::setup()
{
    ViewerApp::setup();

    // load larger glyphs
    fonts()[1].load(fonts()[0].path(), 72);

    register_property(m_asset_dir);
    register_property(m_offscreen_res);
    register_property(m_max_num_balloons);
    register_property(m_sprite_size);
    register_property(m_balloon_noise_intensity);
    register_property(m_balloon_noise_speed);
    register_property(m_parallax_factor);
    register_property(m_motion_blur);
    register_property(m_float_speed);
    register_property(m_use_syphon);
    observe_properties();

    load_settings();
}

/////////////////////////////////////////////////////////////////

void BalloonApp::update(float timeDelta)
{
    ViewerApp::update(timeDelta);

    // check for offscreen fbo
    if(!m_offscreen_fbo || m_blur_fbos.empty() || !m_blur_fbos.front()){ m_offscreen_res->notify_observers(); }

    // construct ImGui window for this frame
    if(display_gui())
    {
        gui::draw_component_ui(shared_from_this());
        gui::draw_scenegraph_ui(scene(), &selected_objects());
        auto obj = selected_objects().empty() ? nullptr : *selected_objects().begin();
        gui::draw_object3D_ui(obj, m_2d_cam);

        if(*m_use_warping){ gui::draw_component_ui(m_warp_component); }
    }

    // animate bg textures
    float factor = *m_float_speed;

    gl::SelectVisitor<gl::Mesh> visitor({g_parallax_bg_tag}, false);
    scene()->root()->accept(visitor);
    uint32_t i = 0;

    auto blur_factor = [](float f) -> float
    {
        return 50.f * f * f * f;
    };

    for(auto m : visitor.get_objects())
    {
        auto val = blur_factor(factor) * *m_motion_blur;

        auto *tex = m->material()->get_texture_ptr();

        if(m_parallax_textures[i])
        {
            gl::render_to_texture(m_blur_fbos[i], [this, i, val]()
            {
                gl::Blur blur(glm::vec2(0.f, val));
                blur.render_output(m_parallax_textures[i]);
            });
            tex = m_blur_fbos[i]->texture_ptr();
            m->material()->add_texture(*tex);
            i++;
        }

        if(tex)
        {
            tex->set_uvw_offset(tex->uvw_offset() + gl::Y_AXIS * timeDelta * factor);
            factor /= *m_parallax_factor;
        }
    }

    // balloon sprite scaling / positioning
    if(m_sprite_mesh)
    {
        m_sprite_mesh->set_scale(glm::vec3(m_sprite_size->value() / gl::window_dimension(), 1.f));
        glm::vec2 pos_offset = glm::vec2(glm::simplex(glm::vec2(get_application_time() * m_balloon_noise_speed->value().x, 0.f)),
                                         glm::simplex(glm::vec2(get_application_time() * m_balloon_noise_speed->value().y, 0.2f)));
        pos_offset *= m_balloon_noise_intensity->value() / glm::vec2(m_offscreen_fbo->size());
        m_sprite_mesh->set_position(glm::vec3(pos_offset, 0.f));
    }
}


/////////////////////////////////////////////////////////////////

void BalloonApp::draw()
{
    auto offscreen_tex = gl::render_to_texture(m_offscreen_fbo, [this]()
    {
        gl::clear();
        gl::set_matrices(m_2d_cam);

        // render our game scene
        scene()->render(m_2d_cam);

        // boxes for selected objects
        for(auto &obj : selected_objects()){ gl::draw_boundingbox(obj->aabb()); }
    });
    offscreen_tex.set_mag_filter(GL_LINEAR);

    gl::clear();

    if(*m_use_warping)
    {
        for(uint32_t i = 0; i < m_warp_component->num_warps(); i++)
        {
            if(m_warp_component->enabled(i))
            {
                m_warp_component->render_output(i, offscreen_tex);
            }
        }
    }
    else{ gl::draw_texture(offscreen_tex, gl::window_dimension()); }

    // syphon output
    if(*m_use_syphon){ m_syphon_out.publish_texture(offscreen_tex); }
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
        scene()->clear();
        m_parallax_meshes.clear();
        m_parallax_textures.clear();
        m_blur_fbos.clear();

        // retrieve static background
        auto static_bg_paths = fs::get_directory_entries(fs::join_paths(*m_asset_dir, "static_backgrounds"),
                                                         fs::FileType::IMAGE, true);

        if(!static_bg_paths.empty())
        {
            async_load_texture(static_bg_paths[0], [this](gl::Texture t)
            {
                m_bg_mesh->material()->add_texture(t);
            }, true, true);
        }

        // retrieve parallax background assets
        auto bg_image_paths = fs::get_directory_entries(fs::join_paths(*m_asset_dir, "parallax_backgrounds"),
                                                        fs::FileType::IMAGE, true);
        auto num_bg_images = bg_image_paths.size();
        m_parallax_meshes.resize(num_bg_images);
        m_parallax_textures.resize(num_bg_images);
        m_blur_fbos.resize(num_bg_images);

        create_scene();

        uint32_t i = 0;

        for(const auto &p : bg_image_paths)
        {
            async_load_texture(p, [this, num_bg_images, i](gl::Texture t)
            {
                m_parallax_textures[i] = t;
                m_parallax_meshes[i]->material()->add_texture(t);
            }, true, true);
            i++;
        }

        // retrieve balloon assets
        auto balloon_image_paths = fs::get_directory_entries(fs::join_paths(*m_asset_dir, "balloon"),
                                                             fs::FileType::IMAGE, true);

        if(!balloon_image_paths.empty())
        {
            async_load_texture(balloon_image_paths[0], [this](gl::Texture t)
            {
                m_sprite_texture = t;
                m_sprite_mesh->material()->add_texture(t);
            }, true, true);
        }
    }
    else if(the_property == m_max_num_balloons)
    {
        m_current_num_balloons = *m_max_num_balloons;
    }
    else if(the_property == m_offscreen_res)
    {
        glm::ivec2 size = *m_offscreen_res;

        if(size.x == 0 || size.y == 0){ size = gl::window_dimension(); }

        gl::Fbo::Format fmt;
        fmt.num_samples = 8;
        m_offscreen_fbo = gl::Fbo::create(size, fmt);

        fmt.num_samples = 0;
        fmt.depth_buffer = false;
        for(auto &fbo : m_blur_fbos)
        {
            fbo = gl::Fbo::create(size, fmt);
            fbo->texture_ptr()->set_wrap(GL_REPEAT, GL_REPEAT);
        }
    }
    else if(the_property == m_use_syphon)
    {
        m_syphon_out = *m_use_syphon ? syphon::Output("balloon") : syphon::Output();
    }
}

gl::MeshPtr BalloonApp::create_sprite_mesh(const gl::Texture &t)
{
    auto mat = gl::Material::create();
    if(t){ mat->add_texture(t); }
    mat->set_blending();
    auto c = gl::COLOR_WHITE;
    c.a = .999f;
    mat->set_diffuse(c);
//    mat->set_depth_write(false);
    gl::MeshPtr ret = gl::Mesh::create(gl::Geometry::create_plane(2, 2), mat);
    return ret;
}

void BalloonApp::create_scene()
{
    scene()->clear();
    float z_val = -10.f;

    // parallax backgrounds
    for(uint32 i = 0; i < m_parallax_meshes.size(); ++i)
    {
        m_parallax_meshes[i] = create_sprite_mesh();
        m_parallax_meshes[i]->set_position(glm::vec3(0.f, 0.f, z_val));
        scene()->add_object(m_parallax_meshes[i]);
        z_val -= .1f;
        m_parallax_meshes[i]->add_tag(g_parallax_bg_tag);
        m_parallax_meshes[i]->set_name("parallax_bg_0" + to_string(i));
    }

    // static background
    m_bg_mesh = create_sprite_mesh();
    m_bg_mesh->set_position(glm::vec3(0.f, 0.f, z_val));
    scene()->add_object(m_bg_mesh);
    m_bg_mesh->set_name("static background");

    m_sprite_mesh = create_sprite_mesh();
    auto sprite_handle = gl::Object3D::create("balloon_handle");
    sprite_handle->add_child(m_sprite_mesh);
    scene()->add_object(sprite_handle);
    m_sprite_mesh->set_scale(glm::vec3(m_sprite_size->value() / gl::window_dimension(), 1.f));
    m_sprite_mesh->set_name("balloon");
}

void BalloonApp::explode_balloon()
{
    m_current_num_balloons--;

    if(!m_current_num_balloons)
    {
        LOG_DEBUG << "crash!";
        m_current_num_balloons = *m_max_num_balloons;
    }
    else{ LOG_DEBUG << "explode_balloon: " << m_current_num_balloons << " left ..."; }
}
