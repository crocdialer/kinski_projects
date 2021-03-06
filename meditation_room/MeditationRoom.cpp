//
//  MeditationRoom.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "gl/ShaderLibrary.h"
#include "MeditationRoom.hpp"

#define SERIAL_END_CODE '\n'

using namespace std;
using namespace kinski;
using namespace glm;

/////////////////////////////////////////////////////////////////

namespace
{
    const double g_scan_for_device_interval = 3.0;
    const std::string g_led_device_id = "LED_CONTROL";
    const std::string g_bio_device_id = "BIO_FEEDBACK";
    std::function<void()> g_fade_out_func;
}

void MeditationRoom::setup()
{
    ViewerApp::setup();
    
    fonts()[1].load(fonts()[0].path(), 64);
    register_property(m_asset_dir);
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
    register_property(m_duration_movie_rewind);
    register_property(m_led_color);
    register_property(m_led_full_bright);
    register_property(m_spot_color_01);
    register_property(m_spot_color_02);
    register_property(m_volume);
    register_property(m_volume_max);
    register_property(m_bio_score);
    register_property(m_bio_thresh);
    register_property(m_bio_sensitivity_accel);
    register_property(m_bio_sensitivity_elong);
    register_property(m_cap_thresh);
    observe_properties();
    
    set_fbo_state();
    
    gl::ShaderPtr rgb_shader;
    
#if defined(KINSKI_GLES)
    rgb_shader = gl::Shader::create(unlit_vert, fs::read_file("rgb_shift_es2.frag"));
#else
    rgb_shader = gl::Shader::create(unlit_vert, fs::read_file("rgb_shift.frag"));
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
    g_fade_out_func = [this]()
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
    m_timer_movie_pause = Timer(main_queue().io_service(), g_fade_out_func);
    m_timer_meditation_cancel = Timer(main_queue().io_service(), g_fade_out_func);
    
    m_timer_cap_trigger = Timer(main_queue().io_service(), [this]()
    {
        m_cap_sense_activated = true;
    });
    
    m_timer_scan_for_device = Timer(main_queue().io_service(), [this](){ connect_devices(); });
    m_timer_scan_for_device.set_periodic();
    m_timer_scan_for_device.expires_from_now(g_scan_for_device_interval);
    
    // web interface
    remote_control().set_components({shared_from_this(), m_warp_component});
    load_settings();
    
    change_state(State::IDLE, true);
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::update(float timeDelta)
{
    ViewerApp::update(timeDelta);

    // construct ImGui window for this frame
    if(display_gui())
    {
        gui::draw_component_ui(shared_from_this());
        gui::draw_component_ui(m_warp_component);
    }

    // update according to current state
    switch (m_current_state)
    {
        case State::IDLE:
            if(m_motion_detected){ change_state(State::WELCOME); }
            break;
            
        case State::WELCOME:
            if(m_audio){ m_audio->set_volume(*m_volume_max); }
            break;
            
        case State::MANDALA_ILLUMINATED:
            if(m_cap_sense_activated)
            {
                change_state(State::DESC_MOVIE);
            }
            else
            {
                if(*m_bio_score > *m_bio_thresh){ change_state(State::MEDITATION); }
            }
                
            break;
            
        case State::DESC_MOVIE:
            if(m_movie)
            {
                m_movie->set_volume(*m_volume_max * m_brightness);
                m_movie->copy_frame_to_texture(textures()[TEXTURE_OUTPUT], true);
            }
            
            // keep on restarting the timer while touched
            if(m_cap_sense_activated)
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
            
            // restart timer while people keep using the chestbelt
            if(*m_bio_score > *m_bio_thresh)
            {
                m_timer_meditation_cancel.expires_from_now(*m_timeout_meditation_cancel);
            }
            
            if(m_audio)
            {
                m_audio->set_volume(*m_volume_max * *m_bio_score);
            }
            break;
    }
    
    if(m_led_needs_refresh)
    {
        set_led_color(*m_led_color);
        m_led_needs_refresh = false;
    }
    
    if(m_dmx_needs_refresh)
    {
        m_dmx_needs_refresh = false;
        
        m_dmx[1] = clamp<int>(m_spot_color_01->value().r * 255, 0, 255);
        m_dmx[2] = clamp<int>(m_spot_color_02->value().r * 255, 0, 255);
        m_dmx.update();
    }
    
    // ensure correct fbo status here
    set_fbo_state();
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::update_bio_visuals(float accel, float elong)
{
    // couple bioscore with meditation-parameters
    float val = map_value<float>(m_bio_sensitivity_accel->value() * accel, 0.f, 10.f, 0.f, 1.f);
    val = glm::smoothstep(0.f, 1.f, val);
    
    // shiftamount
    *m_shift_amount = mix<float>(*m_shift_amount, val * 80.f, .15f);
    
    // shiftvelocity
    *m_shift_velocity = mix<float>(*m_shift_velocity, val * 20.f, .015f);
    
    // circle radius
    m_current_circ_radius = mix<float>(m_current_circ_radius,
                                       *m_circle_radius * (m_bio_sensitivity_elong->value() * elong
                                                           + 1.f + val * 2.f), .05f);
    
    // bluramount
    *m_blur_amount = mix<float>(*m_blur_amount, 2.f + 120.f * val, .1f);
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::draw()
{
    gl::clear();

    if(m_current_state == State::MEDITATION)
    {
        // create undistorted offscreen tex
        textures()[0] = gl::render_to_texture(m_fbos[0], [this]()
        {
            gl::clear();
            gl::draw_circle(gl::window_dimension() / 2.f, m_current_circ_radius, gl::COLOR_WHITE,
                            true, 48);
        });
        m_mat_rgb_shift->add_texture(m_fbos[0]->texture());
        
        // apply distortion shader
        textures()[TEXTURE_OUTPUT] = gl::render_to_texture(m_fbos[1], [this]()
        {
            gl::clear();
            gl::draw_quad(gl::window_dimension(), m_mat_rgb_shift);
        });
    }
    
    // draw final result
    m_warp_component->render_output(0, textures()[TEXTURE_OUTPUT], m_brightness);
    
    if(Logger::get()->severity() >= Severity::DEBUG)
    {
        textures()[TEXTURE_DEBUG] = gl::render_to_texture(m_fbos[2], [this]()
        {
            gl::clear_color(gl::Color(0));
            gl::clear();
            draw_status_info();
            gl::clear_color(clear_color());
        });
        
        // draw status overlay
        m_warp_component->render_output(1, textures()[TEXTURE_DEBUG]);
    }
    
    if(display_gui())
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

void MeditationRoom::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);
    
    int next_state = -1;
    std::vector<State> states = {State::IDLE, State::WELCOME, State::MANDALA_ILLUMINATED,
        State::DESC_MOVIE, State::MEDITATION};
    
    if(!display_gui())
    {
        switch (e.code())
        {
            case Key::_1:
            case Key::_2:
            case Key::_3:
            case Key::_4:
            case Key::_5:
                next_state = e.code() - Key::_1;
                break;
            case Key::_P:
                if(m_current_state == State::DESC_MOVIE && m_movie)
                {
                    m_movie->is_playing() ? m_movie->pause() : m_movie->play();
                }
                break;
            default:
                break;
        }
    }
    
    if(next_state >= 0){ change_state(states[next_state], e.is_shift_down()); }
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files)
    {
//        LOG_INFO << f;
        *m_asset_dir = files.front();
    }
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::teardown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
    
    if(theProperty == m_asset_dir)
    {
        m_assets_found = load_assets();
        if(!m_assets_found){ LOG_ERROR << "could not load assets"; }
    }
    else if(theProperty == m_cap_thresh)
    {
        m_cap_sense->set_thresholds(*m_cap_thresh, *m_cap_thresh * 0.9);
        m_cap_sense->set_charge_current(63);
    }
    else if(theProperty == m_led_color){ m_led_needs_refresh = true; }
    else if(theProperty == m_volume)
    {
        if(m_audio){ m_audio->set_volume(*m_volume); }
    }
    else if(theProperty == m_duration_fade)
    {
        create_animations();
    }
    else if(theProperty == m_circle_radius){ m_current_circ_radius = *m_circle_radius; }
    else if(theProperty == m_spot_color_01 || theProperty == m_spot_color_02)
    {
        m_dmx_needs_refresh = true;
    }
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
    
    if(ret || force_change)
    {
        // create the fade in/out animations based on current values
        create_animations();
        
        textures()[TEXTURE_OUTPUT].reset();
        
        // handle state transition
        switch(the_state)
        {
            case State::IDLE:
                animations()[AUDIO_FADE_IN]->stop();
                animations()[AUDIO_FADE_OUT]->start();
                animations()[LIGHT_FADE_IN]->stop();
                animations()[LIGHT_FADE_OUT]->start();
                animations()[SPOT_01_FADE_IN]->stop();
                animations()[SPOT_01_FADE_OUT]->start();
                animations()[SPOT_02_FADE_IN]->stop();
                animations()[SPOT_02_FADE_OUT]->start();
                
                animations()[AUDIO_FADE_OUT]->set_finish_callback([this]()
                {
                    if(m_movie){ m_movie->seek_to_time(0); LOG_DEBUG << "m_movie->seek_to_time(0)"; }
                });
                break;
            
            case State::WELCOME:
                
                if(m_assets_found)
                {
                    m_audio->set_on_load_callback([this](media::MediaControllerPtr m)
                    {
                        m->set_volume(*m_volume_max);
                    });
                    m_audio->load(m_audio_paths[AUDIO_WELCOME], true, false);
                    
                    m_audio->set_media_ended_callback([this](media::MediaControllerPtr m)
                    {
                        main_queue().post([this]() { change_state(State::MANDALA_ILLUMINATED); });
                    });
                    
                    // start with delays
                    animations()[LIGHT_FADE_OUT]->stop();
                    animations()[SPOT_01_FADE_OUT]->stop();
                    animations()[SPOT_02_FADE_OUT]->stop();
                    animations()[LIGHT_FADE_IN]->start(7.5f);
                    animations()[SPOT_01_FADE_IN]->start(14.5f);
//                    animations()[SPOT_02_FADE_IN]->start(24.f);
                }
                break;
                
            case State::MANDALA_ILLUMINATED:
                
                // load background chanting audio
                if(m_assets_found)
                {
                    m_audio->load(m_audio_paths[AUDIO_CHANTING], true, true);
                    m_audio->set_on_load_callback([](media::MediaControllerPtr m){ m->set_volume(0); });
                }
                animations()[AUDIO_FADE_IN]->stop();
                animations()[AUDIO_FADE_OUT]->start();
                animations()[LIGHT_FADE_OUT]->stop();
                animations()[LIGHT_FADE_IN]->start();
                animations()[SPOT_01_FADE_OUT]->stop();
                animations()[SPOT_01_FADE_IN]->start();
                animations()[SPOT_02_FADE_OUT]->stop();
                animations()[SPOT_02_FADE_IN]->start();
                animations()[PROJECTION_FADE_IN]->stop();
                animations()[PROJECTION_FADE_OUT]->start();
                if(m_movie){ m_movie->pause(); }
                else{ *m_volume = 0.f; }
                m_timer_idle.expires_from_now(*m_timeout_idle);
                m_timer_audio_start.expires_from_now(*m_timeout_audio);
                m_timer_meditation_cancel.cancel();
                break;
                
            case State::DESC_MOVIE:
                
                animations()[AUDIO_FADE_IN]->stop();
                animations()[AUDIO_FADE_OUT]->start();
                animations()[LIGHT_FADE_IN]->start();
                animations()[LIGHT_FADE_OUT]->stop();
                animations()[PROJECTION_FADE_OUT]->stop();
                main_queue().post_with_delay([this]()
                                             {
                                                 animations()[PROJECTION_FADE_IN]->start();
                                             }, (m_movie && m_movie->current_time() > 0.0) ? .4 : 0);
                animations()[SPOT_01_FADE_IN]->stop();
                animations()[SPOT_01_FADE_OUT]->start();
                animations()[SPOT_02_FADE_IN]->stop();
                animations()[SPOT_02_FADE_OUT]->start();
                
                m_timer_idle.cancel();
                m_timer_audio_start.cancel();
                
                if(m_movie)
                {
                    m_movie->set_media_ended_callback([this](media::MediaControllerPtr mc)
                    {
                        LOG_DEBUG << "movie ended";
                        m_timer_movie_pause.cancel();
                        m_movie->pause();

                        main_queue().post_with_delay([this]()
                                                     {
                                                         g_fade_out_func();
                                                     }, 5.0);
                    });
                    m_movie->play();
                    m_timer_movie_pause.expires_from_now(*m_timeout_movie_pause);
                    
                    // jump back a few seconds, in case movie was paused
                    if(m_movie->current_time() > *m_duration_movie_rewind &&
                       *m_duration_movie_rewind > 0.f)
                    {
                        m_movie->seek_to_time(m_movie->current_time() - *m_duration_movie_rewind);
                    }
                }
                break;
                
            case State::MEDITATION:
                
                // load background chanting audio
                if(m_assets_found)
                {
                    m_audio->load(m_audio_paths[AUDIO_WIND], true, true);
                    m_audio->set_on_load_callback([](media::MediaControllerPtr m){ m->set_volume(0); });
                }
                
                animations()[AUDIO_FADE_IN]->stop();
                animations()[AUDIO_FADE_OUT]->start();
                animations()[LIGHT_FADE_IN]->stop();
                animations()[LIGHT_FADE_OUT]->start();
                animations()[PROJECTION_FADE_OUT]->stop();
                animations()[PROJECTION_FADE_IN]->start();
                animations()[SPOT_01_FADE_IN]->stop();
                animations()[SPOT_01_FADE_OUT]->start();
                animations()[SPOT_02_FADE_IN]->stop();
                animations()[SPOT_02_FADE_OUT]->start();

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
    m_fbos.resize(3);
    
    if(!m_fbos[0] || gl::vec2(m_fbos[0]->size()) != m_output_res->value())
    {
        gl::Fbo::Format fmt;
//        fmt.set_num_samples(8);
        m_fbos[0] = gl::Fbo::create(*m_output_res, fmt);
    }
    if(!m_fbos[1] || gl::vec2(m_fbos[1]->size()) != m_output_res->value())
    {
        gl::Fbo::Format fmt;
//        fmt.set_num_samples(8);
        m_fbos[1] = gl::Fbo::create(*m_output_res, fmt);
    }
    
    gl::vec2 info_sz = gl::vec2(460, 300);
    
    if(!m_fbos[2] || gl::vec2(m_fbos[2]->size()) != info_sz)
    {
        gl::Fbo::Format fmt;
        m_fbos[2] = gl::Fbo::create(info_sz, fmt);
    }
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::sensor_touch(int the_pad_index)
{
    if(m_timer_cap_trigger.has_expired())
    {
        m_timer_cap_trigger.expires_from_now(.4);
    }
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::sensor_release(int the_pad_index)
{
    // reset cap sense indicator
    if(!m_cap_sense->is_touched())
    {
        m_timer_cap_trigger.cancel();
        m_cap_sense_activated = false;
    }
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::read_bio_sensor(ConnectionPtr the_uart, const std::vector<uint8_t> &data)
{
    std::string reading_str;
    
    for(auto byte : data)
    {
        switch(byte)
        {
            case SERIAL_END_CODE:
                reading_str = string(m_serial_accumulator.begin(), m_serial_accumulator.end());
                m_serial_accumulator.clear();
                break;
                
            default:
                m_serial_accumulator.push_back(byte);
                break;
        }
    }
    
    if(!reading_str.empty())
    {
        auto splits = split(reading_str, ' ');
        
        if(splits.size() == 2)
        {
            main_queue().post([this, splits]()
                              {
                                  m_bio_acceleration.push_back(string_to<float>(splits.front()));
                                  m_bio_elongation.push_back(string_to<float>(splits.back()));
                                  *m_bio_score = median<float>(m_bio_acceleration) + median<float>(m_bio_elongation);
                                  LOG_TRACE_1 << m_bio_score->value() << " - " << median<float>(m_bio_elongation);
                              });
        }
    }
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::set_led_color(const gl::Color &the_color)
{
    if(m_led_device && m_led_device->is_open())
    {
        char buf[32];
        int num_bytes = sprintf(buf, "%d %d %d %d\n", (int)std::round(the_color.r * 255),
                                (int)std::round(the_color.g * 255),
                                (int)std::round(the_color.b * 255),
                                (int)std::round(the_color.a * 255));
//        int num_bytes = sprintf(buf, "%d\n", (int)std::round(the_color.a * 255));
        m_led_device->write_bytes(buf, num_bytes);
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
            to_string(m_movie->duration(), 1) + ")(" +
            to_string(m_timer_movie_pause.expires_from_now(), 1) + ")";
    }
    else if((m_current_state == State::WELCOME) && m_audio)
    {
        state_str += " (" + to_string(m_audio->current_time(),1) + "/" +
        to_string(m_audio->duration(), 1) + ")";
    }
    gl::draw_text_2D(state_str, fonts()[0], gl::COLOR_WHITE, offset);
    offset += step;
    
    bool motion_sensor_found = m_motion_sensor->is_initialized();
    bool cap_sensor_found = m_cap_sense->is_initialized();
    bool bio_sensor_found = m_bio_sense && m_bio_sense->is_open();
    bool led_device_found = m_led_device && m_led_device->is_open();
    bool dmx_device_found = m_dmx.is_initialized();
    
    string ms_string = motion_sensor_found ? to_string(m_motion_sensor->distance() / 1000, 1) : "not found";
    string cs_string = cap_sensor_found ? "ok" + string(m_cap_sense_activated ? "*" : "") : "not found";
    string bs_string = bio_sensor_found ? to_string(m_bio_score->value(), 2) : "not found";
    string led_string = led_device_found ? "ok" : "not found";
    string dmx_string = dmx_device_found ? "ok" : "not found";
    
    gl::Color cap_col = cap_sensor_found ?
        (m_cap_sense->is_touched() ? gl::COLOR_GREEN : gl::COLOR_WHITE) : gl::COLOR_RED;
    gl::Color motion_col = motion_sensor_found ?
        (m_motion_detected ? gl::COLOR_GREEN : gl::COLOR_WHITE) : gl::COLOR_RED;
    gl::Color bio_col = bio_sensor_found ?
        (*m_bio_score > *m_bio_thresh ? gl::COLOR_GREEN : gl::COLOR_WHITE) : gl::COLOR_RED;
    gl::Color led_str_col = led_device_found ? gl::COLOR_WHITE : gl::COLOR_RED;
    gl::Color dmx_str_col = dmx_device_found ? gl::COLOR_WHITE : gl::COLOR_RED;
    
    // motion sensor
    gl::draw_text_2D("motion-sensor: " + ms_string, fonts()[0], motion_col, offset);
    
    // chair sensor
    offset += step;
    gl::draw_text_2D("capacitive-sensor: " + cs_string, fonts()[0], cap_col, offset);
    
    // bio feedback sensor
    offset += step;
    gl::draw_text_2D("bio-feedback (chestbelt): " + bs_string, fonts()[0], bio_col, offset);
    
    // dmx control
    offset += step;
    gl::draw_text_2D("dmx: " + dmx_string, fonts()[0], dmx_str_col, offset);
    
    // LED device
    offset += step;
    gl::draw_text_2D("LEDs: " + led_string, fonts()[0], led_str_col, offset);
    
    offset += gl::vec2(145, 0);
    gl::Color c(m_led_color->value().a); c.a = 1;
    gl::draw_quad(gl::vec2(50.f), c, offset);
    
}

bool MeditationRoom::load_assets()
{
    if(!fs::is_directory(*m_asset_dir)){ return false; }
    
    auto audio_files = fs::get_directory_entries(*m_asset_dir, fs::FileType::AUDIO, true);
    
    bool ret = true;
    
    if(audio_files.size() == 3)
    {
        m_audio_paths.assign(audio_files.begin(), audio_files.end());
        std::sort(m_audio_paths.begin(), m_audio_paths.end());
        
    }else{ LOG_WARNING << "found " << audio_files.size() << " audio-files, expected 3"; ret = false; }
    
    auto video_files = fs::get_directory_entries(*m_asset_dir, fs::FileType::MOVIE, true);
    
    if(!video_files.empty())
    {
        // description movie
        m_movie->set_on_load_callback([](media::MediaControllerPtr m){ m->set_volume(0); });
        m_movie->load(video_files.front(), false, true);
    }
    else{ ret = false; }
    return ret;
}

void MeditationRoom::create_animations()
{
    // setup animations
    animations()[AUDIO_FADE_IN] = animation::create(m_volume, m_volume->value(),
                                                    m_volume_max->value(), *m_duration_fade);
    animations()[AUDIO_FADE_OUT] = animation::create(m_volume, m_volume->value(), 0.f,
                                                     *m_duration_fade);
    animations()[LIGHT_FADE_IN] = animation::create(m_led_color, m_led_color->value(),
                                                    m_led_full_bright->value(), *m_duration_fade);
    animations()[LIGHT_FADE_OUT] = animation::create(m_led_color, m_led_color->value(), gl::Color(0),
                                                     *m_duration_fade);
    
    animations()[PROJECTION_FADE_IN] = animation::create(&m_brightness, m_brightness, 1.f,
                                                         *m_duration_fade);
    
    animations()[PROJECTION_FADE_OUT] = animation::create(&m_brightness, m_brightness, 0.f,
                                                          *m_duration_fade);
    animations()[PROJECTION_FADE_OUT]->set_finish_callback([this]()
    {
        textures()[TEXTURE_OUTPUT].reset();
    });
    
    animations()[SPOT_01_FADE_IN] = animation::create(m_spot_color_01, m_spot_color_01->value(),
                                                      gl::COLOR_WHITE, *m_duration_fade);
    animations()[SPOT_01_FADE_OUT] = animation::create(m_spot_color_01, m_spot_color_01->value(),
                                                       gl::COLOR_BLACK, *m_duration_fade);
    animations()[SPOT_02_FADE_IN] = animation::create(m_spot_color_02, m_spot_color_02->value(),
                                                      gl::COLOR_WHITE, *m_duration_fade);
    animations()[SPOT_02_FADE_OUT] = animation::create(m_spot_color_02, m_spot_color_02->value(),
                                                      gl::COLOR_BLACK, *m_duration_fade);
}

void MeditationRoom::connect_devices()
{
    sensors::scan_for_devices(background_queue().io_service(),
                              [this](const std::string &the_id, ConnectionPtr the_uart)
    {
        std::vector<std::string> device_ids =
        {
            CapacitiveSensor::id(),
            DistanceSensor::id(),
            g_led_device_id,
            g_bio_device_id
        };
        
        if(contains(device_ids, the_id))
        {
            LOG_TRACE_1 << "discovered device: <" << the_id << "> (" << the_uart->description() << ")";

            main_queue().post([this, the_id, the_uart]
                              {
                                  if(the_id == CapacitiveSensor::id() && !m_cap_sense->is_initialized())
                                  {
                                      m_cap_sense->connect(the_uart);
                                      m_cap_sense->set_thresholds(*m_cap_thresh, *m_cap_thresh * 0.9);
                                      m_cap_sense->set_charge_current(63);

                                      m_cap_sense->set_touch_callback(std::bind(&MeditationRoom::sensor_touch,
                                                                                this, std::placeholders::_1));
                                      m_cap_sense->set_release_callback(std::bind(&MeditationRoom::sensor_release,
                                                                                  this, std::placeholders::_1));
                                      the_uart->set_disconnect_cb([this](ConnectionPtr the_uart)
                                                                  {
                                                                      m_cap_sense->connect(nullptr);
                                                                      m_timer_scan_for_device.expires_from_now(
                                                                              g_scan_for_device_interval);
                                                                  });

                                  }else if(the_id == DistanceSensor::id() && !m_motion_sensor->is_initialized())
                                  {
                                      m_motion_sensor->connect(the_uart);
                                      m_motion_sensor->set_distance_callback([this](int v)
                                                                             {
                                                                                 if(v > 0)
                                                                                 {
                                                                                     m_motion_detected = true;
                                                                                     m_timer_motion_reset.expires_from_now(
                                                                                             5.f);
                                                                                     m_timer_idle.expires_from_now(
                                                                                             *m_timeout_idle);
                                                                                 }
                                                                             });
                                      the_uart->set_disconnect_cb([this](ConnectionPtr the_uart)
                                                                  {
                                                                      m_motion_sensor->connect(nullptr);
                                                                      m_timer_scan_for_device.expires_from_now(
                                                                              g_scan_for_device_interval);
                                                                  });
                                  }else if(the_id == g_led_device_id && !(m_led_device && m_led_device->is_open()))
                                  {
                                      m_led_device = the_uart;
                                      m_led_device->write("0\n");
                                      the_uart->set_disconnect_cb([this](ConnectionPtr the_uart)
                                                                  {
                                                                      m_led_device.reset();
                                                                      m_timer_scan_for_device.expires_from_now(
                                                                              g_scan_for_device_interval);
                                                                  });
                                  }else if(the_id == g_bio_device_id && !(m_bio_sense && m_bio_sense->is_open()))
                                  {
                                      m_bio_sense = the_uart;
                                      m_bio_sense->set_receive_cb(std::bind(&MeditationRoom::read_bio_sensor,
                                                                            this,
                                                                            std::placeholders::_1,
                                                                            std::placeholders::_2));
                                      the_uart->set_disconnect_cb([this](ConnectionPtr the_uart)
                                                                  {
                                                                      m_bio_sense.reset();
                                                                      m_timer_scan_for_device.expires_from_now(
                                                                              g_scan_for_device_interval);
                                                                  });
                                  }

                                  auto is_complete = [this]() -> bool
                                  {
                                      return m_cap_sense->is_initialized() && m_motion_sensor->is_initialized() &&
                                             m_led_device && m_led_device->is_open();
                                      //                   && m_dmx.is_initialized;
                                      //                   && m_bio_sense && m_bio_sense->is_open()
                                  };

                                  if(is_complete())
                                  {
                                      LOG_DEBUG << "sensor setup complete";
                                      m_timer_scan_for_device.cancel();
                                  }
                              });
        }
    });
}
