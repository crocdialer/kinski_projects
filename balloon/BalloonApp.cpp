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
const std::string g_balloon_tag = "balloon";
const std::string g_tombstone_tag = "tombstone";
const std::string g_tag_tomb_balloon = "tomb_balloon";
const std::string g_balloon_handle_name = "balloon_handle";
const std::string g_character_handle_name = "zed_handle";

const glm::vec2 g_tombstone_y_pos_min = glm::vec2(-1.f, -.71f);
const glm::vec2 g_tombstone_y_pos_max = glm::vec2(1.f, -.55f);

std::map<BalloonApp::GamePhase, std::string> g_state_names =
{
    {BalloonApp::GamePhase::IDLE, "IDLE"},
    {BalloonApp::GamePhase::FLOATING, "FLOATING"},
    {BalloonApp::GamePhase::CRASHED, "CRASHED"}
};

const std::vector<gl::Color> g_balloon_color_palette =
{
    gl::Color(1.f, 0.976f, 0.749f, 1.f),
    gl::Color(1.f, 0.941f, 0.4f, 1.f)
};

/////////////////////////////////////////////////////////////////

void BalloonApp::setup()
{
    ViewerApp::setup();

    // load larger glyphs
    fonts()[1].load(fonts()[0].path(), 96);

    register_property(m_asset_dir);
    register_property(m_offscreen_res);
    register_property(m_num_dead);
    register_property(m_max_num_balloons);
    register_property(m_sprite_size);
    register_property(m_timeout_balloon_explode);
    register_property(m_balloon_noise_intensity);
    register_property(m_balloon_noise_speed);
    register_property(m_balloon_scale);
    register_property(m_balloon_line_length);
    register_property(m_parallax_factor);
    register_property(m_motion_blur);
    register_property(m_float_speed_mutliplier);
    register_property(m_float_speed);
    register_property(m_use_syphon);
    m_crash_sites->set_tweakable(false);
    register_property(m_crash_sites);
    observe_properties();

    // create timers
    create_timers();
    
    // create animations
    create_animations();
    
    // load settings from file
    load_settings();
    
    change_gamephase(GamePhase::IDLE);
}

/////////////////////////////////////////////////////////////////

void BalloonApp::update(float the_delta_time)
{
    ViewerApp::update(the_delta_time);

    // construct ImGui window for this frame
    if(display_gui())
    {
        gui::draw_component_ui(shared_from_this());
        gui::draw_scenegraph_ui(scene(), &selected_objects());
        auto obj = selected_objects().empty() ? nullptr : *selected_objects().begin();
        gui::draw_object3D_ui(obj, m_2d_cam);
        
        if(*m_use_warping){ gui::draw_component_ui(m_warp_component); }
    }
    
    // check for offscreen fbo
    if(!m_offscreen_fbo || m_blur_fbos.empty() || !m_blur_fbos.front()){ m_offscreen_res->notify_observers(); }
    
    if(m_dirty_scene){ create_scene(); }
    
    if(m_game_phase == GamePhase::FLOATING)
    {
        *m_float_speed = map_value(m_current_num_balloons / (float) m_max_num_balloons->value(),
                                   0.f, 1.f, m_float_speed->range().first, m_float_speed->range().second);

        // update sprite movie texture
        float balloons_frac = 1.f - (m_current_num_balloons / (float)m_max_num_balloons->value());
        uint32_t sprite_index = balloons_frac * (m_sprite_movies.size() - 1);
        sprite_index = clamp<uint32_t>(sprite_index, 0, m_sprite_movies.size() - 2);
        
        // shitty hack for correct index
        if(!m_current_num_balloons){ sprite_index = m_sprite_movies.size() - 1; }
        
        // play zed_audio
        if(m_current_sprite_index != sprite_index)
        {
            m_current_sprite_index = sprite_index;
            m_zed_sounds[sprite_index]->restart();
        }
        for(uint32_t i = 0; i < m_sprite_movies.size(); ++i)
        {
            if(i != sprite_index){ m_sprite_movies[i]->pause(); }
        }
        if(m_sprite_movies.size() > sprite_index){ m_sprite_movies[sprite_index]->play(); }

        if(!m_sprite_movies.empty() && m_sprite_movies[sprite_index]->copy_frame_to_texture(m_sprite_texture, true))
        {
            m_sprite_mesh->material()->add_texture(m_sprite_texture);
        }

        // pow movie
        if(m_balloon_pow_movie && m_balloon_pow_movie->copy_frame_to_texture(m_pow_texture, true))
        {
            m_pow_mesh->material()->add_texture(m_pow_texture);
        }
    }
    else if(m_game_phase == GamePhase::CRASHED)
    {
        // update movie texture
        if(m_corpse_movie && m_corpse_movie->copy_frame_to_texture(m_corpse_texture, true))
        {
            m_corpse_mesh->material()->add_texture(m_corpse_texture);
        }
    }

    // animate bg textures
    float factor = m_current_float_speed;//*m_float_speed;

    gl::SelectVisitor<gl::Mesh> visitor({g_parallax_bg_tag}, false);
    scene()->root()->accept(visitor);
    uint32_t i = 0;

    auto blur_factor = [](float f) -> float
    {
        return 50.f * f * f * f;
    };

    gl::reset_state();

    for(auto m : visitor.get_objects())
    {
        auto val = blur_factor(factor) * *m_motion_blur;

        auto *tex = m->material()->get_texture_ptr();

        if(m_parallax_textures[i])
        {
            gl::render_to_texture(m_blur_fbos[i], [this, i, val]()
            {
                gl::clear(gl::Color(0.f));
                gl::Blur blur(glm::vec2(0.f, val));
                blur.render_output(m_parallax_textures[i]);
            });
            tex = m_blur_fbos[i]->texture_ptr();

            // set region of interest
            Area_<uint32_t> roi(0, 0, tex->width(), tex->height() / 3.f);
            tex->set_roi(roi);

            m->material()->add_texture(*tex);
            i++;
        }

        if(tex)
        {
            tex->set_uvw_offset(tex->uvw_offset() + gl::Y_AXIS * the_delta_time * factor);
            factor /= *m_parallax_factor;
        }
    }

    // simplex movement
    glm::vec2 pos_offset = glm::vec2(glm::simplex(glm::vec2(get_application_time() * m_balloon_noise_speed->value().x, 0.f)),
                                     glm::simplex(glm::vec2(get_application_time() * m_balloon_noise_speed->value().y, 0.2f)));

    pos_offset *= m_balloon_noise_intensity->value() / glm::vec2(m_offscreen_fbo->size());

    // balloon sprite scaling / positioning
    if(m_sprite_mesh)
    {
        m_sprite_mesh->set_scale(glm::vec3(m_sprite_size->value() / gl::window_dimension(), 1.f));
        m_sprite_mesh->set_position(glm::vec3(pos_offset + m_zed_offset, 0.f));
    }

    if(m_game_phase == GamePhase::IDLE && m_title_mesh && m_animations[ANIM_FOREGROUND_IN]->finished())
    {
        m_title_mesh->position().xy = .5f * pos_offset;
    }

    update_balloon_cloud(the_delta_time);
    
    m_current_float_speed = mix<float>(m_current_float_speed, *m_float_speed * *m_float_speed_mutliplier,
                                       0.03f);
}

/////////////////////////////////////////////////////////////////

void BalloonApp::update_balloon_cloud(float the_delta_time)
{
    auto balloons = scene()->get_objects_by_tag(g_balloon_tag);
    
    for(uint32_t i = 0; i < m_balloon_particles.size(); ++i)
    {
        m_balloon_particles[i].force += glm::vec2(0.f, 7.5f);
        
        for(uint32_t j = 0; j < m_balloon_particles.size(); ++j)
        {
            if(i == j){ continue; }
            auto diff = m_balloon_particles[i].position - m_balloon_particles[j].position;
            float distance = length(diff);
            
            if(distance <= (m_balloon_particles[i].radius + m_balloon_particles[j].radius))
            {
                m_balloon_particles[i].force += (diff / distance) / (1.f + distance * distance);
            }
        }
        // apply force, update velocity
        m_balloon_particles[i].velocity += m_balloon_particles[i].force * the_delta_time;
        m_balloon_particles[i].force = glm::vec2(0);
        
        // update position
        m_balloon_particles[i].position += m_balloon_particles[i].velocity * the_delta_time;
    }
    
    // balloon lines
    auto &line_verts = m_balloon_lines_mesh->geometry()->vertices();
    line_verts.resize(m_current_num_balloons * 2);
    
    // set position for gameobjects
    for(uint32_t i = 0; i < m_balloon_particles.size(); ++i)
    {
        // apply contraints (string length, etc.)
        glm::vec2 anchor = vec2(0.f);
        glm::vec2 pos_diff = m_balloon_particles[i].position - anchor;
        const float max_diff = m_balloon_particles[i].string_length;
        
        if(glm::length(pos_diff) > max_diff)
        {
            m_balloon_particles[i].position = anchor + max_diff * glm::normalize(pos_diff);
        }
        
        auto pos_2d = m_balloon_particles[i].position;
        balloons[i]->set_position(glm::vec3(pos_2d, balloons[i]->position().z));
        
        line_verts[2 * i] = glm::vec3(0.f, 0.f, balloons[i]->position().z);
        line_verts[2 * i + 1] = glm::vec3(pos_2d, balloons[i]->position().z);
        
        // rotation
        glm::quat rotation_quat = glm::rotation(glm::vec3(0.f, 1.f, 0.f),
                                                glm::normalize(glm::vec3(pos_2d, 0.f)));
        
        balloons[i]->set_rotation(rotation_quat);
        
        // add random balloon texture, if missing
        auto mesh = std::dynamic_pointer_cast<gl::Mesh>(balloons[i]);
        if(mesh && !mesh->material()->get_texture() && !m_balloon_textures.empty())
        {
            auto index = (random(0.f, 1.f) < 0.85f) ? 0 : 1;
            mesh->material()->add_texture(m_balloon_textures[index]);
            if(!index){ mesh->material()->set_diffuse(random_balloon_color()); }
        }
    }
    m_balloon_lines_mesh->geometry()->create_gl_buffers();
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
            if(m_game_phase == GamePhase::IDLE){ change_gamephase(GamePhase::FLOATING); }
            else if(m_game_phase == GamePhase::FLOATING){ explode_balloon(); }
            break;
        case Key::_T:
        {
            auto &crash_sites = m_crash_sites->value();
            crash_sites.clear();

            // remove old graves
            auto old_tombstones = scene()->get_objects_by_tag(g_tombstone_tag);
            for(auto &m : old_tombstones){ scene()->remove_object(m); }

            for(uint32_t i = 0; i < 20; ++i){ add_random_tombstone(); }

            break;
        }
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

        // retrieve parallax background assets
        auto bg_image_paths = fs::get_directory_entries(fs::join_paths(*m_asset_dir, "parallax_backgrounds"),
                                                        fs::FileType::IMAGE, true);
        auto num_bg_images = bg_image_paths.size();
        m_parallax_meshes.resize(num_bg_images);
        m_parallax_textures.resize(num_bg_images);
        m_blur_fbos.resize(num_bg_images);
        
        create_scene();
        
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
        
        // retrieve static foreground
        auto static_fg_path = fs::get_directory_entries(fs::join_paths(*m_asset_dir, "foreground"),
                                                        fs::FileType::IMAGE, true);
        
        if(!static_fg_path.empty())
        {
            async_load_texture(static_fg_path[0], [this](gl::Texture t)
            {
                t.set_roi(0, 2, t.width(), t.height() - 2);
                m_fg_mesh->material()->add_texture(t);
            }, true, true);
        }

        uint32_t i = 0;

        for(const auto &p : bg_image_paths)
        {
            async_load_texture(p, [this, i](gl::Texture t)
            {
                m_parallax_textures[i] = t;
                m_parallax_meshes[i]->material()->add_texture(t);
            }, true, true);
            i++;
        }
        
        // movie sprites
        auto zed_movie_paths = fs::get_directory_entries(fs::join_paths(*m_asset_dir, "zed"),
                                                         fs::FileType::MOVIE, true);
        m_sprite_movies.clear();
        
        for(const auto &p : zed_movie_paths)
        {
            m_sprite_movies.push_back(media::MediaController::create(p, true, true));
        }

        // zed sounds
        auto zed_sound_paths = fs::get_directory_entries(fs::join_paths(*m_asset_dir, "zed"),
                                                         fs::FileType::AUDIO, true);

        for(const auto &p : zed_sound_paths)
        {
            auto sound = media::MediaController::create(p, false, false);
            sound->set_on_load_callback([](media::MediaControllerPtr mc)
            {
                mc->set_volume(.5f);
            });
            m_zed_sounds.push_back(sound);
        }

        // retrieve dead zed movie
        auto corpse_paths = fs::get_directory_entries(fs::join_paths(*m_asset_dir, "corpse"),
                                                     fs::FileType::MOVIE, true);
        
        if(!corpse_paths.empty())
        {
            m_corpse_movie = media::MediaController::create(corpse_paths.front(), false, false);
        }

        auto pow_paths = fs::get_directory_entries(fs::join_paths(*m_asset_dir, "pow"),
                                                   fs::FileType::MOVIE, true);

        if(!pow_paths.empty())
        {
            m_balloon_pow_movie = media::MediaController::create(pow_paths.front(), false, false);
        }

        auto pow_audio_paths = fs::get_directory_entries(fs::join_paths(*m_asset_dir, "pow"),
                                                   fs::FileType::AUDIO, true);

        for(auto &p : pow_audio_paths)
        {
            m_balloon_pow_sounds.push_back(media::MediaController::create(p, false, false));
        }

        // retrieve balloon textures
        auto balloon_image_paths = fs::get_directory_entries(fs::join_paths(*m_asset_dir, "balloon"),
                                                         fs::FileType::IMAGE, true);
        
        m_balloon_textures.clear();
        
        for(const auto &p : balloon_image_paths)
        {
            m_balloon_textures.push_back(gl::create_texture_from_file(p, true, true));
        }
        if(!m_balloon_textures.empty())
        {
            m_tomb_balloon->material()->add_texture(m_balloon_textures.back());
        }


        // retrieve title image
        auto title_path = fs::get_directory_entries(fs::join_paths(*m_asset_dir, "title"),
                                                    fs::FileType::IMAGE, true);
        
        if(!title_path.empty())
        {
            async_load_texture(title_path[0], [this](gl::Texture t)
            {
                m_title_mesh->material()->add_texture(t);
            }, true, true);
        }
        
        // retrieve tombstone image
        auto tombstone_paths = fs::get_directory_entries(fs::join_paths(*m_asset_dir, "tombstone"),
                                                         fs::FileType::IMAGE, true);
        
        if(!tombstone_paths.empty())
        {
            async_load_texture(tombstone_paths[0], [this](gl::Texture t)
            {
                m_tombstone_texture = t;
                m_tombstone_template->material()->add_texture(t);
            }, true, true);
        }

        // tombstone font
        auto tombstone_font_paths = fs::get_directory_entries(fs::join_paths(*m_asset_dir, "font"),
                                                              fs::FileType::FONT, true);
        if(!tombstone_font_paths.empty())
        {
            m_tombstone_font.load(tombstone_font_paths.front(), 23);
        }

        // background audio
        auto bg_audio_paths = fs::get_directory_entries(fs::join_paths(*m_asset_dir, "background_audio"),
                                                        fs::FileType::AUDIO, true);
        if(!bg_audio_paths.empty())
        {
            m_background_audio = media::MediaController::create(bg_audio_paths.front(), true, true);
        }

        // start audio
        auto start_audio_paths = fs::get_directory_entries(fs::join_paths(*m_asset_dir, "start_audio"),
                                                           fs::FileType::AUDIO, true);
        if(!start_audio_paths.empty())
        {
            m_start_audio = media::MediaController::create(start_audio_paths.front(), false, false);
        }
    }
    else if(the_property == m_max_num_balloons)
    {
        m_current_num_balloons = *m_max_num_balloons;
        create_balloon_cloud();
    }
    else if(the_property == m_offscreen_res)
    {
        glm::ivec2 size = *m_offscreen_res;

        if(size.x == 0 || size.y == 0){ size = gl::window_dimension(); }

        gl::Fbo::Format fmt;
        fmt.color_internal_format = GL_RGB;
        fmt.num_samples = 8;
        m_offscreen_fbo = gl::Fbo::create(size, fmt);

        fmt.color_internal_format = GL_RGBA;
        fmt.depth_buffer = false;
        fmt.num_samples = 0;
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
    else if(the_property == m_crash_sites)
    {
        auto &crash_sites = m_crash_sites->value();
        
        // remove old graves
        auto old_tombstones = scene()->get_objects_by_tag(g_tombstone_tag);
        for(auto &m : old_tombstones){ scene()->remove_object(m); }
        
        for(uint32_t i = 0; i < crash_sites.size(); ++i)
        {
            create_tombstone_mesh(i, crash_sites[i]);
        }
        
        change_gamephase(GamePhase::IDLE);
    }
}

gl::MeshPtr BalloonApp::create_sprite_mesh(const gl::Texture &t)
{
    auto mat = gl::Material::create();
    if(t){ mat->add_texture(t); }
    mat->set_blending();
    mat->set_culling(gl::Material::CULL_NONE);
    auto c = gl::COLOR_WHITE;
    c.a = .999f;
    mat->set_diffuse(c);
    gl::MeshPtr ret = gl::Mesh::create(gl::Geometry::create_plane(2, 2), mat);
    return ret;
}

void BalloonApp::create_scene()
{
    scene()->clear();
    float z_val = 2.5f;
    float parallax_alpha = .9f;
    
    // parallax backgrounds
    for(uint32 i = 0; i < m_parallax_meshes.size(); ++i)
    {
        m_parallax_meshes[i] = create_sprite_mesh();
        auto color = gl::Color(1.f, 1.f, 1.f, parallax_alpha);
        m_parallax_meshes[i]->material()->set_diffuse(color);
        parallax_alpha -= .3f;
        
        m_parallax_meshes[i]->set_position(glm::vec3(0.f, 0.f, z_val));
        scene()->add_object(m_parallax_meshes[i]);
        z_val -= 5.f;
        m_parallax_meshes[i]->add_tag(g_parallax_bg_tag);
        m_parallax_meshes[i]->set_name("parallax_bg_0" + to_string(i));
    }

    // static background
    z_val = -10.f;
    m_bg_mesh = create_sprite_mesh();
    m_bg_mesh->set_position(glm::vec3(0.f, 0.f, z_val));
    scene()->add_object(m_bg_mesh);
    m_bg_mesh->set_name("static background");

    // foreground
    z_val = -.01f;
    m_fg_mesh = create_sprite_mesh();
    m_fg_mesh->set_position(glm::vec3(0.f, 0.f, z_val));
    scene()->add_object(m_fg_mesh);
    m_fg_mesh->set_name("foreground");
    
    // main sprite
    m_sprite_mesh = create_sprite_mesh();
    auto sprite_handle = gl::Object3D::create(g_character_handle_name);
    sprite_handle->add_child(m_sprite_mesh);
    scene()->add_object(sprite_handle);
    m_sprite_mesh->set_scale(glm::vec3(m_sprite_size->value() / gl::window_dimension(), 1.f));
    m_sprite_mesh->set_name("zed");
    auto balloon_handle = gl::Object3D::create(g_balloon_handle_name);
    balloon_handle->set_position(glm::vec3(0.f, 0.73f, 0.f));
    m_sprite_mesh->add_child(balloon_handle);
    
    // generate balloons
    create_balloon_cloud();

    // balloon explosion mesh
    m_pow_mesh = create_sprite_mesh();
    m_pow_mesh->set_name("explosion pow");
    m_pow_mesh->set_scale(1.5f);
    m_pow_mesh->set_position(glm::vec3(0.f, 0.f, .1f));

    // crashed corpse
    z_val = 5.f;
    m_corpse_mesh = create_sprite_mesh();
    m_corpse_mesh->set_name("zed corpse");
    m_corpse_mesh->set_position(glm::vec3(0.f, 0.f, z_val));
    m_fg_mesh->add_child(m_corpse_mesh);
    
    // title
    m_title_mesh = create_sprite_mesh();
    m_title_mesh->set_name("title");
    m_title_mesh->set_position(glm::vec3(0.f, 2.f, z_val));
    scene()->add_object(m_title_mesh);
    
    // tombstone template
    m_tombstone_template = create_sprite_mesh();
    m_tombstone_template->add_tag(g_tombstone_tag);
    m_tombstone_template->set_scale(glm::vec3(.125f, .085f, 1.f));
    m_tombstone_template->material()->set_culling(gl::Material::CULL_NONE);

    // tomb balloon
    m_tomb_balloon = create_sprite_mesh();
    m_tomb_balloon->set_name("tomb balloon");
    m_tomb_balloon->add_tag(g_tag_tomb_balloon);
    m_tomb_balloon->set_position(glm::vec3(0.05f, 2.86f, 0.1f));
    auto balloon_string = create_sprite_mesh();
    balloon_string->set_scale(glm::vec3(.05f, 1.f, 1.f));
    balloon_string->set_position(glm::vec3(0.f, -1.f, -0.001f));
    m_tomb_balloon->add_child(balloon_string);
}

void BalloonApp::create_balloon_cloud()
{
    // balloon cloud
    float z_val = 0.04f;
    auto balloon_handle = scene()->get_object_by_name(g_balloon_handle_name);
    if(!balloon_handle){ return; }
    balloon_handle->children().clear();
    m_balloon_particles.clear();
    
    auto rect_geom = gl::Geometry::create_plane(1.f, 1.f);
    
    // individual balloons
    for(uint32_t i = 0; i < m_current_num_balloons; ++i)
    {
        auto mat = gl::Material::create();
        mat->set_blending();
        mat->set_diffuse(gl::Color(1.f, 1.f, 1.f, .999f));
        auto balloon = gl::Mesh::create(rect_geom, mat);
        balloon->add_tag(g_balloon_tag);
        balloon->set_name(g_balloon_tag + "_" + to_string(i));
        balloon->set_scale(glm::vec3(.5f, 0.6f, 1.f));
        float line_length = random(.75f, 1.2f);
        auto pos_2D = glm::circularRand(0.2f);
        balloon_particle_t b = {pos_2D, glm::vec2(0.f), glm::vec2(0.f), 0.35f, line_length};
        balloon->set_position(glm::vec3(pos_2D, z_val));
        balloon_handle->add_child(balloon);
        m_balloon_particles.push_back(b);
        z_val += 0.01f;
    }
    
    // balloon string mesh
    auto line_geom = gl::Geometry::create();
    line_geom->vertices().resize(m_current_num_balloons * 2);
    line_geom->colors().resize(m_current_num_balloons * 2, gl::COLOR_WHITE);
    line_geom->set_primitive_type(GL_LINES);
    auto line_mat = gl::Material::create(gl::ShaderType::LINES_2D);
    line_mat->uniform("u_line_thickness", 3.f);
    line_mat->set_line_width(11.f);
    line_mat->uniform("u_window_size", gl::window_dimension());
    m_balloon_lines_mesh = gl::Mesh::create(line_geom, line_mat);
    m_balloon_lines_mesh->set_name("balloon_lines");
    balloon_handle->add_child(m_balloon_lines_mesh);
    m_dirty_scene = false;
}

void BalloonApp::create_timers()
{
    m_balloon_timer = Timer(main_queue().io_service());
}

void BalloonApp::explode_balloon()
{
    // should not happen anyway
    if(m_game_phase != GamePhase::FLOATING){ return; }
    
    if(!m_balloon_timer.has_expired())
    {
        LOG_DEBUG << "blocked explode: waiting for balloon timeout ...";
        return;
    }
    
    m_current_num_balloons--;
    
    auto balloons = scene()->get_objects_by_tag(g_balloon_tag);
    if(!balloons.empty())
    {
        auto b = balloons.front();
        b->add_child(m_pow_mesh);
        m_balloon_pow_movie->restart();
        m_balloon_pow_movie->set_media_ended_callback([this, b](media::MediaControllerPtr mc)
        {
           main_queue().submit([this, b]()
           {
               scene()->remove_object(b);
               m_balloon_particles.pop_front();
           });
        });

        // random pow sound
        if(!m_balloon_pow_sounds.empty())
        {
            auto audio_index = random_int<uint32_t>(0, m_balloon_pow_sounds.size() - 1);
            m_balloon_pow_sounds[audio_index]->restart();
        }
    }
    
    // start recovery timer
    m_balloon_timer.expires_from_now(*m_timeout_balloon_explode);
    
    // no balloons left -> crash
    if(!m_current_num_balloons)
    {
        LOG_DEBUG << "crash!";
        
        // animation removes sprite and trigger GamePhase::CRASHED afterwards
        m_animations[ANIM_ZED_OUT]->start();
    }
    // go on
    else
    {
        rumble_balloons();
        
        // play drop animation
        m_animations[ANIM_ZED_DROP]->start();
        LOG_DEBUG << "explode_balloon: " << m_current_num_balloons << " left ...";
    }
}

void BalloonApp::rumble_balloons()
{
    for(auto &b : m_balloon_particles)
    {
        float f = random(-2.5f, -1.5f);
        b.velocity = f * b.position;
    }
}

bool BalloonApp::is_state_change_valid(GamePhase the_phase, GamePhase the_next_phase)
{
    // sanity check
    switch(the_phase)
    {
        case GamePhase::UNDEFINED:
            return the_next_phase == GamePhase::IDLE;
            break;
        case GamePhase::IDLE:
            return the_next_phase == GamePhase::FLOATING;
            break;
        case GamePhase::FLOATING:
            return the_next_phase == GamePhase::FLOATING || the_next_phase == GamePhase::CRASHED;
            break;
        case GamePhase::CRASHED:
            return the_next_phase == GamePhase::IDLE;
            break;
        default:
            return false;
    }
}

bool BalloonApp::change_gamephase(GamePhase the_next_phase)
{
    if(!is_state_change_valid(m_game_phase, the_next_phase))
    {
        LOG_DEBUG << "invalid state change requested: " << g_state_names[m_game_phase] << " -> " << g_state_names[the_next_phase];
        return false;
    }
    
    switch(the_next_phase)
    {
        case GamePhase::IDLE:
            *m_float_speed = 0.f;
            m_animations[ANIM_TITLE_IN]->start();
            m_corpse_mesh->set_enabled(false);
            m_sprite_mesh->set_enabled(false);
            m_parallax_meshes.front()->set_enabled(false);
            break;
            
        case GamePhase::FLOATING:
            m_sprite_mesh->set_enabled(true);
            m_parallax_meshes.front()->set_enabled(true);

            // start animations
            m_animations[ANIM_TITLE_OUT]->start();
            m_animations[ANIM_FOREGROUND_OUT]->start();
            m_animations[ANIM_ZED_IN]->start();

            // play start movie
            m_start_audio->restart();

            // start first sprite movie
            if(!m_sprite_movies.empty() && m_sprite_movies.front()){ m_sprite_movies.front()->play(); }
            
            // stop corpse movie
            if(m_corpse_movie){ m_corpse_movie->pause(); }
            
            m_current_num_balloons = *m_max_num_balloons;
            create_balloon_cloud();
            break;
            
        case GamePhase::CRASHED:
        {
            *m_float_speed = 0.f;
            m_corpse_mesh->set_enabled(true);

            // create new tombstone and animation
            auto tomb_stone = add_random_tombstone();
            tomb_stone->set_enabled(false);
            tomb_stone->add_child(m_tomb_balloon);
            auto color_opaque = gl::Color(1.f, 1.f, 1.f, .999f);
            m_tomb_balloon->material()->set_diffuse(color_opaque);
            for(auto &c : m_tomb_balloon->children())
            {
                auto child_mesh = std::dynamic_pointer_cast<gl::Mesh>(c);
                if(child_mesh){ child_mesh->material()->set_diffuse(color_opaque); }
            }
            create_tombstone_animations(tomb_stone, random(5.f, 15.f), 0.f);

            main_queue().submit_with_delay([this, tomb_stone]()
            {
                tomb_stone->set_enabled(true);
                m_animations[ANIM_TOMBSTONE_IN]->start();
            }, random(10.f, 40.f));


            // start corpse movie
            if(m_corpse_movie)
            {
                m_corpse_movie->restart();
                m_corpse_movie->set_media_ended_callback([this](media::MediaControllerPtr mc)
                {
                    change_gamephase(GamePhase::IDLE);
                });
            }

            // stop whichever sprite movie was playing
            for(auto &m : m_sprite_movies)
            {
                if(m)
                { m->pause(); }
            }

            m_animations[ANIM_FOREGROUND_IN]->start();
            break;
        }
        case GamePhase::UNDEFINED:
        default:
            return false;
    }
    m_game_phase = the_next_phase;
    LOG_DEBUG << "state: " << g_state_names[m_game_phase];
    return true;
}

void BalloonApp::create_animations()
{
    m_animations[ANIM_TITLE_IN] = std::make_shared<animation::Animation>(2.f, 0.f,
    [this](float progress)
    {
        auto z_val = m_title_mesh->position().z;
        glm::vec3 pos = kinski::mix(glm::vec3(0.f, 2.f, z_val), glm::vec3(0.f, 0.f, z_val), progress);
        m_title_mesh->set_position(pos);
    });
    
    m_animations[ANIM_TITLE_OUT] = std::make_shared<animation::Animation>(2.f, 0.f,
    [this](float progress)
    {
        auto z_val = m_title_mesh->position().z;
        glm::vec3 pos = kinski::mix(glm::vec3(0.f, 2.f, z_val), glm::vec3(0.f, 0.f, z_val), 1.f - progress);
        m_title_mesh->set_position(pos);
    });
    
    m_animations[ANIM_FOREGROUND_IN] = std::make_shared<animation::Animation>(2.f, 0.f,
                                                                              [this](float progress)
    {
        auto z_val = m_fg_mesh->position().z;
        glm::vec3 pos = kinski::mix(glm::vec3(0.f, -2.f, z_val), glm::vec3(0.f, 0.f, z_val), progress);
        m_fg_mesh->set_position(pos);
        m_parallax_meshes[0]->material()->set_diffuse(gl::Color(1.f, 1.f, 1.f, .9f * (1.f - progress)));
    });
    
    m_animations[ANIM_FOREGROUND_OUT] = std::make_shared<animation::Animation>(2.f, 0.f,
                                                                               [this](float progress)
    {
        auto z_val = m_fg_mesh->position().z;
        glm::vec3 pos = kinski::mix(glm::vec3(0.f, -2.f, z_val), glm::vec3(0.f, 0.f, z_val), 1.f - progress);
        m_fg_mesh->set_position(pos);
        m_parallax_meshes[0]->material()->set_diffuse(gl::Color(1.f, 1.f, 1.f, .9f * progress));
    });
    
    // balloon exploded
    m_animations[ANIM_ZED_DROP] = animation::create(&m_zed_offset,
                                                    glm::vec2(0.f), glm::vec2(0.f, -0.3f),
                                                    0.5f);
    
    m_animations[ANIM_ZED_DROP]->set_ease_function(animation::EaseOutElastic(3.f, 2.f));
    
    m_animations[ANIM_ZED_DROP]->set_finish_callback([this]()
    {
        m_animations[ANIM_ZED_DROP_RECOVER] = animation::create(&m_zed_offset, m_zed_offset, glm::vec2(0.f), 2.f);
        m_animations[ANIM_ZED_DROP_RECOVER]->set_ease_function(animation::EaseOutCubic());
        m_animations[ANIM_ZED_DROP_RECOVER]->start();
    });
    
    auto out_of_screen_pos = glm::vec2(0.f, -1.5f);
    
    // zed crashed
    m_animations[ANIM_ZED_OUT] = std::make_shared<animation::Animation>(1.5f, 0.f,
    [this, out_of_screen_pos](float progress)
    {
        m_zed_offset = mix(glm::vec2(0.f), out_of_screen_pos, progress);
    });
    m_animations[ANIM_ZED_OUT]->set_ease_function(animation::EaseInQuad());
    
    m_animations[ANIM_ZED_OUT]->set_finish_callback([this]()
    {
        main_queue().submit_with_delay([this]()
        {
            change_gamephase(GamePhase::CRASHED);
        }, 2.f);
    });
    
    // zed reborn
    m_animations[ANIM_ZED_IN] = std::make_shared<animation::Animation>(2.f, 0.f,
    [this, out_of_screen_pos](float progress)
    {
        m_zed_offset = mix(out_of_screen_pos, glm::vec2(0.f), progress);
    });
}

gl::Color BalloonApp::random_balloon_color()
{
    auto col_index = random_int<uint32_t>(0, g_balloon_color_palette.size() - 1);
    auto color = g_balloon_color_palette[col_index];
    color.a = .999f;
    return color;
}

gl::MeshPtr BalloonApp::add_random_tombstone()
{
    auto pos = glm::linearRand(g_tombstone_y_pos_min, g_tombstone_y_pos_max);
    m_crash_sites->value().push_back(pos);
    *m_num_dead = *m_num_dead + 1;
    auto m = create_tombstone_mesh(*m_num_dead, pos);
    m_fg_mesh->add_child(m);
    return m;
}

gl::MeshPtr BalloonApp::create_tombstone_mesh(uint32_t the_index, const glm::vec2 &the_pos)
{
    gl::MeshPtr m = m_tombstone_template->copy();
    m->set_position(glm::vec3(the_pos, 1.f - the_pos.y));
    glm::vec3 scale = m->scale();
    float scale_y_frac = (the_pos.y - g_tombstone_y_pos_min.y) / (g_tombstone_y_pos_max.y - g_tombstone_y_pos_min.y);
    scale.xy = scale.xy * mix(1.f, .6f, scale_y_frac);
    bool flip_x = false;//random_int(0, 1);
    if(flip_x){ scale.x *= -1.f; }
    m->set_scale(scale);
    auto transform = glm::rotate(m->transform(), random<float>(glm::radians(-10.f), glm::radians(10.f)), gl::Z_AXIS);
    m->set_transform(transform);
    m->set_name("tombstone_" + to_string(the_index));

    // index label with font
    std::string label_string = "zed\n#" + (the_index < 10 ? string("0") : string("")) + to_string(the_index);
    auto label_mesh = m_tombstone_font.create_mesh(label_string)->copy();
    label_mesh->set_position(glm::vec3(glm::vec2(0.3f * (flip_x ? 1.f : -1.f), -0.25f), 0.0001f));
    label_mesh->material()->set_blending();
    label_mesh->material()->set_diffuse(gl::Color(0.2f, 0.2f, .2f, .999f));
    label_mesh->material()->set_culling(gl::Material::CULL_NONE);
    glm::vec3 label_scale = glm::vec3(glm::vec2(.7f / label_mesh->aabb().width()), 1.f);
    if(flip_x){ label_scale.x *= -1.f; }
    label_mesh->set_scale(label_scale);
    label_mesh->set_name("tombstone_label_" + to_string(the_index));
    m->add_child(label_mesh);
    return m;
}

void BalloonApp::create_tombstone_animations(const gl::MeshPtr &the_tombstone_mesh, float duration, float delay)
{
    auto start_pos = the_tombstone_mesh->position() + glm::vec3(0.f, 2.f, 0.f);
    auto final_pos = the_tombstone_mesh->position();

    // tombstone drop
    m_animations[ANIM_TOMBSTONE_IN] = std::make_shared<animation::Animation>(duration, delay,
    [start_pos, final_pos, the_tombstone_mesh](float progress)
    {
        the_tombstone_mesh->set_position(mix(start_pos, final_pos, progress));
    });

    gl::Color color_opaque = gl::COLOR_WHITE, color_transparent = gl::COLOR_WHITE;
    color_opaque.a = 0.999f;
    color_transparent.a = 0.f;

    m_animations[ANIM_TOMBSTONE_OUT] = std::make_shared<animation::Animation>(2.f, 0.f,
    [this, color_opaque, color_transparent](float progress)
    {
        auto color = mix(color_opaque, color_transparent, progress);
        m_tomb_balloon->material()->set_diffuse(color);

        for(auto &c : m_tomb_balloon->children())
        {
            auto child_mesh = std::dynamic_pointer_cast<gl::Mesh>(c);
            if(child_mesh){ child_mesh->material()->set_diffuse(color); }
        }
    });

    m_animations[ANIM_TOMBSTONE_IN]->set_finish_callback([this, the_tombstone_mesh]()
    {
        m_animations[ANIM_TOMBSTONE_OUT]->start();

        m_animations[ANIM_TOMBSTONE_OUT]->set_finish_callback([the_tombstone_mesh]()
        {
            for(auto c : the_tombstone_mesh->children())
            {
                if(c->has_tag(g_tag_tomb_balloon))
                {
                    c->set_parent(nullptr);
                    break;
                }
            }
        });
    });
}
