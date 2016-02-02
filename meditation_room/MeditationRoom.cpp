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
    
    register_property(m_cap_sense_dev_name);
    register_property(m_motion_sense_dev_name);
    register_property(m_output_res);
    register_property(m_shift_angle);
    register_property(m_shift_amount);
    register_property(m_blur_amount);
    register_property(m_timeout_idle);
    
    observe_properties();
    add_tweakbar_for_component(shared_from_this());
    
    // create 2 empty fbos
    m_fbos.resize(2);
    set_fbo_state();
    
    // init a reasonable serial buffer
    m_serial_buf.resize(2048);
    
    gl::Shader rgb_shader;
    rgb_shader.loadFromData(unlit_vert, kinski::read_file("rgb_shift.frag"));
    m_mat_rgb_shift = gl::Material::create(rgb_shader);
    
    // setup timer objects
    m_timer_idle = Timer(io_service(), [this](){ change_state(State::IDLE); });
    
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
            m_tmp_motion = detect_motion();
            break;
            
        case State::MANDALA_ILLUMINATED:
            
            // TODO: remove when state machine works properly
            m_tmp_motion = detect_motion();
            
            m_cap_sense.update(timeDelta);
            
            if(m_cap_sense.is_touched(12))
            {
//                LOG_DEBUG << "touch detected";
            }
            break;
            
        case State::DESC_MOVIE:
            // nothing to do here
            break;
            
        case State::MEDITATION:
            //
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
        if(m_cap_sense.is_touched(12))
        {
            gl::drawText2D("the midas touch", fonts()[1], gl::COLOR_WHITE, gl::windowDimension() / 5.f);
        }
        if(m_tmp_motion)
        {
            gl::drawText2D("emotion", fonts()[1], gl::COLOR_WHITE, vec2(80, 200));
        }
        
        gl::drawCircle(gl::windowDimension() / 2.f, 95.f, true, 48);
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
    
    if(theProperty == m_cap_sense_dev_name)
    {
        m_cap_sense.connect(*m_cap_sense_dev_name);
    }
    else if(theProperty == m_motion_sense_dev_name)
    {
        if(m_motion_sense_dev_name->value().empty()){ m_motion_sense.setup(0, 57600); }
        else{ m_motion_sense.setup(*m_motion_sense_dev_name, 57600); }
        
        // finally flush the newly initialized device
        if(m_motion_sense.isInitialized()){ m_motion_sense.flush(); }
    }
}

/////////////////////////////////////////////////////////////////

bool MeditationRoom::change_state(State the_state)
{
    bool ret = true;
    
    // sanity check
    switch(m_current_state)
    {
        case State::IDLE:
            ret = the_state == State::MANDALA_ILLUMINATED;
            break;
            
        case State::MANDALA_ILLUMINATED:
            ret = (the_state == State::DESC_MOVIE) ||
                (the_state == State::IDLE);
            break;
            
        case State::DESC_MOVIE:
            ret = (the_state == State::MEDITATION) ||
                (the_state == State::MANDALA_ILLUMINATED);
            break;
            
        case State::MEDITATION:
            ret = the_state == State::MANDALA_ILLUMINATED;
            break;
    }
    if(ret)
    {
        m_current_state = the_state;
        
        // handle state transition
        switch(m_current_state)
        {
            case State::IDLE:
                // fade in audio
                // fade out lights
                break;
                
            case State::MANDALA_ILLUMINATED:
                break;
                
            case State::DESC_MOVIE:
                break;
                
            case State::MEDITATION:
                break;
        }
    }
    else{ LOG_DEBUG << "invalid state change requested"; }
    
    return ret;
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

bool MeditationRoom::detect_motion()
{
//    LOG_DEBUG << "read_motion_sensor()";
    
    //TODO: read sensor value via Serial device
    if(m_motion_sense.isInitialized())
    {
        size_t bytes_to_read = std::min(m_motion_sense.available(),
                                        m_serial_buf.size());
        
        if(!bytes_to_read)
        {
            // reconnect
        }
        
        uint8_t *buf_ptr = &m_serial_buf[0];
        m_motion_sense.readBytes(&m_serial_buf[0], bytes_to_read);
        
        for(uint32_t i = 0; i < bytes_to_read; i++)
        {
            if(*buf_ptr++){ return true; }
        }
    }
    return false;
}

void MeditationRoom::read_belt_sensor()
{
    LOG_DEBUG << "read_belt_sensor()";
    
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::set_mandala_leds(const gl::Color &the_color)
{
    LOG_DEBUG << "set_mandala_leds";
    
    //TODO: send colour via Serial device
}
