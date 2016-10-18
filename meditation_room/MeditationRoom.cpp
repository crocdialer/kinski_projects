//
//  MeditationRoom.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "gl/ShaderLibrary.h"
#include "MeditationRoom.hpp"

#define SENSOR_UPDATE_FREQUENCY 50.0
#define SERIAL_END_CODE '\n'

using namespace std;
using namespace kinski;
using namespace glm;

/////////////////////////////////////////////////////////////////

void MeditationRoom::setup()
{
    ViewerApp::setup();
    
    fonts()[1].load(fonts()[0].path(), 64);
    register_property(m_asset_dir);
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
    register_property(m_timeout_audio);
    register_property(m_timeout_movie_pause);
    register_property(m_timeout_meditation_cancel);
    register_property(m_duration_fade);
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
    
    // buffer incoming bytes from serial connection (used for bio-feedback)
    m_serial_read_buf.resize(1 << 16);
    
    m_motion_sensor.set_motion_callback([this](int v)
    {
        if(v > 0)
        {
            m_motion_detected = true;
            m_timer_motion_reset.expires_from_now(5.f);
            m_timer_idle.expires_from_now(*m_timeout_idle);
        }
    });
    
    gl::Shader rgb_shader;
    
#if defined(KINSKI_GLES)
    rgb_shader.loadFromData(unlit_vert, fs::read_file("rgb_shift_es2.frag"));
#else
    rgb_shader.loadFromData(unlit_vert, fs::read_file("rgb_shift.frag"));
#endif
    
    
    m_mat_rgb_shift = gl::Material::create(rgb_shader);
    m_mat_rgb_shift->set_depth_test(false);
    
    // generate animations
    create_animations();
    
    // setup timer objects
    m_timer_idle = Timer(main_queue().io_service(), [this](){ change_state(State::IDLE); });
    m_timer_audio_start = Timer(main_queue().io_service(), [this]()
    {
        m_audio->restart();
        animations()[AUDIO_FADE_IN]->start();
    });
    m_timer_motion_reset = Timer(main_queue().io_service(), [this](){ m_motion_detected = false; });
    
    // fade movie in/out
    auto fade_out_func = [this]()
    {
        animations()[PROJECTION_FADE_IN]->stop();
        
        create_animations();
        auto anim_cp = animations()[PROJECTION_FADE_OUT];
        anim_cp->start();
        
        animations()[PROJECTION_FADE_OUT]->set_finish_callback([this, anim_cp]()
        {
            change_state(State::MANDALA_ILLUMINATED);
        });
    };
    m_timer_movie_pause = Timer(main_queue().io_service(), fade_out_func);
    m_timer_meditation_cancel = Timer(main_queue().io_service(), fade_out_func);
    
    m_timer_sensor_update = Timer(background_queue().io_service(), [this]()
    {
        double now = getApplicationTime();
        double time_delta = now - m_sensor_read_timestamp;
        m_sensor_read_timestamp = now;
        
        // update sensor status
        m_motion_sensor.update(time_delta);
        m_cap_sense.update(time_delta);
        read_bio_sensor(time_delta);
    });
    m_timer_sensor_update.set_periodic();
    m_timer_sensor_update.expires_from_now(1.0 / SENSOR_UPDATE_FREQUENCY);
    
    // warp component
    m_warp = std::make_shared<WarpComponent>();
    m_warp->observe_properties();
    add_tweakbar_for_component(m_warp);
    
    // web interface
    remote_control().set_components({shared_from_this(), m_warp});
    load_settings();
    
    change_state(State::IDLE, true);
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
//    // update sensor status
//    m_motion_sensor.update(timeDelta);
//    m_cap_sense.update(timeDelta);
//    read_bio_sensor(timeDelta);
    
    // update according to current state
    switch (m_current_state)
    {
        case State::IDLE:
            if(m_motion_detected){ change_state(State::WELCOME); }
            break;
            
        case State::WELCOME:
            
            break;
            
        case State::MANDALA_ILLUMINATED:
            if(m_cap_sense.is_touched(12)){ change_state(State::DESC_MOVIE); }
            else if(*m_bio_score > *m_bio_thresh){ change_state(State::MEDITATION); }
            break;
            
        case State::DESC_MOVIE:
            if(m_movie)
            {
                m_movie->set_volume(*m_volume_max * m_brightness);
                m_movie->copy_frame_to_texture(textures()[TEXTURE_OUTPUT], true);
            }
            
            // keep on restarting the timer while touched
            if(m_cap_sense.is_touched(12))
            {
                m_timer_movie_pause.expires_from_now(*m_timeout_movie_pause);
                
                // movie began to fade out, but user wants to continue -> fade back in
                if(animations()[PROJECTION_FADE_OUT]->is_playing())
                {
                    animations()[PROJECTION_FADE_OUT]->stop();
                    
                    // create new animation object
                    create_animations();
                    animations()[PROJECTION_FADE_IN]->start();
                }
            }
            break;
            
        case State::MEDITATION:
            update_bio_visuals(median<float>(m_bio_acceleration), median<float>(m_bio_elongation));
            m_mat_rgb_shift->uniform("u_shift_amount", *m_shift_amount);
            m_mat_rgb_shift->uniform("u_shift_angle", *m_shift_angle);
            m_mat_rgb_shift->uniform("u_blur_amount", *m_blur_amount);
            m_mat_rgb_shift->uniform("u_window_dimension", *m_output_res);
            *m_shift_angle += *m_shift_velocity * timeDelta;
            
            // restart timer while people keep using chestbelt
            if(*m_bio_score > *m_bio_thresh)
            {
                m_timer_meditation_cancel.expires_from_now(*m_timeout_meditation_cancel);
            }
            break;
    }
    
    // ensure correct fbo status here
    set_fbo_state();
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::update_bio_visuals(float accel, float elong)
{
    // couple bioscore with meditation-parameters
    float val = map_value<float>(m_bio_sensitivity->value() * accel, 0.f, 10.f, 0.f, 1.f);
    val = glm::smoothstep(0.f, 1.f, val);
    
    // shiftamount
    *m_shift_amount = mix<float>(*m_shift_amount, val * 80.f, .15f);
    
    // shiftvelocity
    *m_shift_velocity = mix<float>(*m_shift_velocity, val * 20.f, .015f);
    
    // circle radius
    m_current_circ_radius = mix<float>(m_current_circ_radius,
                                       *m_circle_radius * (elong + 1.f + val * 2.f), .05f);
    
    // bluramount
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
    m_warp->render_output(textures()[TEXTURE_OUTPUT], m_brightness);
    
    if(Logger::get()->severity() >= Severity::DEBUG)
    {
        // draw status overlay
        draw_status_info();
    }
    
    if(displayTweakBar())
    {
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
    std::vector<State> states = {State::IDLE, State::WELCOME, State::MANDALA_ILLUMINATED,
        State::DESC_MOVIE, State::MEDITATION};
    
    if(!displayTweakBar())
    {
        switch (e.getCode())
        {
            case Key::_1:
            case Key::_2:
            case Key::_3:
            case Key::_4:
            case Key::_5:
                next_state = e.getCode() - Key::_1;
                break;
                
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
    
    if(theProperty == m_asset_dir)
    {
        if(!load_assets()){ LOG_ERROR << "could not load assets"; }
    }
    else if(theProperty == m_cap_sense_dev_name)
    {
        m_cap_sense.connect(*m_cap_sense_dev_name);
        m_cap_sense.set_thresholds(24, 8);
        m_cap_sense.set_charge_current(63);
    }
    else if(theProperty == m_motion_sense_dev_name)
    {
        m_motion_sensor.connect(*m_motion_sense_dev_name);
    }
    else if(theProperty == m_bio_sense_dev_name)
    {
        auto serial = Serial::create();
        
        if(!m_bio_sense_dev_name->value().empty())
        {
            serial->setup(*m_bio_sense_dev_name, 57600);
            
            if(serial->is_initialized()){ serial->flush(); }
        }
        m_bio_sense = serial;
    }
    else if(theProperty == m_led_dev_name)
    {
        auto serial = Serial::create();
        
        if(!m_led_dev_name->value().empty())
        {
            serial->setup(*m_led_dev_name, 57600);
            
            if(serial->is_initialized()){ serial->flush(); }
        }
        m_led_device = serial;
    }
    else if(theProperty == m_led_color){ set_led_color(*m_led_color); }
    else if(theProperty == m_volume)
    {
        if(m_audio){ m_audio->set_volume(*m_volume); }
    }
    else if(theProperty == m_duration_fade)
    {
        create_animations();
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
            ret = the_state == State::WELCOME;
            break;
        
        case State::WELCOME:
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
    if(!ret){ LOG_DEBUG << "invalid state change requested"; }
    if(force_change){ LOG_DEBUG << "forced state change"; }
    
    // create the fade in/out animations based on current values
    create_animations();
    
    if(ret || force_change)
    {
        textures()[TEXTURE_OUTPUT].reset();
        
        // handle state transition
        switch(the_state)
        {
            case State::IDLE:
                animations()[AUDIO_FADE_IN]->stop();
                animations()[AUDIO_FADE_OUT]->start();
                animations()[LIGHT_FADE_IN]->stop();
                animations()[LIGHT_FADE_OUT]->start();
                if(m_movie){ m_movie->restart(); m_movie->pause(); m_movie->set_volume(0); }
                break;
            
            case State::WELCOME:
                //TODO: implement
                main_queue().submit_with_delay([this]()
                {
                    change_state(State::MANDALA_ILLUMINATED);
                }, 2.0);
                break;
                
            case State::MANDALA_ILLUMINATED:
                animations()[AUDIO_FADE_IN]->stop();
                animations()[AUDIO_FADE_OUT]->start();
                animations()[LIGHT_FADE_OUT]->stop();
                animations()[LIGHT_FADE_IN]->start();
                if(m_movie){ m_movie->pause(); }
                else{ *m_volume = 0.f; }
                m_timer_idle.expires_from_now(*m_timeout_idle);
                m_timer_audio_start.expires_from_now(*m_timeout_audio);
                m_timer_meditation_cancel.cancel();
                break;
                
            case State::DESC_MOVIE:
                animations()[AUDIO_FADE_IN]->stop();
                animations()[AUDIO_FADE_OUT]->start();
                animations()[LIGHT_FADE_IN]->stop();
                animations()[LIGHT_FADE_OUT]->start();
                animations()[PROJECTION_FADE_OUT]->stop();
                animations()[PROJECTION_FADE_IN]->start();
                
                m_timer_idle.cancel();
                m_timer_audio_start.cancel();
                
                if(m_movie)
                {
                    m_movie->set_media_ended_callback([this](media::MovieControllerPtr)
                    {
                        change_state(State::MANDALA_ILLUMINATED);
                    });
                    
                    // jump back a few seconds, in case movie was paused
                    m_movie->seek_to_time(m_movie->current_time() - 3.0);
                    m_movie->play();
                    m_timer_movie_pause.expires_from_now(*m_timeout_movie_pause);
                }
                break;
                
            case State::MEDITATION:
                animations()[AUDIO_FADE_IN]->stop();
                animations()[AUDIO_FADE_OUT]->start();
                animations()[LIGHT_FADE_IN]->stop();
                animations()[LIGHT_FADE_OUT]->start();
                animations()[PROJECTION_FADE_OUT]->stop();
                animations()[PROJECTION_FADE_IN]->start();
                if(m_movie){ m_movie->pause(); }
                
                m_timer_idle.cancel();
                m_timer_audio_start.cancel();
                m_timer_meditation_cancel.expires_from_now(*m_timeout_meditation_cancel);
                
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
//        fmt.set_num_samples(8);
        m_fbos[0] = gl::Fbo(*m_output_res, fmt);
    }
    if(!m_fbos[1] || m_fbos[1].size() != m_output_res->value())
    {
        gl::Fbo::Format fmt;
//        fmt.set_num_samples(8);
        m_fbos[1] = gl::Fbo(*m_output_res, fmt);
    }
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::read_bio_sensor(float time_delta)
{
    m_last_sensor_reading += time_delta;
    std::string reading_str;
    
    // parse sensor input
    while(m_bio_sense->available())
    {
        size_t bytes_to_read = std::min(m_bio_sense->available(), m_serial_read_buf.size());
        uint8_t *buf_ptr = &m_serial_read_buf[0];
        
        m_bio_sense->read_bytes(&m_serial_read_buf[0], bytes_to_read);
        if(bytes_to_read){ m_last_sensor_reading = 0.f; }
        bool reading_complete = false;
        
        for(uint32_t i = 0; i < bytes_to_read; i++)
        {
            const uint8_t &byte = *buf_ptr++;
            
            switch(byte)
            {
                case SERIAL_END_CODE:
                    reading_str = string(m_serial_accumulator.begin(), m_serial_accumulator.end());
                    m_serial_accumulator.clear();
                    reading_complete = true;
                    break;
                    
                default:
                    m_serial_accumulator.push_back(byte);
                    break;
            }
        }
        
        if(reading_complete)
        {
            auto splits = split(reading_str, ' ');
            
            if(splits.size() == 2)
            {
                m_bio_acceleration.push(string_to<float>(splits[0]));
                m_bio_elongation.push(string_to<float>(splits[1]));
                *m_bio_score = median<float>(m_bio_acceleration);
                LOG_TRACE_1 << m_bio_score->value() << " - " <<  median<float>(m_bio_elongation);
            }
        }
    }

    // reconnect
    if((m_last_sensor_reading > m_sensor_timeout) && (m_sensor_timeout > 0.f))
    {
        m_last_sensor_reading = 0.f;
        m_bio_sense_dev_name->notify_observers();
    }
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::set_led_color(const gl::Color &the_color)
{
    if(m_led_device->is_initialized())
    {
        char buf[32];
        sprintf(buf, "%d %d %d %d\n", (int)std::round(the_color.r * 255),
                                      (int)std::round(the_color.g * 255),
                                      (int)std::round(the_color.b * 255),
                                      (int)std::round(the_color.a * 255));
        
        m_led_device->write(buf);
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
    
    bool motion_sensor_found = m_motion_sensor.is_initialized();
    bool cap_sensor_found = m_cap_sense.is_initialized();
    bool bio_sensor_found = m_bio_sense->is_initialized();
    bool led_device_found = m_led_device->is_initialized();
    
    string ms_string = motion_sensor_found ? "ok" : "not found";
    string cs_string = cap_sensor_found ? "ok" : "not found";
    string bs_string = bio_sensor_found ? to_string(m_bio_score->value(), 2) : "not found";
    string led_string = led_device_found ? "ok" : "not found";
    
    gl::Color cap_col = cap_sensor_found ?
        (m_cap_sense.is_touched() ? gl::COLOR_GREEN : gl::COLOR_WHITE) : gl::COLOR_RED;
    gl::Color motion_col = motion_sensor_found ?
        (m_motion_detected ? gl::COLOR_GREEN : gl::COLOR_WHITE) : gl::COLOR_RED;
    gl::Color bio_col = bio_sensor_found ?
        (*m_bio_score > *m_bio_thresh ? gl::COLOR_GREEN : gl::COLOR_WHITE) : gl::COLOR_RED;
    gl::Color led_str_col = led_device_found ? gl::COLOR_WHITE : gl::COLOR_RED;
    
    // motion sensor
    gl::draw_text_2D("motion-sensor: " + ms_string, fonts()[0], motion_col, offset);
    
    // chair sensor
    offset += step;
    gl::draw_text_2D("capacitive-sensor: " + cs_string, fonts()[0], cap_col, offset);
    
    // bio feedback sensor
    offset += step;
    gl::draw_text_2D("bio-feedback (accelo): " + bs_string, fonts()[0], bio_col, offset);
    
    // LED device
    offset += step;
    gl::draw_text_2D("LEDs: " + led_string, fonts()[0], led_str_col, offset);
    
    offset += gl::vec2(145, 0);
    gl::Color c(m_led_color->value().a); c.a = 1;
    gl::draw_quad(c, gl::vec2(50.f), offset);
    
}

bool MeditationRoom::load_assets()
{
    if(!fs::is_directory(*m_asset_dir)){ return false; }
    
    auto audio_files = fs::get_directory_entries(*m_asset_dir, fs::FileType::AUDIO, true);
    
    if(!audio_files.empty())
    {
        // background chanting audio
        m_audio->load(audio_files.front(), true, true);
        m_audio->set_on_load_callback([](media::MediaControllerPtr m){ m->set_volume(0); });
    }
    
    auto video_files = fs::get_directory_entries(*m_asset_dir, fs::FileType::MOVIE, true);
    
    if(!video_files.empty())
    {
        // description movie
        m_movie->load(video_files.front());
        m_movie->set_on_load_callback([](media::MediaControllerPtr m){ m->set_volume(0); });
    }
    return true;
}

void MeditationRoom::create_animations()
{
    // setup animations
    animations()[AUDIO_FADE_IN] = animation::create(m_volume, m_volume->value(),
                                                    m_volume_max->value(), *m_duration_fade);
    animations()[AUDIO_FADE_OUT] = animation::create(m_volume, m_volume->value(), 0.f,
                                                     *m_duration_fade);
    animations()[LIGHT_FADE_IN] = animation::create(m_led_color, m_led_color->value(), gl::Color(0, 0, 0, .4),
                                                    *m_duration_fade);
    animations()[LIGHT_FADE_OUT] = animation::create(m_led_color, m_led_color->value(), gl::Color(0),
                                                     *m_duration_fade);
    
    animations()[PROJECTION_FADE_IN] = animation::create(&m_brightness, m_brightness, 1.f,
                                                         *m_duration_fade);
    
    animations()[PROJECTION_FADE_OUT] = animation::create(&m_brightness, m_brightness, 0.f,
                                                          *m_duration_fade);
}
