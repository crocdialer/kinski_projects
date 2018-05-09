//
//  FractureApp.cpp
//  gl
//
//  Created by Fabian on 01/02/15.
//
//

#include <gl/DeferredRenderer.hpp>
#include "core/Timer.hpp"
#include "FractureApp.h"
#include "assimp/assimp.hpp"
#include "gl/ShaderLibrary.h"

using namespace std;
using namespace kinski;
using namespace glm;

/////////////////////////////////////////////////////////////////

void FractureApp::setup()
{
    ViewerApp::setup();
    
    m_texture_paths->set_tweakable(false);
    
    register_property(m_view_type);
    register_property(m_model_path);
    register_property(m_crosshair_path);
    register_property(m_texture_paths);
    register_property(m_physics_running);
    register_property(m_physics_debug_draw);
    register_property(m_num_fracture_shards);
    register_property(m_breaking_thresh);
    register_property(m_gravity);
    register_property(m_friction);
    register_property(m_shoot_velocity);
    register_property(m_shots_per_sec);
    register_property(m_obj_scale);
    register_property(m_fbo_resolution);
    register_property(m_fbo_cam_pos);
    register_property(m_use_syphon);
    register_property(m_syphon_server_name);
    observe_properties();
    
    // init physics
    m_physics.init();
    
    // box shooting stuff
    float sphere_radius = .5f;
    m_box_shape = std::make_shared<btBoxShape>(btVector3(.5f, .5f, .5f));
    m_box_geom = gl::Geometry::create_box(vec3(.5f));
    m_sphere_shape = std::make_shared<btSphereShape>(sphere_radius);
    m_shoot_mesh = gl::Mesh::create(gl::Geometry::create_sphere(sphere_radius, 24),
                                    gl::Material::create(gl::ShaderType::PHONG));
//    m_shoot_mesh->material()->queue_texture_load("~/Downloads/tennisball.jpg");
    
    m_gui_cam = gl::OrthoCamera::create(0, gl::window_dimension().x, gl::window_dimension().y,
                                               0, 0, 1);
    
    // init joystick crosshairs
    m_crosshair_pos.resize(get_joystick_states().size());
    for(auto &p : m_crosshair_pos){ p = gl::window_dimension() / 2.f; }

    scene()->set_renderer(gl::DeferredRenderer::create());

    load_settings();
}

/////////////////////////////////////////////////////////////////

void FractureApp::update(float timeDelta)
{
    ViewerApp::update(timeDelta);

    // construct ImGui window for this frame
    if(display_tweakbar())
    {
        gui::draw_component_ui(shared_from_this());
        gui::draw_component_ui(m_light_component);
    }

    m_time_since_last_shot += timeDelta;
    
    if(m_needs_refracture){ fracture_test(*m_num_fracture_shards); }
    if(*m_physics_running){ m_physics.step_simulation(timeDelta); }
    
    handle_joystick_input(timeDelta);
    
    // movie updates
    if(m_movie && m_movie->copy_frame_to_texture(textures()[TEXTURE_INNER], true)){}
    
    if(m_crosshair_movie && m_crosshair_movie->copy_frame_to_texture(m_crosshair_tex, true)){}
}

/////////////////////////////////////////////////////////////////

void FractureApp::draw()
{
    gl::clear();

    // draw the output texture
    if(m_fbos[0] && m_fbo_cam && *m_use_syphon)
    {
        auto tex = gl::render_to_texture(m_fbos[0],[&]
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            if(*m_physics_debug_draw){ m_physics.debug_render(m_fbo_cam); }
            else{ scene()->render(m_fbo_cam); }
            
            // gui stuff
            gl::set_matrices(m_gui_cam);
            int crosshair_width = 70;
            
            for(auto &p : m_crosshair_pos)
            {
                if(m_crosshair_tex)
                {
                    gl::draw_texture(m_crosshair_tex, vec2(crosshair_width), p - vec2(crosshair_width) / 2.f);
                }
                else{ gl::draw_circle(p, 15.f, gl::COLOR_WHITE, false); }
            }
            
        });
        textures()[TEXTURE_SYPHON] = tex;
        m_syphon.publish_texture(tex);
    }
    
    switch (*m_view_type)
    {
        case VIEW_DEBUG:
            gl::set_matrices(camera());
            if(*m_physics_debug_draw){ m_physics.debug_render(camera()); }
            else{ scene()->render(camera()); gl::reset_state(); }
            if(draw_grid()){ gl::draw_grid(50, 50); }
            m_light_component->draw_light_dummies();
            break;
            
        case VIEW_OUTPUT:
            gl::draw_texture(textures()[TEXTURE_SYPHON], gl::window_dimension());
            break;
            
        default:
            break;
    }
    
    // draw texture map(s)
    if(display_tweakbar()){ draw_textures(textures()); }
}

/////////////////////////////////////////////////////////////////

void FractureApp::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
    m_gui_cam = gl::OrthoCamera::create(0, w, h, 0, 0, 1);
}

/////////////////////////////////////////////////////////////////

void FractureApp::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);
    
    if(!display_tweakbar())
    {
        switch (e.getCode())
        {
            case GLFW_KEY_V:
                fracture_test(*m_num_fracture_shards);
                break;
            case GLFW_KEY_P:
                *m_physics_running = !*m_physics_running;
                break;
                
            default:
                break;
        }
    }
}

/////////////////////////////////////////////////////////////////

void FractureApp::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void FractureApp::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);
    
    if(e.isRight())
    {
        gl::CameraPtr cam = *m_view_type == VIEW_OUTPUT ? m_fbo_cam : camera();
        auto ray = gl::calculate_ray(cam, vec2(e.getX(), e.getY()));
//        shoot_box(ray, *m_shoot_velocity);
        shoot_ball(ray, *m_shoot_velocity, .8f);
    }
}

/////////////////////////////////////////////////////////////////

void FractureApp::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);
}

/////////////////////////////////////////////////////////////////

void FractureApp::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void FractureApp::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void FractureApp::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
}

/////////////////////////////////////////////////////////////////

void FractureApp::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    m_texture_paths->value().clear();
    
    for(const string &f : files)
    {
        LOG_INFO << f;

        // add path to searchpaths
        fs::add_search_path(fs::get_directory_part(f));
        m_search_paths->value().push_back(f);
        
        switch (fs::get_file_type(f))
        {
            case fs::FileType::MODEL:
                *m_model_path = f;
                break;
            
            case fs::FileType::IMAGE:
            case fs::FileType::MOVIE:
                m_texture_paths->value().push_back(f);
                break;
                
            default:
                break;
        }
    }
    m_texture_paths->notify_observers();
}

/////////////////////////////////////////////////////////////////

void FractureApp::teardown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void FractureApp::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
    
    if(theProperty == m_model_path)
    {
        fs::add_search_path(fs::get_directory_part(*m_model_path));
        gl::MeshPtr m = assimp::load_model(*m_model_path);
        
        if(m)
        {
//            for(auto &t : m->material()->textures()){ textures().push_back(t); }
            
            scene()->remove_object(m_mesh);
            m_physics.remove_mesh_from_simulation(m_mesh);
            m_mesh = m;
            
            auto aabb = m->aabb();
            float scale_factor = 10.f / aabb.width();
            m->set_scale(scale_factor);
            
            scene()->add_object(m_mesh);
            m_physics.add_mesh_to_simulation(m_mesh);
        }
    }
    else if(theProperty == m_crosshair_path)
    {
        auto ft = fs::get_file_type(*m_crosshair_path);
        if(ft == fs::FileType::MOVIE)
        {
            m_crosshair_movie = media::MediaController::create(*m_crosshair_path, true, true);
        }
        else if(ft == fs::FileType::IMAGE)
        {
            m_crosshair_movie.reset();
            
            try
            {
                m_crosshair_tex  = gl::create_texture_from_file(*m_crosshair_path);
            } catch (fs::FileNotFoundException &e) { LOG_WARNING << e.what(); }
        }
    }
    else if(theProperty == m_texture_paths)
    {
        std::vector<gl::Texture> tex_array;
        
        for(const string &f : m_texture_paths->value())
        {
            if(fs::get_file_type(f) == fs::FileType::IMAGE)
            {
                try{ tex_array.push_back(gl::create_texture_from_file(f, true, true, 16.f)); }
                catch (Exception &e) { LOG_WARNING << e.what(); }
            }else if(fs::get_file_type(f) == fs::FileType::MOVIE)
            {
                m_movie = media::MediaController::create(f, false, true);
                m_movie->set_on_load_callback([this, tex_array](media::MediaControllerPtr m)
                {
                    m_needs_refracture = true;
                    m->set_volume(0.f);
                    m->play();
                });
            }
        }
        if(tex_array.size() > 0){ textures()[TEXTURE_OUTER] = tex_array[0]; }
        if(tex_array.size() > 1){ textures()[TEXTURE_INNER] = tex_array[1]; }
    }
    else if(theProperty == m_fbo_resolution)
    {
        gl::Fbo::Format fmt;
        fmt.num_samples = 8;
        m_fbos[0] = gl::Fbo::create(m_fbo_resolution->value().x, m_fbo_resolution->value().y, fmt);
        float aspect = m_fbos[0]->aspect_ratio();//m_obj_scale->value().x / m_obj_scale->value().y;
        m_fbo_cam = gl::PerspectiveCamera::create(aspect, 45.f, .1f, 100.f);
        m_fbo_cam->position() = *m_fbo_cam_pos;
    }
    else if(theProperty == m_fbo_cam_pos)
    {
        m_fbo_cam->position() = *m_fbo_cam_pos;
    }
    else if(theProperty == m_use_syphon)
    {
        m_syphon = *m_use_syphon ? syphon::Output(*m_syphon_server_name) : syphon::Output();
    }
    else if(theProperty == m_syphon_server_name)
    {
        try{m_syphon.setName(*m_syphon_server_name);}
        catch(syphon::SyphonNotRunningException &e){LOG_WARNING<<e.what();}
    }
    else if(theProperty == m_gravity)
    {
        if(m_physics.dynamicsWorld())
        {
            m_physics.dynamicsWorld()->setGravity(btVector3(0 , -1.f, 0) * *m_gravity);
        }
    }
    else if(theProperty == m_friction)
    {
        if(m_physics.dynamicsWorld())
        {
            for(int i = m_physics.dynamicsWorld()->getNumCollisionObjects() - 1; i >= 0; i--)
            {
                btRigidBody* b = btRigidBody::upcast(m_physics.dynamicsWorld()->getCollisionObjectArray()[i]);
                
                if(b){ b->setFriction(*m_friction); }
            }
        }
    }
    else if(theProperty == m_num_fracture_shards)
    {
        m_needs_refracture = true;
    }
}

/////////////////////////////////////////////////////////////////

void FractureApp::shoot_box(const gl::Ray &the_ray, float the_velocity,
                            const glm::vec3 &the_half_extents)
{
    static gl::ShaderPtr phong_shader;
    if(!phong_shader){ phong_shader = gl::create_shader(gl::ShaderType::PHONG); }
    gl::MeshPtr mesh = gl::Mesh::create(m_box_geom, gl::Material::create(phong_shader));
    mesh->set_scale(.2f * the_half_extents);
    mesh->set_position(the_ray.origin);
    scene()->add_object(mesh);
    m_box_shape->setLocalScaling(physics::type_cast(mesh->scale()));
    
    
    btRigidBody *rb = m_physics.add_mesh_to_simulation(mesh, pow(2 * the_half_extents.x, 3.f),
                                                       m_box_shape);
    rb->setFriction(*m_friction);
    rb->setLinearVelocity(physics::type_cast(the_ray.direction * the_velocity));
    rb->setCcdSweptSphereRadius(glm::length(mesh->scale() / 2.f));
    rb->setCcdMotionThreshold(glm::length(mesh->scale() / 2.f));
}

/////////////////////////////////////////////////////////////////

void FractureApp::shoot_ball(const gl::Ray &the_ray, float the_velocity,
                             float the_radius)
{
    gl::MeshPtr mesh = m_shoot_mesh->copy();
    mesh->set_scale(2 * the_radius);
    mesh->set_position(the_ray.origin);
    scene()->add_object(mesh);
    m_sphere_shape->setLocalScaling(physics::type_cast(mesh->scale()));
    
    
    btRigidBody *rb = m_physics.add_mesh_to_simulation(mesh,
                                                       100 * glm::pi<float>() * 4.f / 3.f * pow(the_radius, 3.f),
                                                       m_sphere_shape);
    rb->setFriction(*m_friction);
    rb->setLinearVelocity(physics::type_cast(the_ray.direction * the_velocity));
    rb->setCcdSweptSphereRadius(glm::length(mesh->scale() / 2.f));
    rb->setCcdMotionThreshold(glm::length(mesh->scale() / 2.f));
}

/////////////////////////////////////////////////////////////////

void FractureApp::fracture_test(uint32_t num_shards)
{
    scene()->clear();
    m_physics.init();
    m_gravity->notify_observers();
    
    auto phong = gl::create_shader(gl::ShaderType::PHONG);
    auto phong_shadow = gl::create_shader(gl::ShaderType::PHONG_SHADOWS);
    
    m_physics.set_world_boundaries(vec3(100), vec3(0));
    
//    btRigidBody *wall;
    {
        // ground plane
        auto ground_mat = gl::Material::create(phong_shadow);
        auto ground = gl::Mesh::create(gl::Geometry::create_box(vec3(.5f)), ground_mat);
        for(auto &c : ground->geometry()->colors()){ c = gl::COLOR_WHITE; }
        
        ground->set_scale(vec3(100, 1, 100));
        auto ground_aabb = ground->aabb();
        ground->position().y -= ground_aabb.halfExtents().y;
        auto col_shape = std::make_shared<btBoxShape>(physics::type_cast(ground_aabb.halfExtents()));
        btRigidBody *rb = m_physics.add_mesh_to_simulation(ground, 0.f, col_shape);
        rb->setFriction(*m_friction);
        scene()->add_object(ground);
        
        // back plane
        auto back = gl::Mesh::create(gl::Geometry::create_box(vec3(.5f)), ground_mat);
        back->set_scale(vec3(100, 20, .3));
        auto back_aabb = back->aabb();
        back->position() += vec3(0, back_aabb.halfExtents().y, -2.f * back_aabb.halfExtents().z);
        col_shape = std::make_shared<btBoxShape>(physics::type_cast(back_aabb.halfExtents()));
        
        // stopper
        auto stopper = gl::Mesh::create(gl::Geometry::create_box(vec3(.5f)), ground_mat);
        stopper->set_scale(vec3(m_obj_scale->value().x, .1f, .1f));
        auto stopper_aabb = back->aabb();
        stopper->position() = vec3(0, 0,  - m_obj_scale->value().z / 2.f);
        col_shape = std::make_shared<btBoxShape>(physics::type_cast(stopper_aabb.halfExtents()));
        m_physics.add_mesh_to_simulation(stopper);
        
//        wall = rb = m_physics.add_mesh_to_simulation(back, 0.f, col_shape);
//        rb->setFriction(*m_friction);
//        scene().addObject(back);
        
        // wall left/right
//        m_physics.set_world_boundaries(vec3(m_obj_scale->value().x / 2.f, 100, 1000));
    }
    
    
    for(auto *b : m_physics.bounding_bodies()){ b->setFriction(*m_friction); }
    
    if(m_mesh)
    {
        scene()->add_object(m_mesh);
        m_physics.add_mesh_to_simulation(m_mesh);
    }
    
    for(auto &l : lights()){ scene()->add_object(l); }
    
    auto m = gl::Mesh::create(gl::Geometry::create_box(vec3(.5f)), gl::Material::create(phong_shadow));
    m->set_scale(*m_obj_scale);
    auto aabb = m->aabb();
    m->position().y += aabb.halfExtents().y;
    
//    std::vector<gl::Texture> textures;
//    for(const auto &tex_path : m_texture_paths->value())
//    {
//        try
//        {
//            textures.push_back(gl::createTextureFromFile(tex_path, true, true, 8.f));
//        }catch(Exception &e){ LOG_WARNING << e.what(); }
//    }
//    m->material()->add_texture(tex);
    
    // voronoi points
    std::vector<glm::vec3> voronoi_points;
    voronoi_points.resize(num_shards);
    for(auto &vp : voronoi_points)
    {
        vp = m->position() + glm::linearRand(-aabb.halfExtents(), aabb.halfExtents());
    }//(aabb.min, aabb.max); }
    
    Stopwatch t;
    t.start();
    
    if(m_needs_refracture || m_voronoi_shards.empty())
    {
        m_voronoi_shards = physics::voronoi_convex_hull_shatter(m, voronoi_points);
        m_needs_refracture = false;
    }
    
    // original object without fracturing
//    m->position() += vec3(5, 0, 0);
//    scene()->add_object(m);
//    m_physics.add_mesh_to_simulation(m);
    
    float density = 1.8;
    float convex_margin = 0.f;
    
    gl::MaterialPtr inner_mat(gl::Material::create()),
    outer_mat(gl::Material::create(phong_shadow));
    
    inner_mat->set_diffuse(gl::COLOR_BLACK);
    inner_mat->set_emission(gl::COLOR_WHITE);
    
    outer_mat->add_texture(textures()[TEXTURE_OUTER]);
    inner_mat->add_texture(textures()[TEXTURE_INNER]);
    
    for(auto &s : m_voronoi_shards)
    {
        auto mesh_copy = s.mesh->copy();
        scene()->add_object(mesh_copy);
        mesh_copy->materials() = {outer_mat, inner_mat};
        
        
        auto col_shape = physics::create_convex_collision_shape(mesh_copy);
        btRigidBody* rb = m_physics.add_mesh_to_simulation(mesh_copy, density * s.volume, col_shape);

        rb->getCollisionShape()->setMargin(convex_margin);
        rb->setRestitution(0.5f);
        rb->setFriction(*m_friction);
        rb->setRollingFriction(*m_friction);
        
        //ccd
        auto aabb = mesh_copy->aabb();
        rb->setCcdSweptSphereRadius(glm::length(aabb.halfExtents()) / 2.f);
        rb->setCcdMotionThreshold(glm::length(aabb.halfExtents()) / 2.f);
        
        // pin to wall
//        btTransform trA = rb->getWorldTransform(), trB = rb->getWorldTransform();
//        btVector3 pin_point = trA.getOrigin();
//        pin_point[2] = 0.f;//trB.getOrigin().z();
//        trB.setOrigin(pin_point);
//        btFixedConstraint* fixed = new btFixedConstraint(*rb, *wall, trA, trB);
//        fixed->setBreakingImpulseThreshold(*m_breaking_thresh);
//        fixed ->setOverrideNumSolverIterations(30);
//        m_physics.dynamicsWorld()->addConstraint(fixed,true);
        
    }
    m_physics.dynamicsWorld()->performDiscreteCollisionDetection();
    m_physics.attach_constraints(*m_breaking_thresh);
    
    LOG_DEBUG << "fracturing took " << t.time_elapsed() << " secs";
}

void FractureApp::handle_joystick_input(float the_time_delta)
{
    // update joystick positions
    auto joystick_states = get_joystick_states();
    
    int i = 0;
    
    for(auto &joystick : joystick_states)
    {
        float min_val = .38f, multiplier = 400.f;
        float x_axis = abs(joystick.axis()[0]) > min_val ? joystick.axis()[0] : 0.f;
        float y_axis = abs(joystick.axis()[1]) > min_val ? joystick.axis()[1] : 0.f;
        m_crosshair_pos[i] += vec2(x_axis, y_axis) * multiplier * the_time_delta;
        
        if(m_fbos[0])
            m_crosshair_pos[i] = glm::clamp(m_crosshair_pos[i], vec2(0), vec2(m_fbos[0]->size()));
        
        if(joystick.buttons()[11] && m_fbo_cam && m_time_since_last_shot > 1.f / *m_shots_per_sec)
        {
            auto ray = gl::calculate_ray(m_fbo_cam, m_crosshair_pos[i], m_fbos[0]->size());
            shoot_box(ray, *m_shoot_velocity);
            m_time_since_last_shot = 0.f;
        }
        
        if(joystick.buttons()[8]){ *m_physics_debug_draw = !*m_physics_debug_draw; }
        if(joystick.buttons()[9]){ fracture_test(*m_num_fracture_shards); break; }
        i++;
    }
}
