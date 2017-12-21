//
//  AsteroidField.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "AsteroidField.h"

#include "gl/Visitor.hpp"

// module headers
#include "assimp/assimp.hpp"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void AsteroidField::setup()
{
    ViewerApp::setup();
    register_property(m_model_folder);
    register_property(m_texture_folder);
    register_property(m_num_objects);
    register_property(m_sky_box_path);
    register_property(m_half_extents);
    register_property(m_velocity);
    register_property(m_mode);
    observe_properties();
    
    add_tweakbar_for_component(shared_from_this());
    add_tweakbar_for_component(m_light_component);
    
    m_skybox_mesh = gl::Mesh::create(gl::Geometry::create_sphere(1.f, 16), gl::Material::create());
    m_skybox_mesh->material()->set_depth_write(false);
    m_skybox_mesh->material()->set_two_sided();
    
    // finally load state from file
    load_settings();
    
    // and load our assets
    load_assets();
}

/////////////////////////////////////////////////////////////////

void AsteroidField::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    if(m_dirty_flag)
    {
        m_dirty_flag = false;
        create_scene(*m_num_objects);
    }
    
    switch (*m_mode)
    {
        case MODE_NORMAL:
            lights()[0]->set_enabled(true);
            break;
            
        case MODE_LIGHTSPEED:
            lights()[0]->set_enabled(false);
            break;
            
        default:
            break;
    }
    
    // fetch all model-objects in scene
    gl::SelectVisitor<gl::Mesh> mv;
    scene()->root()->accept(mv);
    
    for(auto &m : mv.get_objects())
    {
        // translation update
        m->position() += m_velocity->value() * timeDelta;
        
        // reposition within AABB if necessary
        if(!m_aabb.intersect(m->position()))
        {
            auto &p = m->position();
            
            // find out-of-bounds dimension
            if(p.x < m_aabb.min.x){ p.x += m_aabb.width(); }
            else if(p.x > m_aabb.max.x){ p.x -= m_aabb.width(); }
            if(p.y < m_aabb.min.y){ p.y += m_aabb.height(); }
            else if(p.y > m_aabb.max.y){ p.y -= m_aabb.height(); }
            if(p.z < m_aabb.min.z){ p.z += m_aabb.depth(); }
            else if(p.z > m_aabb.max.z){ p.z -= m_aabb.depth(); }
        }
    }
}

/////////////////////////////////////////////////////////////////

void AsteroidField::draw()
{
    // skybox drawing
    if(*m_mode == MODE_NORMAL)
    {
        gl::set_projection(camera());
        mat4 m = camera()->view_matrix();
        m[3] = vec4(0, 0, 0, 1);
        gl::load_matrix(gl::MODEL_VIEW_MATRIX, m);
        gl::draw_mesh(m_skybox_mesh);
    }
    
    /////////////////////////////////////////////////////////
    
    // draw asteroid field
    gl::set_matrices(camera());
    if(*m_draw_grid){ gl::draw_grid(50, 50); }
    
    scene()->render(camera());
}

/////////////////////////////////////////////////////////////////

void AsteroidField::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void AsteroidField::teardown()
{
    LOG_PRINT<<"ciao asteroid field";
}

/////////////////////////////////////////////////////////////////

void AsteroidField::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
    
    if(theProperty == m_model_folder)
    {
        
    }
    else if(theProperty == m_texture_folder)
    {
        
    }
    else if(theProperty == m_sky_box_path)
    {
        try
        {
            m_skybox_mesh->material()->set_textures({gl::create_texture_from_file(*m_sky_box_path, true, true)});
        }
        catch (Exception &e){ LOG_WARNING << e.what(); }
    }
    else if(theProperty == m_half_extents)
    {
        m_dirty_flag = true;
        m_aabb = gl::AABB(-m_half_extents->value(), m_half_extents->value());
    }
    else if(theProperty == m_num_objects)
    {
        m_dirty_flag = true;
    }
}

void AsteroidField::load_assets()
{
    m_proto_objects.clear();
    
    fs::add_search_path(*m_model_folder);
    fs::add_search_path(*m_texture_folder);
    auto shader = gl::create_shader(gl::ShaderType::GOURAUD);
    
    for (const auto &p : get_directory_entries(*m_model_folder, fs::FileType::MODEL))
    {
        auto mesh = assimp::load_model(p);
        if(mesh)
        {
            auto &verts = mesh->geometry()->vertices();
            vec3 centroid = gl::calculate_centroid(verts);
            for(auto &v : verts){ v -= centroid; }
            mesh->geometry()->create_gl_buffers();
            mesh->geometry()->compute_aabb();
            
            mesh->material()->set_shader(shader);
            mesh->material()->set_ambient(gl::COLOR_WHITE);
            
            auto aabb = mesh->aabb();
            float scale_factor = 50.f / aabb.width();
            mesh->set_scale(scale_factor);
            
            m_proto_objects.push_back(mesh);
        }
    }
    
    m_proto_textures.clear();
    
    for(auto &p : fs::get_directory_entries(*m_texture_folder))
    {
        try
        {
            m_proto_textures.push_back(gl::create_texture_from_file(p, true, true));
        }
        catch (Exception &e){ LOG_WARNING << e.what(); }
    }
}

void AsteroidField::create_scene(int num_objects)
{
    scene()->clear();
    
    // add lights to scene
    for (auto l : lights()){ scene()->add_object(l ); }
    m_light_component->set_lights(lights());
    
    int m = 0;
    
    for(int i = 0; i < num_objects; i++)
    {
        auto test_mesh = m_proto_objects[m % m_proto_objects.size()]->copy();
        test_mesh->set_scale(test_mesh->scale() * random<float>(.5f, 3.f));
        m++;
        
        if(test_mesh->material()->textures().empty())
        {
            test_mesh->material()->add_texture(m_proto_textures[m % m_proto_textures.size()]);
        }
        
        // random spawn position
        test_mesh->set_position(glm::linearRand(m_aabb.min, m_aabb.max));
        
        // object rotation via update-functor
        vec3 rot_vec = glm::ballRand(1.f);
        float rot_speed = glm::radians(random<float>(10.f, 90.f));
        test_mesh->set_update_function([test_mesh, rot_vec, rot_speed](float t)
        {
            test_mesh->transform() = glm::rotate(test_mesh->transform(), rot_speed * t, rot_vec);
        });
        scene()->add_object(test_mesh);
    }
}
