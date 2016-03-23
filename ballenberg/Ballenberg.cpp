//
//  Ballenberg.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "Ballenberg.hpp"
#include "gl/ShaderLibrary.h"
#include "core/networking.hpp"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void Ballenberg::setup()
{
    ViewerApp::setup();
    
    fonts()[1].load(fonts()[0].path(), 64);
    register_property(m_asset_base_dir);
    register_property(m_cap_sense_dev_name);
    register_property(m_motion_sense_dev_name_01);
    register_property(m_motion_sense_dev_name_02);
    register_property(m_dmx_dev_name);
    register_property(m_timeout_idle);
    register_property(m_duration_lamp_noise);
    register_property(m_volume);
    register_property(m_cap_sense_thresh_touch);
    register_property(m_cap_sense_thresh_release);
    register_property(m_cap_sense_charge_current);
    register_property(m_spot_color);
    
    observe_properties();
    add_tweakbar_for_component(shared_from_this());
    
    // setup timer objects
    m_timer_idle = Timer(io_service(), [this](){ change_state(State::IDLE, true); });
    m_timer_lamp_noise = Timer(io_service());
    
    // warp component
//    m_warp = std::make_shared<WarpComponent>();
//    m_warp->observe_properties();
//    add_tweakbar_for_component(m_warp);
    
    m_cap_sense.set_touch_callback([](int i)
    {
        LOG_DEBUG << "touched pad: " << i;
    });
    
    // enable file logging
    Logger::get()->set_use_log_file(true);
    
    if(!load_assets()){ LOG_ERROR << "could not load assets"; }
    load_settings();
    
    change_state(State::IDLE, true);
}

/////////////////////////////////////////////////////////////////

void Ballenberg::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    // update sensor status
    m_motion_sense_01.update(timeDelta);
    m_motion_sense_02.update(timeDelta);
    m_cap_sense.update(timeDelta);
    
    // motion -> kitchen
    if(m_motion_sense_01.distance() && m_timer_motion_reset.has_expired())
    {
        m_timer_motion_reset.expires_from_now(20.f);
        
        // play random recipe movie
        std::string path = join_paths(*m_asset_base_dir, "movies/kitchen");
        auto tmp = get_directory_entries(path, FileType::MOVIE, true);
        std::vector<std::string> paths(tmp.begin(), tmp.end());
        
        if(!paths.empty())
        {
            uint32_t rnd_idx = kinski::random<uint32_t>(0, tmp.size());
            
            net::async_send_tcp(background_queue().io_service(), "play " +
                                join_paths("/home/pi/ballenberg_assets/movies/kitchen",
                                           get_filename_part(paths[rnd_idx])),
                                m_ip_kitchen, 33333);
        }else{ LOG_WARNING << "could not find a recipe movie"; }
    }
    
    // motion -> living room
    if(m_motion_sense_02.distance())
    {
        // play religion movie #1
        std::string path = join_paths(*m_asset_base_dir, "movies/living_room");
        
        auto tmp = get_directory_entries(path, FileType::MOVIE, true);
        std::vector<std::string> paths(tmp.begin(), tmp.end());
        
        if(!paths.empty())
        {
            net::async_send_tcp(background_queue().io_service(), "play " +
                                join_paths("/home/pi/ballenberg_assets/movies/living_room",
                                           get_filename_part(paths[0])),
                                m_ip_living_room, 33333);
        }else{ LOG_WARNING << "could not find a religion movie"; }
    }
    
    // cap-sense(bottle) -> living room
    if(m_cap_sense.is_touched())
    {
        // play religion movie #2
        std::string path = join_paths(*m_asset_base_dir, "movies/living_room");
        
        auto tmp = get_directory_entries(path, FileType::MOVIE, true);
        std::vector<std::string> paths(tmp.begin(), tmp.end());
    
        m_timer_lamp_noise.expires_from_now(*m_duration_lamp_noise);
        
        if(paths.size() > 1)
        {
            net::async_send_tcp(background_queue().io_service(), "play " +
                                join_paths("/home/pi/ballenberg_assets/movies/living_room",
                                           get_filename_part(paths[1])),
                                m_ip_living_room, 33333);
        }else{ LOG_WARNING << "could not find a religion movie"; }
    }

    // lamp flicker -> set random color
    if(m_timer_lamp_noise.has_expired()){ *m_spot_color = gl::COLOR_BLACK; }
    else{ *m_spot_color = glm::linearRand(gl::COLOR_BLACK, gl::COLOR_WHITE);}
}

/////////////////////////////////////////////////////////////////

void Ballenberg::draw()
{
    gl::draw_text_2D("Ballenberg IXDM", fonts()[1], gl::COLOR_WHITE, gl::vec2(50, 10));
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
    
    if(!displayTweakBar())
    {
        switch (e.getCode())
        {
            case Key::_1:
                m_timer_motion_reset.cancel();
                net::async_send_tcp(background_queue().io_service(), "seek_to_time 0.0",
                                    m_ip_kitchen, 33333);
                net::async_send_tcp(background_queue().io_service(), "pause",
                                    m_ip_kitchen, 33333);
                
            case Key::_2:
                net::async_send_tcp(background_queue().io_service(), "seek_to_time 0.0",
                                    m_ip_living_room, 33333);
                net::async_send_tcp(background_queue().io_service(), "pause",
                                    m_ip_living_room, 33333);
            case Key::_3:
            case Key::_4:
                next_state = e.getCode() - Key::_1;
                break;
                
            default:
                break;
        }
    }
    if(next_state >= 0){ change_state(State::IDLE, e.isShiftDown()); }
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
            if(m_cap_sense.connect(*m_cap_sense_dev_name))
            {
                m_cap_sense.set_thresholds(*m_cap_sense_thresh_touch, *m_cap_sense_thresh_release);
                m_cap_sense.set_charge_current(*m_cap_sense_charge_current);
            }
        });
    }
    else if(theProperty == m_motion_sense_dev_name_01)
    {
        m_motion_sense_01.connect(*m_motion_sense_dev_name_01);
    }
    else if(theProperty == m_motion_sense_dev_name_02)
    {
        m_motion_sense_02.connect(*m_motion_sense_dev_name_02);
    }
    else if(theProperty == m_dmx_dev_name)
    {
        m_dmx.connect(*m_dmx_dev_name);
    }
    else if(theProperty == m_volume)
    {

    }
    else if(theProperty == m_cap_sense_thresh_touch ||
            theProperty == m_cap_sense_thresh_release)
    {
        if(m_cap_sense.is_initialized())
        {
            m_cap_sense.set_thresholds(*m_cap_sense_thresh_touch, *m_cap_sense_thresh_release);
        }
    }
    else if(theProperty == m_cap_sense_charge_current)
    {
        if(m_cap_sense.is_initialized())
        {
            m_cap_sense.set_charge_current(*m_cap_sense_charge_current);
        }
    }
    else if(theProperty == m_spot_color)
    {
        //TODO: set dmx values
        m_dmx[1] = 255 * m_spot_color->value().r;
        m_dmx.update();
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
            break;
            
        default:
            break;
    }
    
    if(!ret){ LOG_DEBUG << "invalid state change requested"; }
    if(force_change){ LOG_DEBUG << "forced state change"; }
    
    if(ret || force_change)
    {
        switch(the_state)
        {
            case State::IDLE:
                break;
        }
        m_current_state = the_state;
    }
    
    return ret;
}

/////////////////////////////////////////////////////////////////

void Ballenberg::draw_status_info()
{
    vec2 offset(50, 75), step(0, 35);
    
    // display current state
//    auto state_str = "State: " + m_state_string_map[m_current_state];
//    gl::draw_text_2D(state_str, fonts()[0], gl::COLOR_WHITE, offset);
//    offset += step;
    
    bool cap_sensor_found = m_cap_sense.is_initialized();
    string cs_string = cap_sensor_found ? "ok" : "not found";
    string dmx_string = m_dmx.is_initialized() ? "ok" : "not found";
    
    gl::Color cap_col = (cap_sensor_found && m_cap_sense.is_touched()) ? gl::COLOR_GREEN : gl::COLOR_WHITE;
    gl::Color motion_col = gl::COLOR_WHITE;
    
    // motion sensor 1
    string ms_string_01 = m_motion_sense_01.is_initialized() ? "ok" : "not found";
    motion_col = m_motion_sense_01.distance() ? gl::COLOR_GREEN : gl::COLOR_WHITE;
    gl::draw_text_2D("motion-sensor (kitchen): " + (m_motion_sense_01.distance() ?
                     as_string(m_motion_sense_01.distance()) : ms_string_01), fonts()[0],
                     motion_col, offset);
    offset += step;
    
    // motion sensor 2
    string ms_string_02 = m_motion_sense_02.is_initialized() ? "ok" : "not found";
    motion_col = m_motion_sense_02.distance() ? gl::COLOR_GREEN : gl::COLOR_WHITE;
    gl::draw_text_2D("motion-sensor (living room): " + (m_motion_sense_02.distance() ?
                     as_string(m_motion_sense_02.distance()) : ms_string_02), fonts()[0],
                     motion_col, offset);
    offset += step;
    
    // dmx device
    gl::draw_text_2D("dmx (living room): " + dmx_string, fonts()[0], gl::COLOR_WHITE, offset);
    offset += step;
    
    // cap sensor
    gl::draw_text_2D("cap-sensor (living room): " + cs_string, fonts()[0], cap_col, offset);
}

bool Ballenberg::load_assets()
{
    if(!is_directory(*m_asset_base_dir)){ return false; }
    
    auto audio_files = get_directory_entries(*m_asset_base_dir, FileType::AUDIO, true);
    auto video_files = get_directory_entries(*m_asset_base_dir, FileType::MOVIE, true);
    
    if(!audio_files.empty())
    {

    }
    
    if(!video_files.empty())
    {
        
    }
    
    return true;
}