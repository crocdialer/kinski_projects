//
//  ParticleSample.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "ParticleSample.hpp"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void ParticleSample::setup()
{
    ViewerApp::setup();
    register_property(m_draw_fps);
    register_property(m_texture_path);
    register_property(m_num_particles);
    register_property(m_emission_rate);
    register_property(m_num_burst_particles);
    register_property(m_gravity);
    register_property(m_point_size);
    register_property(m_point_color);
    register_property(m_start_velocity_min);
    register_property(m_start_velocity_max);
    register_property(m_lifetime_min);
    register_property(m_lifetime_max);
    register_property(m_bouncyness);
    register_property(m_use_contraints);
    register_property(m_debug_life);
    observe_properties();
    load_settings();
}

/////////////////////////////////////////////////////////////////

void ParticleSample::init_particles(uint32_t the_num)
{
    m_particle_system->init_with_count(the_num);
    m_particle_system->set_kernel_source("kernels.cl");
    m_particle_mesh = m_particle_system->mesh();

    float vals[2];
    glGetFloatv(GL_POINT_SIZE_RANGE, vals);
    m_point_size->set_range(vals[0], vals[1]);
    m_particle_mesh->material()->set_point_size(*m_point_size);
    m_particle_mesh->material()->set_diffuse(*m_point_color);

    auto t = textures()[TEXTURE_PARTICLE];
    if(t){ m_particle_mesh->material()->add_texture(t); }
    m_particle_mesh->material()->set_shader(gl::create_shader(gl::ShaderType::POINTS_SPHERE));
    m_particle_mesh->add_tag(gl::SceneRenderer::TAG_NO_CULL);
    m_particle_system->set_gravity(*m_gravity);
    m_particle_system->set_lifetime(*m_lifetime_min, *m_lifetime_max);
    m_particle_system->set_start_velocity(*m_start_velocity_min, *m_start_velocity_max);
    m_particle_system->set_bouncyness(*m_bouncyness);
    m_particle_system->forces() = {gl::vec4(0, 15, 0, 500)};
    m_particle_system->planes() = { gl::Plane(gl::vec3(), gl::vec3(0, 1, 0)) };
    m_particle_system->set_mesh(m_particle_mesh);
    m_particle_system->set_emission_rate(*m_emission_rate);
    scene()->add_object(m_particle_system);

//    m_particle_system->transform() = gl::rotate(gl::mat4(), glm::pi<float>() / 6.f, gl::X_AXIS);
    gl::vec3 axis = glm::inverse(gl::mat3(m_particle_system->transform())) * gl::Y_AXIS;
    m_particle_system->set_update_function([this, axis](float dt)
    {
        m_particle_system->transform() = gl::rotate(m_particle_system->transform(),
                                                    dt * 0.2f, axis);
    });
    m_needs_init = false;
}

/////////////////////////////////////////////////////////////////

void ParticleSample::update(float timeDelta)
{
    ViewerApp::update(timeDelta);

    // construct ImGui window for this frame
    if(display_tweakbar())
    {
        gl::draw_component_ui(shared_from_this());
        gl::draw_component_ui(m_light_component);
//        if(*m_use_warping){ gl::draw_component_ui(m_warp_component); }
    }

    if(m_needs_init){ init_particles(*m_num_particles); }
}

/////////////////////////////////////////////////////////////////

void ParticleSample::draw()
{
    gl::clear();
    gl::set_matrices(camera());
    if(*m_draw_grid){ gl::draw_grid(50, 50); }

    gl::draw_transform(m_particle_system->global_transform(), 10.f);

    scene()->render(camera());

    // draw enabled light dummies
    m_light_component->draw_light_dummies();
    
    if(*m_draw_fps)
    {
        gl::draw_text_2D(to_string(fps(), 1), fonts()[0],
                         glm::mix(gl::COLOR_OLIVE, gl::COLOR_WHITE,
                                  glm::smoothstep(0.f, 1.f, fps() / max_fps())),
                         gl::vec2(10));
    }
    
    if(Logger::get()->severity() >= Severity::DEBUG)
    {
        gl::draw_text_2D(format("particles: %dk / %dk", m_particle_system->num_particles() / 1000,
                                m_particle_system->max_num_particles() / 1000), fonts()[0],
                         gl::COLOR_WHITE, gl::vec2(gl::window_dimension().x - 250, 10));
        
        gl::draw_text_2D(format("press 'e' to emit bursts"), fonts()[0],
                         gl::COLOR_WHITE, gl::vec2(gl::window_dimension().x - 250, 30));
    }
}

/////////////////////////////////////////////////////////////////

void ParticleSample::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void ParticleSample::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);
    
    switch(e.getCode())
    {
        case Key::_E:
            m_particle_system->emit_particles(*m_num_burst_particles);
            break;
    }
}

/////////////////////////////////////////////////////////////////

void ParticleSample::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void ParticleSample::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);
}

/////////////////////////////////////////////////////////////////

void ParticleSample::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);

    if(e.isControlDown())
    {
        auto ray = gl::calculate_ray(camera(), e.getPos());
        gl::Plane ground(vec3(0), vec3(0, 1, 0));
        auto intersection = ground.intersect(ray);

        if(intersection)
        {
            m_particle_system->set_position(ray * intersection.distance);
        }
    }
}

/////////////////////////////////////////////////////////////////

void ParticleSample::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void ParticleSample::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void ParticleSample::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void ParticleSample::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void ParticleSample::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void ParticleSample::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
}

/////////////////////////////////////////////////////////////////

void ParticleSample::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ *m_texture_path = f; break; }
}

/////////////////////////////////////////////////////////////////

void ParticleSample::teardown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void ParticleSample::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);

    if(theProperty == m_num_particles)
    {
        m_needs_init = true;
    }
    else if(theProperty == m_gravity)
    {
        m_particle_system->set_gravity(*m_gravity);
    }
    else if(theProperty == m_point_size)
    {
        if(m_particle_mesh){ m_particle_mesh->material()->set_point_size(*m_point_size); }
    }
    else if(theProperty == m_point_color)
    {
        if(m_particle_mesh){ m_particle_mesh->material()->set_diffuse(*m_point_color); }
    }
    else if(theProperty == m_start_velocity_min ||
            theProperty == m_start_velocity_max)
    {
        m_particle_system->set_start_velocity(*m_start_velocity_min, *m_start_velocity_max);
    }
    else if(theProperty == m_emission_rate)
    {
        m_particle_system->set_emission_rate(*m_emission_rate);
    }
    else if(theProperty == m_lifetime_min ||
            theProperty == m_lifetime_max)
    {
        m_particle_system->set_lifetime(*m_lifetime_min, *m_lifetime_max);
    }
    else if(theProperty == m_bouncyness)
    {
        m_particle_system->set_bouncyness(*m_bouncyness);
    }
    else if(theProperty == m_debug_life)
    {
        m_particle_system->set_debug_life(*m_debug_life);

        if(!*m_debug_life && m_particle_mesh)
        {
            for(auto &c : m_particle_mesh->geometry()->colors()){ c = gl::COLOR_WHITE; }
            m_particle_mesh->geometry()->create_gl_buffers();
        }
    }
    else if(theProperty == m_use_contraints)
    {
        m_particle_system->set_use_constraints(*m_use_contraints);
    }
    else if(theProperty == m_texture_path)
    {
        async_load_texture(*m_texture_path, [this](gl::Texture t)
        {
            textures()[TEXTURE_PARTICLE] = t;

            if(m_particle_mesh){ m_particle_mesh->material()->add_texture(t); }
        });
    }
}
