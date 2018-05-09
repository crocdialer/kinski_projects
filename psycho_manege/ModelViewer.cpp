//
//  ModelViewer.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "ModelViewer.h"

#include "gl/ShaderLibrary.h"
#include "gl/Visitor.hpp"

// internal module includes
#include "assimp/assimp.hpp"

using namespace std;
using namespace kinski;
using namespace glm;

#define NUM_LVLS 3

void s_midi_callback(double timeStamp, std::vector<unsigned char> *message, void *userData)
{
    ModelViewer* app = static_cast<ModelViewer*>(userData);
    
    // schedule on main queue
    app->main_queue().submit(std::bind(&ModelViewer::midi_callback, app, timeStamp, *message));
}

/////////////////////////////////////////////////////////////////

void ModelViewer::setup()
{
    ViewerApp::setup();
    
    // offscreen props
    register_property(m_use_syphon);
    register_property(m_use_test_pattern);
    register_property(m_syphon_server_name);
    register_property(m_num_screens);
    register_property(m_offscreen_size);
    register_property(m_offscreen_fov);
    register_property(m_use_offscreen_manual_angle);
    register_property(m_offscreen_manual_angle);
    register_property(m_offscreen_offset);
    
    // models props
    register_property(m_model_path);
    register_property(m_use_bones);
    register_property(m_display_bones);
    register_property(m_animation_index);
    register_property(m_animation_speed);
    register_property(m_obj_wire_frame);
    register_property(m_obj_texturing);
    register_property(m_obj_audio_react_low);
    register_property(m_obj_audio_react_hi);
    register_property(m_obj_scale);
    register_property(m_obj_audio_auto_rotate);
    
    // scene props
    register_property(m_rotation_lvl_1);
    register_property(m_rotation_lvl_2);
    register_property(m_rotation_lvl_3);
    register_property(m_distance_lvl_1);
    register_property(m_distance_lvl_2);
    register_property(m_distance_lvl_3);
    register_property(m_num_objects_lvl_1);
    register_property(m_num_objects_lvl_2);
    register_property(m_num_objects_lvl_3);
    register_property(m_displace_factor);
    register_property(m_displace_res);
    register_property(m_displace_velocity);
    register_property(m_asset_paths);
    
    // audio / midi props
    register_property(m_midi_port_nr);
    register_property(m_freq_low);
    register_property(m_freq_mid_low);
    register_property(m_freq_mid_high);
    register_property(m_freq_high);
    
    m_asset_paths->set_tweakable(false);
    register_property(m_cube_map_folder);
    observe_properties();

//    m_light_component->set_lights(lights());
    
    // add lights to scene
    for (auto l : lights()){ scene()->add_object(l ); }
    
    // roots for our scene
    m_roots.resize(NUM_LVLS);
    m_proto_meshes.resize(NUM_LVLS);
    
    for(auto &r : m_roots)
    {
        r = gl::Object3D::create();
        scene()->add_object(r);
    }
    
    // pre-create our shaders
    m_shaders[SHADER_UNLIT] = gl::create_shader(gl::ShaderType::UNLIT);
    m_shaders[SHADER_UNLIT_SKIN] = gl::create_shader(gl::ShaderType::UNLIT_SKIN);
    m_shaders[SHADER_PHONG] = gl::create_shader(gl::ShaderType::PHONG);
    m_shaders[SHADER_PHONG_SKIN] = gl::create_shader(gl::ShaderType::PHONG_SKIN);
    m_shaders[SHADER_UNLIT_DISPLACE] = gl::create_shader_from_file("unlit_displace.vert", "unlit.frag");
    m_shaders[SHADER_UNLIT_SKIN_DISPLACE] = gl::create_shader_from_file("unlit_skin_displace.vert", "unlit.frag");
    
    // init select indicator mesh
    m_select_indicator = gl::Mesh::create(gl::Geometry::create_cone(10.f, 30.f, 4),
                                          gl::Material::create());
    m_select_indicator->transform() = rotate(mat4(), glm::pi<float>(), gl::X_AXIS);
    m_select_indicator->material()->set_diffuse(gl::COLOR_PURPLE);
    m_select_indicator->material()->set_wireframe();
    
    // init timers
    m_timer_select_enable = Timer(main_queue().io_service());
    
    // init midi
    size_t num_midi_ports = m_midi_in->getPortCount();
    
    for (size_t i = 0; i < num_midi_ports; i++)
    {
        LOG_INFO << "midi port: " << m_midi_in->getPortName(i);
    }

    // init audio recording stuff
    init_audio();
    
    load_settings();
    m_light_component->refresh();
}

/////////////////////////////////////////////////////////////////

void ModelViewer::update(float timeDelta)
{
    ViewerApp::update(timeDelta);

    // construct ImGui window for this frame
    if(display_tweakbar())
    {
        gui::draw_component_ui(shared_from_this());
    }

    // everything kosher with our cams?
    if(m_needs_cam_refresh)
    {
        m_needs_cam_refresh = false;
        setup_offscreen_cameras(*m_num_screens, !*m_use_offscreen_manual_angle);
    }

    // fetch all meshes in scenegraph
    gl::SelectVisitor<gl::Mesh> mv;
    scene()->root()->accept(mv);

    //////////////////////////////// Parallax Lvls /////////////////////////////////////////

    float p_rot[NUM_LVLS] = {*m_rotation_lvl_1, *m_rotation_lvl_2, *m_rotation_lvl_3};

    for(size_t i = 0; i < m_roots.size(); i++)
    {
        m_roots[i]->transform() = rotate(m_roots[i]->transform(), glm::radians(p_rot[i] * timeDelta),
                                         gl::Y_AXIS);
    }

    //////////////////////////////// Joystick //////////////////////////////////////////////

    process_joystick_input(timeDelta);

    //////////////////////////////// Midi / Audio //////////////////////////////////////////

    update_audio();

    float audio_low = std::max(m_last_volumes[0], m_last_volumes[1]) * *m_obj_audio_react_low;
    float audio_hi = std::max(m_last_volumes[2], m_last_volumes[3]) * *m_obj_audio_react_hi;

    if(audio_low > 0.f)
    {
        float max = m_displace_factor->range().second;

        // LOWS
        *m_displace_factor = audio_low * max;

    }

    if(audio_hi > 0.f)
    {
        // HIGHS
        float max = m_obj_scale->range().second;

        // LOWS
        *m_displace_res = audio_hi * max;
    }

    //////////////////////////////// Displacement //////////////////////////////////////////

    // update displacement map
    textures()[TEXTURE_NOISE] = m_noise.simplex(get_application_time() * *m_displace_velocity);
//    gl::reset_state();

    // update displacement textures in models
    for(gl::Mesh *m : mv.get_objects())
    {
        for(auto &mat : m->materials())
        {
            gl::Texture diffuse_tex;

            if(mat->textures().size() > 1)
            {
                diffuse_tex = mat->textures()[0];
            }
            mat->clear_textures();

            if(diffuse_tex){ mat->add_texture(diffuse_tex); }
            mat->add_texture(textures()[TEXTURE_NOISE]);
        }
    }
    
//    mv.clear();
//    m_debug_scene.root()->accept(mv);
//    
//    std::vector<gl::Mesh*> debug_meshes(mv.getObjects().begin(), mv.getObjects().end());
//    LOG_DEBUG << "total objects in debug scene: " << debug_meshes.size();
//    LOG_DEBUG << "visible objects in debug scene: " << m_debug_scene.num_visible_objects();
}

/////////////////////////////////////////////////////////////////

void ModelViewer::draw()
{
    gl::clear();
    gl::set_matrices(camera());
    if(draw_grid()){ gl::draw_grid(50, 50); }
    
    m_light_component->draw_light_dummies();

    // render scene and debug elements
    scene()->render(camera());

    m_debug_scene->render(camera());

    if(*m_use_syphon || *m_use_warping)
    {
        auto tex = textures()[TEXTURE_OUTPUT] = create_offscreen_texture();
    }
    if(*m_use_syphon){ m_syphon_out.publish_texture(textures()[TEXTURE_OUTPUT]); }
    if(*m_use_warping)
    {
        for(uint32_t i = 0; i < m_warp_component->num_warps(); i++)
        {
            if(m_warp_component->enabled(i))
            {
                m_warp_component->render_output(i, textures()[TEXTURE_OUTPUT]);
            }
        }
    }
    
    // draw texture map(s)
    if(display_tweakbar())
    {
        if(selected_mesh() && !selected_mesh()->material()->textures().empty())
        {
            textures()[TEXTURE_SELECTED] = selected_mesh()->material()->textures()[0];
        }
        else{ textures()[TEXTURE_SELECTED].reset(); }

        draw_textures(textures());

        // draw fft vals
        for (uint32_t i = 0; i < m_last_volumes.size(); i++)
        {
            float v = m_last_volumes[i];
            gl::draw_quad(vec2(40, gl::window_dimension().y * v),
                          gl::COLOR_ORANGE,
                          vec2(100 + i * 50, gl::window_dimension().y * (1 - v)));
        }
    }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);
    
    switch (e.getCode())
    {
        case GLFW_KEY_DELETE:
            {
                int sel_lvl = cam_index() % m_roots.size();
                m_roots[sel_lvl]->children().clear();
                m_proto_meshes[sel_lvl].clear();
            }
            break;
            
        case GLFW_KEY_G:
        {
            for(size_t i = 0; i < NUM_LVLS; i++)
            {
                reset_lvl(i);
            }
        }
            break;
        case GLFW_KEY_T:
        {
            toggle_selection();
            break;
        }
        default:
            break;
    }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);

    // something might have changed
    update_select_indicator();
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    int lvl = m_roots.size() - e.getY() / (gl::window_dimension().y / m_roots.size());
    LOG_DEBUG << "dropped assets on lvl: " << lvl;
    
    for(const string &f : files){ load_asset(f, lvl); }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::teardown()
{
    LOG_PRINT<<"ciao lethargy poop";
}

/////////////////////////////////////////////////////////////////

void ModelViewer::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
    
    if(theProperty == m_model_path)
    {
        if(!m_model_path->value().empty()){ load_asset(*m_model_path, 0, true); }
    }
    else if(theProperty == m_use_bones)
    {
        if(auto m = selected_mesh())
        {
            bool use_bones = m->geometry()->has_bones() && *m_use_bones;
            for(auto &mat : m->materials())
            {
                mat->set_shader(m_shaders[use_bones ? SHADER_UNLIT_SKIN_DISPLACE : SHADER_UNLIT_DISPLACE]);
            }
        }
    }
    else if(theProperty == m_animation_index)
    {
        if(selected_mesh())
        {
            selected_mesh()->set_animation_index(*m_animation_index);
        }
    }
    else if(theProperty == m_animation_speed)
    {
        if(const auto &m = selected_mesh())
        {
            m->set_animation_speed(*m_animation_speed);
        }
    }
    else if(theProperty == m_displace_factor)
    {
        if(const auto &m = selected_mesh())
        {
            for(auto &mat : m->materials())
            {
                mat->uniform("u_displace_factor", *m_displace_factor / m->scale().x);
            }
        }
    }
    else if(theProperty == m_obj_scale)
    {
        if(selected_mesh() && selected_mesh()->parent())
        {
            auto p = selected_mesh()->parent();
            p->set_scale(*m_obj_scale);
        }
    }
    else if(theProperty == m_obj_audio_auto_rotate)
    {
        if(auto m = selected_mesh())
        {
            if(m == m_select_indicator){ return; }
            
            auto v = *m_obj_audio_auto_rotate * 60.f;
            
            m->set_update_function([this, m, v](float td)
            {
                float val = td * v;
                m->transform() = rotate(m->transform(), glm::radians(val),
                                        m_rotation_axis->value());
            });
        }
    }
    else if(theProperty == m_obj_wire_frame)
    {
        if(selected_mesh())
        {
            for(auto &mat : selected_mesh()->materials()){ mat->set_wireframe(*m_obj_wire_frame); }
        }
    }
    else if(theProperty == m_cube_map_folder)
    {
        async_load_texture(*m_cube_map_folder, [this](const gl::Texture &t)
        {
            m_cube_map = t;
        }, false, true);
    }
    else if(theProperty == m_use_syphon)
    {
        m_syphon_out = *m_use_syphon ? syphon::Output(*m_syphon_server_name) : syphon::Output();
    }
    else if(theProperty == m_syphon_server_name)
    {
        try{m_syphon_out.setName(*m_syphon_server_name);}
        catch(syphon::SyphonNotRunningException &e){LOG_WARNING<<e.what();}
    }
    else if(theProperty == m_offscreen_fov || theProperty == m_offscreen_offset ||
            theProperty == m_num_screens || theProperty == m_offscreen_size ||
            theProperty == m_use_offscreen_manual_angle || theProperty == m_offscreen_manual_angle)
    {
        m_needs_cam_refresh = true;
    }
    else if(theProperty == m_displace_res)
    {
        m_noise.set_scale(vec2(*m_displace_res));
    }
    else if(theProperty == m_asset_paths)
    {
        for(const auto &p : m_asset_paths->value())
        {
            LOG_DEBUG << p;
        }
    }
    else if(theProperty == m_freq_low ||
            theProperty == m_freq_mid_low ||
            theProperty == m_freq_mid_high ||
            theProperty == m_freq_high )
    {
        m_frequency_map =
        {
            {FREQ_LOW, std::pair<float, float>(0.f, *m_freq_low)},
            {FREQ_MID_LOW, std::pair<float, float>(*m_freq_low, *m_freq_mid_low)},
            {FREQ_MID_HIGH, std::pair<float, float>(*m_freq_mid_low, *m_freq_mid_high)},
            {FREQ_HIGH, std::pair<float, float>(*m_freq_mid_high, *m_freq_high)}
        };
    }
    else if(theProperty == m_midi_port_nr)
    {
        try
        {
            m_midi_in->openPort(*m_midi_port_nr);
            m_midi_in->setCallback(&s_midi_callback, this);
            m_midi_in->ignoreTypes(false, false, false);
            LOG_DEBUG << "opened: " << *m_midi_port_nr;
        }catch(std::exception &e)
        {
            LOG_WARNING << e.what();
            LOG_WARNING << m_midi_in->getPortCount() << " midi port(s) available";
        }

    }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::build_skeleton(gl::BonePtr currentBone, vector<vec3> &points,
                                 vector<string> &bone_names)
{
    if(!currentBone) return;
    
    // read out the bone names
    bone_names.push_back(currentBone->name);
    
    for (auto child_bone : currentBone->children)
    {
        mat4 globalTransform = currentBone->worldtransform;
        mat4 childGlobalTransform = child_bone->worldtransform;
        points.push_back(globalTransform[3].xyz());
        points.push_back(childGlobalTransform[3].xyz());
        
        build_skeleton(child_bone, points, bone_names);
    }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::setup_offscreen_cameras(int num_screens, bool as_circle)
{
    // remove old objects
    for(auto &m : m_offscreen_meshes){ m_debug_scene->remove_object(m); }
    
    m_offscreen_cams.resize(num_screens);
    m_offscreen_meshes.resize(num_screens);
    
    float rot_angle_deg = as_circle? (-360.f / num_screens) : -*m_offscreen_manual_angle;
    float aspect = m_offscreen_size->value().x / m_offscreen_size->value().y;
    
    for(size_t i = 0; i < m_offscreen_cams.size(); i++)
    {
        m_offscreen_cams[i] = gl::PerspectiveCamera::create(aspect, *m_offscreen_fov, 5.f, 1000.f);
        m_offscreen_cams[i]->set_transform(rotate(mat4(), glm::radians(rot_angle_deg) * i, gl::Y_AXIS));
        m_offscreen_cams[i]->set_position(*m_offscreen_offset);
        
        // create debug mesh
        m_offscreen_meshes[i] = gl::create_frustum_mesh(m_offscreen_cams[i]);
        m_offscreen_meshes[i]->material()->set_diffuse(m_cam_colors[i]);
        m_debug_scene->add_object(m_offscreen_meshes[i]);
    }
    
    // create our FBO
    try
    {
        m_offscreen_fbo = gl::Fbo::create(m_offscreen_size->value().x * *m_num_screens, m_offscreen_size->value().y);
    }catch(Exception &e){ LOG_ERROR << e.what(); }
}

/////////////////////////////////////////////////////////////////

gl::Texture ModelViewer::create_offscreen_texture()
{
    if(!m_offscreen_fbo)
    {
        LOG_WARNING << "no fbo defined";
        return textures()[TEXTURE_OUTPUT];
    }
    
    float screen_width = m_offscreen_size->value().x, screen_height = m_offscreen_size->value().y;
    
    textures()[TEXTURE_OUTPUT] = gl::render_to_texture(m_offscreen_fbo,
                                                      [this, screen_width, screen_height]
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDepthMask(GL_TRUE);
        
        // draw each offscreen camera
        for(size_t i = 0; i < m_offscreen_cams.size(); i++)
        {
            // set viewport here
            gl::set_window_dimension(vec2(screen_width, screen_height), vec2(i * screen_width, 0));
            
            if(*m_use_test_pattern){ gl::draw_quad(gl::window_dimension(), m_cam_colors[i]); }
            else{ scene()->render(m_offscreen_cams[i]); }
        }
        
    });
    return textures()[TEXTURE_OUTPUT];
}

/////////////////////////////////////////////////////////////////

bool ModelViewer::load_asset(const std::string &the_path, uint32_t the_lvl, bool add_to_scene)
{
    gl::MeshPtr m;
    gl::Texture t;
    
    auto asset_dir = fs::get_directory_part(the_path);
    fs::add_search_path(asset_dir);
    
    switch (fs::get_file_type(the_path))
    {
        case fs::FileType::DIRECTORY:
            for(const auto &p : fs::get_directory_entries(the_path)){ load_asset(p, the_lvl); }
            break;
            
        case fs::FileType::MODEL:
            m = assimp::load_model(the_path);
            break;
            
        case fs::FileType::IMAGE:
            
            try { t = gl::create_texture_from_file(the_path, true, true); }
            catch (Exception &e) { LOG_WARNING << e.what(); }
            
            if(t)
            {
                auto geom = gl::Geometry::create_plane(t.width(), t.height(), 100, 100);
                auto mat = gl::Material::create(m_shaders[SHADER_UNLIT_DISPLACE]);
                mat->add_texture(t);
                mat->set_depth_write(false);
                m = gl::Mesh::create(geom, mat);
                m->transform() = rotate(mat4(), glm::half_pi<float>(), gl::Y_AXIS);
            }
            break;
            
        default:
            break;
    }
    
    if(m)
    {
        bool use_bones = m->geometry()->has_bones() && *m_use_bones;
        
        for(auto &mat : m->materials())
        {
            mat->set_shader(m_shaders[use_bones ? SHADER_UNLIT_SKIN_DISPLACE : SHADER_UNLIT_DISPLACE]);
            mat->set_blending();
//            mat->set_two_sided();
//            mat->set_wireframe();
            mat->set_diffuse(gl::COLOR_WHITE);
            mat->add_texture(textures()[TEXTURE_NOISE]);
            mat->uniform("u_displace_factor", 0.f);
        }
        
        // apply scaling
        auto aabb = m->aabb();
        float scale_factor = random(1.f, 3.f) * 25.f / length(aabb.halfExtents());
        m->set_scale(scale_factor);
        
        // look for animations for this mesh
        auto animation_folder = fs::join_paths(asset_dir, "animations");
        
        for(const auto &f : get_directory_entries(animation_folder, fs::FileType::MODEL))
        {
            assimp::add_animations_to_mesh(f, m);
        }
        
        // insert into proto array
        m_proto_meshes[the_lvl].push_back(m);
        
        // insert into scene root
        if(add_to_scene){ m_roots[the_lvl]->add_child(m); }
        
        LOG_DEBUG << "lvl " << the_lvl << " now has " << m_proto_meshes[the_lvl].size() << " protos";
        return true;
    }
    
    return false;
}

void ModelViewer::update_select_indicator()
{
    scene()->remove_object(m_select_indicator);
    
    if(auto m = selected_mesh())
    {
        auto aabb = m->aabb();
        aabb = aabb.transform(m->global_transform());
        float h = aabb.height() + m_select_indicator->aabb().height();
        
        gl::Object3DPtr mesh_root = m->parent();
        mesh_root = mesh_root ? mesh_root->parent() : mesh_root;
        
        if(mesh_root)
        {
            m_select_indicator->position() = m->parent()->position() + vec3(0.f, h, 0.f);
            mesh_root->add_child(m_select_indicator);
        }
        
        // update properties !?
        auto &anim_idx = *m_animation_index;
        anim_idx = m->animation_index();
        auto &anim_speed = *m_animation_speed;
        anim_speed = m->animation_speed();
        
        auto &obj_wireframe = *m_wireframe;
        obj_wireframe = m->material()->wireframe();
        auto &obj_scale = *m_obj_scale;
        
        if(m->parent()){ obj_scale = m->parent()->scale().x; }
    }
}

void ModelViewer::reset_lvl(size_t the_lvl)
{
    m_roots[the_lvl]->children().clear();
    
    uint32_t num_objects[] = {*m_num_objects_lvl_1, *m_num_objects_lvl_2, *m_num_objects_lvl_3};
    
    uint32_t num_protos = m_proto_meshes[the_lvl].size();
    
    // move down the x-axis
    // random position
    float lvl_distance[NUM_LVLS] = {*m_distance_lvl_1, *m_distance_lvl_2, *m_distance_lvl_3};
    float dist_variance = 20.f, height_variance = 25.f;
    
    for(size_t i = 0; num_protos && i < num_objects[the_lvl]; i++)
    {
        gl::MeshPtr m = m_proto_meshes[the_lvl][i % num_protos]->copy();
        
        // copy Materials
        std::vector<gl::MaterialPtr> materials = m->materials();
        
        for(size_t j = 0; j < materials.size(); j++)
        {
            m->materials()[j] = gl::Material::create();
            *m->materials()[j] = *materials[j];
            m->materials()[j]->set_two_sided(*m_obj_wire_frame);
            m->materials()[j]->set_wireframe(*m_obj_wire_frame);
            if(!*m_obj_texturing)
            {
                m->materials()[j]->clear_textures();
                m->materials()[j]->queued_textures().clear();
            }
        }
        
        // use standard (white) vertex color
        if(!*m_obj_texturing){ m->geometry()->colors().clear(); }

        // center our mesh
        m->position() -= gl::calculate_centroid(m->geometry()->vertices()) * m->scale();
        
        // create a obj root for rotations and scaling
        auto obj_root = gl::Object3D::create();
        obj_root->add_child(m);
        vec2 circ_rand = glm::circularRand(lvl_distance[the_lvl] + random(-dist_variance, dist_variance));
        vec3 pos = vec3(circ_rand.x, random(0.f, height_variance), circ_rand.y);
        obj_root->position() = pos;
        
        m_roots[the_lvl]->add_child(obj_root);
        
        // random start times for animations
        for (auto &anim : m->animations()){ anim.current_time = random(0.f, anim.duration); }
    }
}

void ModelViewer::toggle_selection(int the_inc)
{
    if(m_timer_select_enable.has_expired())
    {
        gl::SelectVisitor<gl::Mesh> mv;
        scene()->root()->accept(mv);
        auto mesh_list = mv.get_objects();
        mesh_list.remove(m_select_indicator.get());
        
        std::vector<gl::Mesh*> m_vec(mesh_list.begin(), mesh_list.end());
        uint32_t idx = 0;
        
        for(; idx < m_vec.size(); idx++){ if(m_vec[idx] == selected_mesh().get()){ break; } }
        if(!m_vec.empty())
        {
            gl::MeshPtr next_mesh =
            dynamic_pointer_cast<gl::Mesh>(m_vec[(idx + the_inc) % m_vec.size()]->shared_from_this());
            set_selected_mesh(next_mesh);
            update_select_indicator();
        }
    }
}

void ModelViewer::process_joystick_input(float time_delta)
{
    for(auto &js : get_joystick_states())
    {
        if(selected_mesh() && selected_mesh()->parent())
        {
            // rotate
            
            auto p = selected_mesh()->parent();
            
            const float min_val = 0.2f, multiplier = 120.f;
            float x_axis = abs(js.axis()[0]) > min_val ? js.axis()[0] : 0.f;
            float y_axis = abs(js.axis()[1]) > min_val ? js.axis()[1] : 0.f;
            
            p->transform() = rotate(p->transform(), glm::radians(time_delta * multiplier * x_axis),
                                    gl::Y_AXIS);
            p->transform() = rotate(p->transform(), glm::radians(time_delta * multiplier * y_axis),
                                    gl::Z_AXIS);
            
            // scale
            
            if(js.buttons()[JOY_LEFT] || js.buttons()[JOY_RIGHT])
            {
                const float scale_multiplier = 3.f;
                
                float inc = js.buttons()[JOY_LEFT] ? -1.f : 1.f;
                inc *= scale_multiplier * time_delta;
                *m_obj_scale += inc;
            }
        }
        
        
        if(m_timer_select_enable.has_expired())
        {
            bool input = false;
            
            // switch selected mesh
            if(js.buttons()[JOY_CROSS_LEFT] || js.buttons()[JOY_CROSS_RIGHT])
            {
                input = true;
                int inc = js.buttons()[JOY_CROSS_LEFT] ? -1 : 1;
                toggle_selection(inc);
            }
            
            // switch animation
            if(js.buttons()[JOY_CROSS_UP] || js.buttons()[JOY_CROSS_DOWN])
            {
                input = true;
                int inc = js.buttons()[JOY_CROSS_DOWN] ? -1 : 1;
                if(auto m = selected_mesh())
                {
                    m->set_animation_index((m->animation_index() + inc) % m->animations().size());
                }
            }
            
            if(js.buttons()[JOY_X])
            {
                input = true;
                
                if(auto m = selected_mesh())
                {
                    for(auto &mat : m->materials())
                    {
                        mat->set_wireframe(!mat->wireframe());
                    }
                }
            }
            
            if(input){ m_timer_select_enable.expires_from_now(.3f); }
        }
    }
}
