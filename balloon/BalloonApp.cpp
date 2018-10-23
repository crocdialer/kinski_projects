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
const std::string g_balloon_handle_name = "balloon_handle";
const std::string g_character_handle_name = "zed_handle";

std::map<BalloonApp::GamePhase, std::string> g_state_names =
{
    {BalloonApp::GamePhase::IDLE, "IDLE"},
    {BalloonApp::GamePhase::FLOATING, "FLOATING"},
    {BalloonApp::GamePhase::CRASHED, "CRASHED"}
};

/////////////////////////////////////////////////////////////////

void BalloonApp::setup()
{
    ViewerApp::setup();

    // load larger glyphs
    fonts()[1].load(fonts()[0].path(), 96);

    register_property(m_asset_dir);
    register_property(m_offscreen_res);
    register_property(m_max_num_balloons);
    register_property(m_sprite_size);
    register_property(m_timeout_balloon_explode);
    register_property(m_balloon_noise_intensity);
    register_property(m_balloon_noise_speed);
    register_property(m_balloon_scale);
    register_property(m_parallax_factor);
    register_property(m_motion_blur);
    register_property(m_float_speed);
    register_property(m_use_syphon);
    observe_properties();

    // create timers
    create_timers();
    
    // create animations
    create_animations();
    
    // load settings from file
    load_settings();
    
    change_gamephase(GamePhase::FLOATING);
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
        *m_float_speed = map_value(m_current_num_balloons / (float)m_max_num_balloons->value(),
                                   0.f, 1.f, -1.f, 1.f);
        
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
                m->material()->add_texture(*tex);
                i++;
            }
            
            if(tex)
            {
                tex->set_uvw_offset(tex->uvw_offset() + gl::Y_AXIS * the_delta_time * factor);
                factor /= *m_parallax_factor;
            }
        }
        
        // update movie texture
        if(m_sprite_movie->copy_frame_to_texture(m_sprite_texture, true))
        {
            m_sprite_mesh->material()->add_texture(m_sprite_texture);
            textures()[0] = m_sprite_texture;
        }
        
        // balloon sprite scaling / positioning
        if(m_sprite_mesh)
        {
            m_sprite_mesh->set_scale(glm::vec3(m_sprite_size->value() / gl::window_dimension(), 1.f));
            glm::vec2 pos_offset = glm::vec2(glm::simplex(glm::vec2(get_application_time() * m_balloon_noise_speed->value().x, 0.f)),
                                             glm::simplex(glm::vec2(get_application_time() * m_balloon_noise_speed->value().y, 0.2f)));
            pos_offset *= m_balloon_noise_intensity->value() / glm::vec2(m_offscreen_fbo->size());
            pos_offset += m_zed_offset;
            m_sprite_mesh->set_position(glm::vec3(pos_offset, 0.f));
        }
    }
    
    update_balloon_cloud(the_delta_time);
    
    m_current_float_speed = mix<float>(m_current_float_speed, *m_float_speed, 0.03f);
}

/////////////////////////////////////////////////////////////////

void BalloonApp::update_balloon_cloud(float the_delta_time)
{
    auto balloons = scene()->get_objects_by_tag(g_balloon_tag);
    
    for(uint32_t i = 0; i < m_balloon_particles.size(); ++i)
    {
        glm::vec2 force = glm::vec2(0.f, 7.5f);
        
        for(uint32_t j = 0; j < m_balloon_particles.size(); ++j)
        {
            if(i == j){ continue; }
            auto diff = m_balloon_particles[i].position - m_balloon_particles[j].position;
            float distance = length(diff);
            
            if(distance <= (m_balloon_particles[i].radius + m_balloon_particles[j].radius))
            {
                force += (diff / distance) / (1.f + distance * distance);
            }
        }
        // update velocity
        m_balloon_particles[i].velocity += force * the_delta_time;
        
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
    
    if(m_game_phase == GamePhase::IDLE || m_game_phase == GamePhase::CRASHED)
    {
        gl::draw_text_2D(g_state_names[m_game_phase], fonts()[1], gl::COLOR_WHITE,
                         gl::window_dimension() / 3.f);
    }
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
                                   m_fg_mesh->material()->add_texture(t);
                               }, true, true);
        }
        
        for(uint32 i = 0; i < m_parallax_meshes.size(); ++i)
        {
            m_parallax_meshes[i] = create_sprite_mesh();
        }

        uint32_t i = 0;

        for(const auto &p : bg_image_paths)
        {
            async_load_texture(p, [this, i](gl::Texture t)
            {
                Area_<uint32_t> roi(0, 0, t.width(), t.height() / 3.f);
                t.set_roi(roi);
                m_parallax_textures[i] = t;
                m_parallax_meshes[i]->material()->add_texture(t);
            }, true, true);
            i++;
        }

        // retrieve main-sprite assets
//        auto zed_image_paths = fs::get_directory_entries(fs::join_paths(*m_asset_dir, "zed"),
//                                                             fs::FileType::IMAGE, true);
//
//        if(!zed_image_paths.empty())
//        {
//            async_load_texture(zed_image_paths[0], [this](gl::Texture t)
//            {
//                m_sprite_texture = t;
//                m_sprite_mesh->material()->add_texture(t);
//            }, true, true);
//        }
        
        auto zed_movie_paths = fs::get_directory_entries(fs::join_paths(*m_asset_dir, "zed"),
                                                         fs::FileType::MOVIE, true);
        
        if(!zed_movie_paths.empty())
        {
            m_sprite_movie->load(zed_movie_paths.front(), true, true);
        }
        
        // retrieve main-sprite assets
        auto balloon_image_paths = fs::get_directory_entries(fs::join_paths(*m_asset_dir, "balloon"),
                                                         fs::FileType::IMAGE, true);
        
        m_balloon_textures.clear();
        
        for(const auto &p : balloon_image_paths)
        {
            m_balloon_textures.push_back(gl::create_texture_from_file(p, true, true));
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
    float z_val = .05f;

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
    z_val = -10.f;
    m_bg_mesh = create_sprite_mesh();
    m_bg_mesh->set_position(glm::vec3(0.f, 0.f, z_val));
    scene()->add_object(m_bg_mesh);
    m_bg_mesh->set_name("static background");

    // foreground
    z_val = -5.f;
    m_fg_mesh = create_sprite_mesh();
    m_fg_mesh->set_position(glm::vec3(0.f, -2.f, z_val));
    scene()->add_object(m_fg_mesh);
    m_fg_mesh->set_name("foreground");
    
    m_sprite_mesh = create_sprite_mesh();
    auto sprite_handle = gl::Object3D::create(g_character_handle_name);
    sprite_handle->add_child(m_sprite_mesh);
    scene()->add_object(sprite_handle);
    m_sprite_mesh->set_scale(glm::vec3(m_sprite_size->value() / gl::window_dimension(), 1.f));
    m_sprite_mesh->set_name("zed");
    auto balloon_handle = gl::Object3D::create(g_balloon_handle_name);
    balloon_handle->set_position(glm::vec3(0.f, 0.73f, 0.1f));
    m_sprite_mesh->add_child(balloon_handle);
    
    create_balloon_cloud();
}

void BalloonApp::create_balloon_cloud()
{
    // balloon cloud
    float z_val = .1f;
    auto rect_geom = gl::Geometry::create_plane(1.f, 1.f);
    auto balloon_handle = scene()->get_object_by_name(g_balloon_handle_name);
    balloon_handle->children().clear();
    m_balloon_particles.clear();
    
    auto mat_template = gl::Material::create();
    mat_template->set_blending();
    auto c = gl::COLOR_WHITE;
    c.a = .999f;
    mat_template->set_diffuse(c);
    
    for(uint32_t i = 0; i < *m_max_num_balloons; ++i)
    {
        auto mat = gl::Material::create();
        *mat = *mat_template;
        auto balloon = gl::Mesh::create(rect_geom, mat);
        balloon->add_tag(g_balloon_tag);
        balloon->set_name(g_balloon_tag + "_" + to_string(i));
        balloon->set_scale(glm::vec3(.5f, 0.6f, 1.f));
        float line_length = random(.75f, 1.2f);
        auto pos_2D = glm::circularRand(0.2f);
        balloon_particle_t b = {pos_2D, glm::vec2(0.f), 0.35f, line_length};
        balloon->set_position(glm::vec3(pos_2D, z_val));
        balloon_handle->add_child(balloon);
        m_balloon_particles.push_back(b);
        z_val += 0.1f;
    }
    
    // balloon strings
    auto line_geom = gl::Geometry::create();
    line_geom->vertices().resize(*m_max_num_balloons * 2);
    line_geom->colors().resize(*m_max_num_balloons * 2, gl::COLOR_WHITE);
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
    if(!m_balloon_timer.has_expired())
    {
        LOG_DEBUG << "blocked explode: waiting for balloon timeout ...";
        return;
    }
    
    m_current_num_balloons--;
    
    auto balloons = scene()->get_objects_by_tag(g_balloon_tag);
    if(!balloons.empty())
    {
        scene()->remove_object(balloons.front());
        m_balloon_particles.pop_front();
    }
    
    rumble_balloons();
    
    // play drop animation
    m_animations[ANIM_ZED_DROP]->start();
    LOG_DEBUG << "explode_balloon: " << m_current_num_balloons << " left ...";
    
    // start recovery timer
    m_balloon_timer.expires_from_now(*m_timeout_balloon_explode);
    
    if(!m_current_num_balloons)
    {
        LOG_DEBUG << "crash!";
        change_gamephase(GamePhase::CRASHED);
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
        case GamePhase::IDLE:
            return the_next_phase == GamePhase::FLOATING;
            break;
        case GamePhase::FLOATING:
            return the_next_phase == GamePhase::FLOATING || the_next_phase == GamePhase::CRASHED;
            break;
        case GamePhase::CRASHED:
            return the_next_phase == GamePhase::IDLE;
            break;
    }
    return false;
}

bool BalloonApp::change_gamephase(GamePhase the_next_phase)
{
    if(!is_state_change_valid(m_game_phase, the_next_phase)){ return false; }
    
    switch(the_next_phase)
    {
        case GamePhase::IDLE:
            break;
            
        case GamePhase::FLOATING:
            m_animations[ANIM_FOREGROUND_OUT]->start();
            m_current_num_balloons = *m_max_num_balloons;
            create_balloon_cloud();
            break;
            
        case GamePhase::CRASHED:
            m_animations[ANIM_FOREGROUND_IN]->start();
            main_queue().submit_with_delay([this]()
            {
                change_gamephase(GamePhase::IDLE);
            }, 5.f);
            break;
    }
    m_game_phase = the_next_phase;
    LOG_DEBUG << "state: " << g_state_names[m_game_phase];
    return true;
}

void BalloonApp::create_animations()
{
    m_animations[ANIM_FOREGROUND_IN] = std::make_shared<animation::Animation>(2.f, 0.f,
                                                                              [this](float progress)
    {
        auto z_val = m_fg_mesh->position().z;
        glm::vec3 pos = kinski::mix(glm::vec3(0.f, -2.f, z_val), glm::vec3(0.f, 0.f, z_val), progress);
        m_fg_mesh->set_position(pos);
    });
    
    m_animations[ANIM_FOREGROUND_OUT] = std::make_shared<animation::Animation>(2.f, 0.f,
                                                                               [this](float progress)
    {
        auto z_val = m_fg_mesh->position().z;
        glm::vec3 pos = kinski::mix(glm::vec3(0.f, 0.f, z_val), glm::vec3(0.f, -2.f, z_val), progress);
        m_fg_mesh->set_position(pos);
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
}
