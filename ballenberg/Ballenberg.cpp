//
//  Ballenberg.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "Ballenberg.hpp"
#include "gl/ShaderLibrary.h"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void Ballenberg::setup()
{
    ViewerApp::setup();
    
    fonts()[1].load("Courier New Bold.ttf", 64);
    register_property(m_asset_base_dir);
    register_property(m_cap_sense_dev_name);
    register_property(m_motion_sense_dev_name);
    register_property(m_timeout_idle);
    register_property(m_timeout_movie_start);
    register_property(m_timeout_fade);
    register_property(m_volume);
    
    observe_properties();
    add_tweakbar_for_component(shared_from_this());
    
    
    // init a reasonable serial buffer
    m_serial_buf.resize(2048);
    
    // setup timer objects
    m_timer_idle = Timer(io_service(), [this](){ change_state(State::IDLE, true); });
    m_timer_motion_reset = Timer(io_service(), [this](){ m_motion_detected = false; });
    
    // warp component
//    m_warp = std::make_shared<WarpComponent>();
//    m_warp->observe_properties();
//    add_tweakbar_for_component(m_warp);
    
    m_cap_sense.set_touch_callback([](int i)
    {
        LOG_DEBUG << "touched pad: " << i;
    });
    
    if(!load_assets()){ LOG_ERROR << "could not load assets"; }
    load_settings();
    
    change_state(State::IDLE, true);
}

/////////////////////////////////////////////////////////////////

void Ballenberg::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    // update sensor status
    detect_motion();
    m_cap_sense.update(timeDelta);
    
    // update according to current state
    
    switch (m_current_state)
    {
        case State::IDLE:
            if(m_motion_detected){ change_state(State::MANDALA_ILLUMINATED); }
            break;
            
        case State::MANDALA_ILLUMINATED:
            if(m_cap_sense.is_touched(12) && m_show_movie){ change_state(State::DESC_MOVIE); }
            break;
            
        case State::DESC_MOVIE:
            
            break;
            
        case State::MEDITATION:
            break;
    }
}

/////////////////////////////////////////////////////////////////

void Ballenberg::draw()
{
    // draw final result
    gl::draw_texture(textures()[TEXTURE_OUTPUT], gl::window_dimension());
    
    // draw status overlay
    draw_status_info();
    
    if(displayTweakBar()){ draw_textures(textures()); }
}

/////////////////////////////////////////////////////////////////

void Ballenberg::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void Ballenberg::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
    
    int next_state = -1;
    std::vector<State> states = {State::IDLE, State::MANDALA_ILLUMINATED, State::DESC_MOVIE,
        State::MEDITATION};
    
    if(!displayTweakBar())
    {
        switch (e.getCode())
        {
            case Key::_1:
            case Key::_2:
            case Key::_3:
            case Key::_4:
                next_state = e.getCode() - Key::_1;
                break;
                
            default:
                break;
        }
    }
    
    if(next_state >= 0){ change_state(states[next_state], e.isShiftDown()); }
}

/////////////////////////////////////////////////////////////////

void Ballenberg::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void Ballenberg::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void Ballenberg::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void Ballenberg::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void Ballenberg::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void Ballenberg::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void Ballenberg::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{
    
}

/////////////////////////////////////////////////////////////////

void Ballenberg::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{
    
}

/////////////////////////////////////////////////////////////////

void Ballenberg::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{
    
}

/////////////////////////////////////////////////////////////////

void Ballenberg::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void Ballenberg::tearDown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void Ballenberg::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
    
    if(theProperty == m_cap_sense_dev_name)
    {
        background_queue().submit([this]()
        {
            m_cap_sense.connect(*m_cap_sense_dev_name);
        });
    }
    else if(theProperty == m_motion_sense_dev_name)
    {
        if(!m_motion_sense_dev_name->value().empty())
        {
            background_queue().submit([this]()
            {
                m_motion_sense.setup(*m_motion_sense_dev_name, 57600);
              
                // finally flush the newly initialized device
                if(m_motion_sense.isInitialized()){ m_motion_sense.flush(); }
            });
        }
    }
    else if(theProperty == m_volume)
    {
//        if(m_audio)
//        {
//            m_audio->set_volume(*m_volume);
//        }
    }
    else if(theProperty == m_timeout_fade)
    {
        // setup animations
        animations()[AUDIO_FADE_IN] = animation::create(m_volume, 0.f, .4f, *m_timeout_fade);
        animations()[AUDIO_FADE_OUT] = animation::create(m_volume, .4f, .0f, *m_timeout_fade);
    }
}

/////////////////////////////////////////////////////////////////

bool Ballenberg::change_state(State the_state, bool force_change)
{
    bool ret = true;
    
    // sanity check
    switch(m_current_state)
    {
        case State::IDLE:
            ret = the_state == State::MANDALA_ILLUMINATED;
            break;
            
        case State::MANDALA_ILLUMINATED:
            ret =   (the_state == State::IDLE) || (the_state == State::DESC_MOVIE) ||
                    (the_state == State::MEDITATION);
            break;
            
        case State::DESC_MOVIE:
            ret = the_state == State::MANDALA_ILLUMINATED;
            break;
            
        case State::MEDITATION:
            ret = the_state == State::MANDALA_ILLUMINATED;
            break;
    }
    
    if (!ret){ LOG_DEBUG << "invalid state change requested"; }
    if (force_change){ LOG_DEBUG << "forced state change"; }
    
    if(ret || force_change)
    {
        textures()[TEXTURE_OUTPUT].reset();
        
        //TODO: handle state transition
        switch(the_state)
        {
            case State::IDLE:
                if(animations()[AUDIO_FADE_IN]) { animations()[AUDIO_FADE_IN]->start(); }
                if(animations()[LIGHT_FADE_OUT]){ animations()[LIGHT_FADE_OUT]->start(); }
                m_show_movie = true;
//                if(m_audio){ m_audio->set_volume(1.f); }
//                *m_led_color = gl::COLOR_BLACK;
                break;
                
            case State::MANDALA_ILLUMINATED:
                
                //TODO: fade in here
//                *m_led_color = gl::COLOR_WHITE;
                if(animations()[AUDIO_FADE_IN]){ animations()[AUDIO_FADE_IN]->stop(); }
                if(animations()[LIGHT_FADE_OUT]){ animations()[LIGHT_FADE_OUT]->stop(); }
                if(animations()[LIGHT_FADE_IN]){ animations()[LIGHT_FADE_IN]->start(); }
                    
                if((m_current_state == State::IDLE) && animations()[AUDIO_FADE_OUT])
                {
                    animations()[AUDIO_FADE_OUT]->start();
                }
                else{ *m_volume = 0.f; }
                
                m_timer_idle.expires_from_now(*m_timeout_idle);
                
                break;
                
            case State::DESC_MOVIE:
                break;
                
            case State::MEDITATION:

                break;
        }
        m_current_state = the_state;
    }
    
    return ret;
}

/////////////////////////////////////////////////////////////////

void Ballenberg::detect_motion()
{
    if(m_motion_sense.isInitialized())
    {
        size_t bytes_to_read = std::min(m_motion_sense.available(),
                                        m_serial_buf.size());
        
        if(!bytes_to_read)
        {
//            m_motion_sense_dev_name->notifyObservers();
        }
        
        uint8_t *buf_ptr = &m_serial_buf[0];
        m_motion_sense.readBytes(&m_serial_buf[0], bytes_to_read);
        
        for(uint32_t i = 0; i < bytes_to_read; i++)
        {
            if(*buf_ptr++)
            {
                m_motion_detected = true;
                m_timer_motion_reset.expires_from_now(5.f);
                break;
            }
        }
    }
}

/////////////////////////////////////////////////////////////////

void Ballenberg::draw_status_info()
{
    vec2 offset(50, 50), step(0, 35);
    
    // display current state
    auto state_str = "State: " + m_state_string_map[m_current_state];
    gl::draw_text_2D(state_str, fonts()[0], gl::COLOR_WHITE, offset);
    offset += step;
    
    bool motion_sensor_found = m_motion_sense.isInitialized();
    bool cap_sensor_found = m_cap_sense.is_initialized();
    
    string ms_string = motion_sensor_found ? "ok" : "not found";
    string cs_string = cap_sensor_found ? "ok" : "not found";
    
    gl::Color cap_col = (cap_sensor_found && m_cap_sense.is_touched()) ? gl::COLOR_GREEN : gl::COLOR_RED;
    gl::Color motion_col = (motion_sensor_found && m_motion_detected) ?
        gl::COLOR_GREEN : gl::COLOR_RED;
    
    // motion sensor
    gl::draw_text_2D("motion-sensor: " + ms_string, fonts()[0], motion_col, offset);
    
    // chair sensor
    offset += step;
    gl::draw_text_2D("cap-sensor: " + cs_string, fonts()[0], cap_col, offset);
}

bool Ballenberg::load_assets()
{
    if(!is_directory(*m_asset_base_dir)){ return false; }
    
    auto audio_files = get_directory_entries(*m_asset_base_dir, FileType::AUDIO, true);
    auto video_files = get_directory_entries(*m_asset_base_dir, FileType::MOVIE, true);
    
    if(!audio_files.empty())
    {
        // background chanting audio
//        m_audio.reset(new audio::Fmod_Sound(audio_files.front()));
//        m_audio->set_loop(true);
//        m_audio->play();
    }
    
    if(!video_files.empty())
    {
        // description movie
//        m_movie->load(video_files.front());
    }
    
    return true;
}