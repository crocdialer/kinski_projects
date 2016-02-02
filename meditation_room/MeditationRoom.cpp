//
//  MeditationRoom.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "MeditationRoom.h"
#include "gl/ShaderLibrary.h"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void MeditationRoom::setup()
{
    ViewerApp::setup();
    
    fonts()[1].load("Courier New Bold.ttf", 78);
    
    register_property(m_output_res);
    register_property(m_shift_angle);
    register_property(m_shift_amount);
    register_property(m_blur_amount);
    observe_properties();
    add_tweakbar_for_component(shared_from_this());
    
    // create 2 empty fbos
    m_fbos.resize(2);
    set_fbo_state();
    
    gl::Shader rgb_shader;
    rgb_shader.loadFromData(unlit_vert, kinski::read_file("rgb_shift.frag"));
    m_mat_rgb_shift = gl::Material::create(rgb_shader);
    
    load_settings();
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    // read sensors, according to current state
    
    switch (m_current_state)
    {
        case State::IDLE:
        case State::MANDALA_LED:
        case State::DESC_MOVIE:
        case State::MEDITATION:
            m_chair_sensor.update(timeDelta);
            
            if(m_chair_sensor.is_touched())
            {
                LOG_DEBUG << "touch detected";
            }
            break;
    }
    
    // ensure correct fbo status here
    set_fbo_state();
    
    m_mat_rgb_shift->textures() = {m_fbos[0].getTexture()};
    m_mat_rgb_shift->uniform("u_shift_amount", *m_shift_amount);
    m_mat_rgb_shift->uniform("u_shift_angle", *m_shift_angle);
    m_mat_rgb_shift->uniform("u_blur_amount", *m_blur_amount);
    m_mat_rgb_shift->uniform("u_window_dimension", gl::windowDimension());
    
    *m_shift_angle += 2.5f * timeDelta;
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::draw()
{
    textures()[0] = gl::render_to_texture(m_fbos[0], [this]()
    {
        gl::clear();
        gl::drawText2D("xxtreme shalom", fonts()[1], gl::COLOR_WHITE, gl::windowDimension() / 5.f);
        gl::drawCircle(gl::windowDimension() / 2.f, 95.f, false, 48);
    });
    
    gl::drawQuad(m_mat_rgb_shift, gl::windowDimension());
    
    if(displayTweakBar()){ draw_textures(textures()); }
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
    
    m_fbos[0] = gl::Fbo(w, h);
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::got_message(const std::vector<uint8_t> &the_message)
{
    LOG_INFO<<string(the_message.begin(), the_message.end());
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::tearDown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::set_fbo_state()
{
    if(!m_fbos[0] || m_fbos[0].getSize() != m_output_res->value())
    {
        gl::Fbo::Format fmt;
        fmt.setSamples(16);
        m_fbos[0] = gl::Fbo(*m_output_res, fmt);
    }
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::read_door_sensor()
{

}

/////////////////////////////////////////////////////////////////

void MeditationRoom::set_mandala_leds(const gl::Color &the_color)
{
    //TODO:
}
