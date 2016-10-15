//
//  MeditationRoom.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "gl/ShaderLibrary.h"
#include "MeditationRoom.hpp"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void MeditationRoom::setup()
{
    ViewerApp::setup();
    
    fonts()[1].load(fonts()[0].path(), 64);
    
    register_property(m_cap_sense_dev_name);
    register_property(m_motion_sense_dev_name);
    register_property(m_bio_sense_dev_name);
    register_property(m_led_dev_name);
    register_property(m_output_res);
    register_property(m_circle_radius);
    register_property(m_shift_angle);
    register_property(m_shift_amount);
    register_property(m_shift_velocity);
    register_property(m_blur_amount);
    register_property(m_timeout_idle);
    register_property(m_timeout_movie_start);
    register_property(m_timeout_fade);
    register_property(m_led_color);
    register_property(m_volume);
    register_property(m_volume_max);
    register_property(m_bio_score);
    register_property(m_bio_thresh);
    register_property(m_bio_sensitivity);
    
    observe_properties();
    add_tweakbar_for_component(shared_from_this());
    
    // create 2 empty fbos
    m_fbos.resize(2);
    set_fbo_state();
    
    // init a reasonable serial buffer
    m_serial_buf.resize(2048);
    
    gl::Shader rgb_shader;
    
#if defined(KINSKI_GLES)
    rgb_shader.loadFromData(unlit_vert, fs::read_file("rgb_shift_es2.frag"));
#else
    rgb_shader.loadFromData(unlit_vert, fs::read_file("rgb_shift.frag"));
#endif
    
    
    m_mat_rgb_shift = gl::Material::create(rgb_shader);
    m_mat_rgb_shift->set_depth_test(false);
    
    // setup timer objects
    m_timer_idle = Timer(main_queue().io_service(), [this](){ change_state(State::IDLE, true); });
    m_timer_motion_reset = Timer(main_queue().io_service(), [this](){ m_motion_detected = false; });
    m_timer_movie_start = Timer(main_queue().io_service(), [this](){ if(m_movie){ m_movie->restart(); } });
    
    // warp component
    m_warp = std::make_shared<WarpComponent>();
    m_warp->observe_properties();
    add_tweakbar_for_component(m_warp);
    
    // output window
//    auto output_window = GLFW_Window::create(1280, 720, "output", false, 0, windows().back()->handle());
//    add_window(output_window);
    
    if(!load_assets()){ LOG_ERROR << "could not load assets"; }
    load_settings();
    
    change_state(State::IDLE, true);
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    // update sensor status
    detect_motion();
    read_bio_sensor();
    m_cap_sense.update(timeDelta);
    
    // update according to current state
    
    switch (m_current_state)
    {
        case State::IDLE:
            if(m_motion_detected){ change_state(State::MANDALA_ILLUMINATED); }
            break;
            
        case State::MANDALA_ILLUMINATED:
            if(m_cap_sense.is_touched(12) && m_show_movie){ change_state(State::DESC_MOVIE); }
            else if(*m_bio_score > *m_bio_thresh){ change_state(State::MEDITATION); }
            break;
            
        case State::DESC_MOVIE:
            
            break;
            
        case State::MEDITATION:
//            m_mat_rgb_shift->textures() = {m_fbos[0].texture()};
            m_mat_rgb_shift->uniform("u_shift_amount", *m_shift_amount);
            m_mat_rgb_shift->uniform("u_shift_angle", *m_shift_angle);
            m_mat_rgb_shift->uniform("u_blur_amount", *m_blur_amount);
            m_mat_rgb_shift->uniform("u_window_dimension", *m_output_res);
            *m_shift_angle += *m_shift_velocity * timeDelta;
            if(*m_bio_score > *m_bio_thresh){ m_timer_idle.expires_from_now(*m_timeout_idle); }
            break;
    }
    
    // ensure correct fbo status here
    set_fbo_state();
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::update_bio_visuals()
{
    if(m_movie && m_current_state == State::DESC_MOVIE)
    {
        m_movie->copy_frame_to_texture(textures()[TEXTURE_OUTPUT], true);
    }
    
    // couple bioscore with meditation-parameters
    float val = map_value<float>(m_bio_sensitivity->value() * *m_bio_score, 0.f, 10.f, 0.f, 1.f);
    val = glm::smoothstep(0.f, 1.f, val);
    
    // shiftamount
    *m_shift_amount = mix<float>(*m_shift_amount, val * 80.f, .15f);
    
    // shiftvelocity
    *m_shift_velocity = mix<float>(*m_shift_velocity, val * 20.f, .015f);
    
    // circle radius
    m_current_circ_radius = mix<float>(m_current_circ_radius, *m_circle_radius * (1.f + val * 2.f), .05f);
    
    // blurmount
    *m_blur_amount = mix<float>(*m_blur_amount, 2.f + 120.f * val, .1f);
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::draw()
{
    if(m_current_state == State::MEDITATION)
    {
        // create undistorted offscreen tex
        textures()[0] = gl::render_to_texture(m_fbos[0], [this]()
        {
            gl::clear();
            gl::draw_circle(gl::window_dimension() / 2.f, m_current_circ_radius, gl::COLOR_WHITE,
                            true, 48);
        });
        m_mat_rgb_shift->textures() = {m_fbos[0].texture()};
        
        // apply distortion shader
        textures()[TEXTURE_OUTPUT] = gl::render_to_texture(m_fbos[1], [this]()
        {
            gl::clear();
            gl::draw_quad(m_mat_rgb_shift, gl::window_dimension());
        });
    }
    
    // draw final result
    m_warp->render_output(textures()[TEXTURE_OUTPUT]);
    
    if(displayTweakBar())
    {
        // draw status overlay
        draw_status_info();
        
        draw_textures(textures());
    }
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::keyPress(const KeyEvent &e)
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
                
//            case Key::_O:
//            {
//                // output window
//                auto output_window = GLFW_Window::create(1280, 720, "output", false, 0,
//                                                         windows().back()->handle());
//                add_window(output_window);
//                output_switch();
//            }
//                break;
                
            default:
                break;
        }
    }
    
    if(next_state >= 0){ change_state(states[next_state], e.isShiftDown()); }
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
                m_motion_sense->setup(*m_motion_sense_dev_name, 57600);
              
                // finally flush the newly initialized device
                if(m_motion_sense->is_initialized()){ m_motion_sense->flush(); }
            });
        }
    }
    else if(theProperty == m_bio_sense_dev_name)
    {
        if(!m_bio_sense_dev_name->value().empty())
        {
            background_queue().submit([this]()
            {
                m_bio_sense->setup(*m_bio_sense_dev_name, 57600);
                
                // finally flush the newly initialized device
                if(m_bio_sense->is_initialized()){ m_bio_sense->flush(); }
            });
        }
    }
    else if(theProperty == m_led_dev_name)
    {
        if(!m_led_dev_name->value().empty())
        {
            background_queue().submit([this]()
            {
                m_led_device->setup(*m_led_dev_name, 57600);
              
                // finally flush the newly initialized device
                if(m_led_device->is_initialized()){ m_led_device->flush(); }
            });
        }
    }
    else if(theProperty == m_led_color){ set_led_color(*m_led_color); }
    else if(theProperty == m_volume)
    {
        if(m_audio){ m_audio->set_volume(*m_volume); }
    }
    else if(theProperty == m_timeout_fade)
    {
        create_animations();
    }
    else if(theProperty == m_bio_score)
    {
        update_bio_visuals();
    }
    else if (theProperty == m_circle_radius){ m_current_circ_radius = *m_circle_radius; }
}

/////////////////////////////////////////////////////////////////

bool MeditationRoom::change_state(State the_state, bool force_change)
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
    
    // create the fade in/out animations based on current values
    create_animations();
    
    if(ret || force_change)
    {
        textures()[TEXTURE_OUTPUT].reset();
        
        //TODO: handle state transition
        switch(the_state)
        {
            case State::IDLE:
                animations()[AUDIO_FADE_IN]->start();
                animations()[LIGHT_FADE_OUT]->start();
                m_show_movie = true;
                break;
                
            case State::MANDALA_ILLUMINATED:
                animations()[AUDIO_FADE_IN]->stop();
                animations()[LIGHT_FADE_OUT]->stop();
                animations()[LIGHT_FADE_IN]->start();
                if(m_movie){ m_movie->pause(); }
                if(m_current_state == State::IDLE){ animations()[AUDIO_FADE_OUT]->start(); }
                else{ *m_volume = 0.f; }
                m_timer_idle.expires_from_now(*m_timeout_idle);
                break;
                
            case State::DESC_MOVIE:
                if(animations()[LIGHT_FADE_IN]){ animations()[LIGHT_FADE_IN]->stop(); }
                if(animations()[LIGHT_FADE_OUT]){ animations()[LIGHT_FADE_OUT]->start(44.f); }
                m_timer_idle.cancel();
                
                if(m_movie)
                {
                    LOG_DEBUG << "starting movie in " << m_timeout_movie_start->value() <<" secs";
                    m_timer_movie_start.expires_from_now(*m_timeout_movie_start);
                    m_movie->set_media_ended_callback([this](media::MovieControllerPtr)
                    {
                        m_show_movie = false;
                        change_state(State::MANDALA_ILLUMINATED);
                    });
                }
                break;
                
            case State::MEDITATION:
                animations()[LIGHT_FADE_IN]->stop();
                animations()[LIGHT_FADE_OUT]->start();
                if(m_movie){ m_movie->pause(); }
                break;
        }
        m_current_state = the_state;
    }
    return ret;
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::set_fbo_state()
{
    if(!m_fbos[0] || m_fbos[0].size() != m_output_res->value())
    {
        gl::Fbo::Format fmt;
        fmt.set_num_samples(8);
        m_fbos[0] = gl::Fbo(*m_output_res, fmt);
    }
    if(!m_fbos[1] || m_fbos[1].size() != m_output_res->value())
    {
        gl::Fbo::Format fmt;
        fmt.set_num_samples(8);
        m_fbos[1] = gl::Fbo(*m_output_res, fmt);
    }
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::detect_motion()
{
    if(m_motion_sense->is_initialized())
    {
        size_t bytes_to_read = std::min(m_motion_sense->available(),
                                        m_serial_buf.size());
        
        if(!bytes_to_read)
        {
//            m_motion_sense_dev_name->notifyObservers();
        }
        
        uint8_t *buf_ptr = &m_serial_buf[0];
        m_motion_sense->read_bytes(&m_serial_buf[0], bytes_to_read);
        
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

void MeditationRoom::read_bio_sensor()
{
    static std::vector<uint8_t> buf, accum;
//    const size_t num_bytes = sizeof(float);
    buf.resize(32);
    
    enum SerialCodes{SERIAL_END_CODE = '\n'};
    
    if(m_bio_sense->is_initialized())
    {
        size_t bytes_to_read = std::min(m_bio_sense->available(), buf.size());
        
        if(!bytes_to_read){}
        
        float val;
        bool reading_complete = false;
        uint8_t *buf_ptr = &buf[0];
        m_bio_sense->read_bytes(&buf[0], bytes_to_read);
        
        for(uint32_t i = 0; i < bytes_to_read; i++)
        {
            uint8_t byte = *buf_ptr++;
            
            switch(byte)
            {
                case SERIAL_END_CODE:
                    val = string_to<float>(string(accum.begin(), accum.end()));
                    accum.clear();
                    reading_complete = true;
                    break;
                    
                default:
                    accum.push_back(byte);
                    break;
            }
        }
        if(reading_complete){ *m_bio_score = clamp(val, 0.f, 5.f); }
    }
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::set_led_color(const gl::Color &the_color)
{
    if(m_led_device->is_initialized())
    {
        // create RGBA integer val
//        uint32_t rgb_val = (static_cast<uint32_t>(the_color.r * 255) << 24) |
//        (static_cast<uint32_t>(the_color.g * 255) << 16) |
//        (static_cast<uint32_t>(the_color.b * 255) << 8) |
//        static_cast<uint32_t>(the_color.a * 255);
        
//        m_led_device.writeBytes(&rgb_val, sizeof(rgb_val));
//        m_led_device.writeByte('\n');
        
        uint8_t c = the_color.r * 255;
        m_led_device->write_bytes(&c, 1);
        m_led_device->flush();
    }
    
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::draw_status_info()
{
    vec2 offset(50, 50), step(0, 35);
    
    // display current state
    auto state_str = "State: " + m_state_string_map[m_current_state];
    if((m_current_state == State::DESC_MOVIE) && m_movie)
    {
        state_str += " (" + to_string(m_movie->current_time(),1) + "/" +
            to_string(m_movie->duration(), 1) + ")";
    }
    gl::draw_text_2D(state_str, fonts()[0], gl::COLOR_WHITE, offset);
    offset += step;
    
    bool motion_sensor_found = m_motion_sense->is_initialized();
    bool cap_sensor_found = m_cap_sense.is_initialized();
    bool bio_sensor_found = m_bio_sense->is_initialized();
    bool led_device_found = m_led_device->is_initialized();
    
    string ms_string = motion_sensor_found ? "ok" : "not found";
    string cs_string = cap_sensor_found ? "ok" : "not found";
    string bs_string = bio_sensor_found ? to_string(m_bio_score->value(), 2) : "not found";
    string led_string = led_device_found ? "ok" : "not found";
    
    gl::Color cap_col = (cap_sensor_found && m_cap_sense.is_touched()) ? gl::COLOR_GREEN : gl::COLOR_RED;
    gl::Color motion_col = (motion_sensor_found && m_motion_detected) ?
        gl::COLOR_GREEN : gl::COLOR_RED;
    gl::Color bio_col = (bio_sensor_found && *m_bio_score > *m_bio_thresh) ?
    gl::COLOR_GREEN : gl::COLOR_RED;
    
    // motion sensor
    gl::draw_text_2D("motion-sensor: " + ms_string, fonts()[0], motion_col, offset);
    
    // chair sensor
    offset += step;
    gl::draw_text_2D("chair-sensor: " + cs_string, fonts()[0], cap_col, offset);
    
    // bio feedback sensor
    offset += step;
    gl::draw_text_2D("bio-sensor (accelo): " + bs_string, fonts()[0], bio_col, offset);
    
    // LED device
    offset += step;
    gl::draw_text_2D("LEDs: " + led_string, fonts()[0], gl::COLOR_WHITE, offset);
    
    offset += gl::vec2(155, 0);
    gl::draw_quad(*m_led_color, gl::vec2(75.f), offset);
    
}

bool MeditationRoom::load_assets()
{
    // asset path
    const std::string asset_base_dir = "~/Desktop/mkb_assets";
    
    if(!fs::is_directory(asset_base_dir)){ return false; }
    
    auto audio_files = fs::get_directory_entries(asset_base_dir, fs::FileType::AUDIO, true);
    
    if(!audio_files.empty())
    {
        // background chanting audio
        m_audio->load(audio_files.front(), true, true);
    }
    
    auto video_files = fs::get_directory_entries(asset_base_dir, fs::FileType::MOVIE, true);
    
    if(!video_files.empty())
    {
        // description movie
        m_movie->load(video_files.front());
    }
    return true;
}

void MeditationRoom::create_animations()
{
    // setup animations
    animations()[AUDIO_FADE_IN] = animation::create(m_volume, m_volume->value(),
                                                    m_volume_max->value(), *m_timeout_fade);
    animations()[AUDIO_FADE_OUT] = animation::create(m_volume, m_volume->value(), 0.f,
                                                     *m_timeout_fade);
    animations()[LIGHT_FADE_IN] = animation::create(m_led_color, m_led_color->value(), gl::COLOR_WHITE,
                                                    *m_timeout_fade);
    animations()[LIGHT_FADE_OUT] = animation::create(m_led_color, m_led_color->value(), gl::COLOR_BLACK,
                                                     *m_timeout_fade);
}
