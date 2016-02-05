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
    
    fonts()[1].load("Courier New Bold.ttf", 64);
    
    register_property(m_cap_sense_dev_name);
    register_property(m_motion_sense_dev_name);
    register_property(m_bio_sense_dev_name);
    register_property(m_led_dev_name);
    register_property(m_output_res);
    register_property(m_circle_radius);
    register_property(m_shift_angle);
    register_property(m_shift_amount);
    register_property(m_blur_amount);
    register_property(m_timeout_idle);
    register_property(m_timeout_movie_start);
    register_property(m_led_color);
    
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
    m_timer_motion_reset = Timer(io_service(), [this](){ m_motion_detected = false; });
    m_timer_movie_start = Timer(io_service(), [this](){ if(m_movie){ m_movie->restart(); } });
    
    // warp component
    m_warp = std::make_shared<WarpComponent>();
    m_warp->observe_properties();
    add_tweakbar_for_component(m_warp);
    
    // output window
    auto output_window = GLFW_Window::create(1280, 720, "output", false, 0, windows().back()->handle());
    add_window(output_window);
    
    if(!load_assets()){ LOG_ERROR << "could not load assets"; }
    load_settings();
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    // update motion sensor status
    detect_motion();
    
    m_cap_sense.update(timeDelta);
    
    // update according to current state
    
    switch (m_current_state)
    {
        case State::IDLE:
            textures()[TEXTURE_OUTPUT].reset();
            if(m_audio){ m_audio->set_volume(1.f); }
            *m_led_color = gl::COLOR_BLACK;
            
            if(m_motion_detected){ change_state(State::MANDALA_ILLUMINATED); }
            break;
            
        case State::MANDALA_ILLUMINATED:
            textures()[TEXTURE_OUTPUT].reset();
            
            //TODO: fade in here
            *m_led_color = gl::COLOR_WHITE;
            
            if(m_audio){ m_audio->set_volume(0.f); }
            
            if(m_cap_sense.is_touched(12))
            {
                change_state(State::DESC_MOVIE);
            }
            break;
            
        case State::DESC_MOVIE:
            //TODO: fade out here
            *m_led_color = gl::COLOR_BLACK;
            
            if(m_movie && !m_movie->isPlaying() && m_timer_movie_start.has_expired())
            {
                LOG_DEBUG << "starting movie in " << m_timeout_movie_start->value() <<" secs";
                m_timer_movie_start.expires_from_now(*m_timeout_movie_start);
                m_movie->set_movie_ended_callback([this](video::MovieControllerPtr)
                {
                    change_state(State::MANDALA_ILLUMINATED);
                });
            }
            break;
            
        case State::MEDITATION:
            //TODO: fade out here
            *m_led_color = gl::COLOR_BLACK;
            if(m_movie){ m_movie->pause(); }
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
    if(m_current_state == State::DESC_MOVIE)
    {
        if(m_movie && m_movie->copy_frame_to_texture(textures()[TEXTURE_OUTPUT], true)){}
    }
    else if(m_current_state == State::MEDITATION)
    {
        // create undistorted offscreen tex
        textures()[0] = gl::render_to_texture(m_fbos[0], [this]()
        {
            gl::clear();
            gl::drawCircle(gl::windowDimension() / 2.f, *m_circle_radius, true, 48);
        });
        
        // apply distortion shader
        textures()[TEXTURE_OUTPUT] = gl::render_to_texture(m_fbos[1], [this]()
        {
            gl::clear();
            gl::drawQuad(m_mat_rgb_shift, gl::windowDimension());
        });
    }
    output_switch();
    
    // draw final result
    gl::drawTexture(textures()[TEXTURE_OUTPUT], gl::windowDimension());
    
    // draw status overlay
    draw_status_info();
    
    if(displayTweakBar()){ draw_textures(textures()); }
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
    static std::vector<State> states = {State::IDLE, State::MANDALA_ILLUMINATED, State::DESC_MOVIE,
        State::MEDITATION};
    
    switch (e.getCode())
    {
        case KEY_1:
        case KEY_2:
        case KEY_3:
        case KEY_4:
            next_state = e.getCode() - KEY_1;
            break;
        
        case KEY_O:
        {
            // output window
            auto output_window = GLFW_Window::create(1280, 720, "output", false, 0,
                                                     windows().back()->handle());
            add_window(output_window);
            output_switch();
        }
            break;
            
        default:
            break;
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
        if(!m_motion_sense_dev_name->value().empty())
        {
            m_motion_sense.setup(*m_motion_sense_dev_name, 57600);
            
            // finally flush the newly initialized device
            if(m_motion_sense.isInitialized()){ m_motion_sense.flush(); }
        }
    }
    else if(theProperty == m_bio_sense_dev_name)
    {
        if(!m_bio_sense_dev_name->value().empty())
        {
            m_bio_sense.setup(*m_bio_sense_dev_name, 57600);
            
            // finally flush the newly initialized device
            if(m_bio_sense.isInitialized()){ m_bio_sense.flush(); }
        }
    }
    else if(theProperty == m_led_dev_name)
    {
        if(!m_led_dev_name->value().empty())
        {
            m_led_device.setup(*m_led_dev_name, 57600);
            
            // finally flush the newly initialized device
            if(m_led_device.isInitialized()){ m_led_device.flush(); }
        }
    }
    else if(theProperty == m_led_color){ set_led_color(*m_led_color); }
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
    
    if(ret || force_change)
    {
        m_current_state = the_state;
        textures()[TEXTURE_OUTPUT].reset();
        
        //TODO: handle state transition
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
    if(!m_fbos[1] || m_fbos[1].getSize() != m_output_res->value())
    {
        gl::Fbo::Format fmt;
        fmt.setSamples(16);
        m_fbos[1] = gl::Fbo(*m_output_res, fmt);
    }
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::detect_motion()
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

void MeditationRoom::read_bio_sensor()
{
    
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::set_led_color(const gl::Color &the_color)
{
    if(m_led_device.isInitialized())
    {
        // create RGBA integer val
        uint32_t rgba_val = (static_cast<uint32_t>(the_color.r * 255) << 24) |
        (static_cast<uint32_t>(the_color.g * 255) << 16) |
        (static_cast<uint32_t>(the_color.b * 255) << 8) |
        static_cast<uint32_t>(the_color.a * 255);
        
        m_led_device.writeBytes(&rgba_val, sizeof(rgba_val));
    }
    
}

/////////////////////////////////////////////////////////////////

void MeditationRoom::draw_status_info()
{
    vec2 offset(50, 50), step(0, 35);
    
    // display current state
    gl::drawText2D("State: " + m_state_string_map[m_current_state], fonts()[0], gl::COLOR_WHITE, offset);
    offset += step;
    
    bool motion_sensor_found = m_motion_sense.isInitialized();
    bool cap_sensor_found = m_cap_sense.is_initialized();
    bool bio_sensor_found = m_bio_sense.isInitialized();
    bool led_device_found = m_led_device.isInitialized();
    
    string ms_string = motion_sensor_found ? "ok" : "not found";
    string cs_string = cap_sensor_found ? "ok" : "not found";
    string bs_string = bio_sensor_found ? "ok" : "not found";
    string led_string = led_device_found ? "ok" : "not found";
    
    gl::Color cap_col = (cap_sensor_found && m_cap_sense.is_touched(12)) ? gl::COLOR_GREEN : gl::COLOR_RED;
    gl::Color motion_col = (motion_sensor_found && m_motion_detected) ?
        gl::COLOR_GREEN : gl::COLOR_RED;
    
    // motion sensor
    gl::drawText2D("motion-sensor: " + ms_string, fonts()[0], motion_col, offset);
    
    // chair sensor
    offset += step;
    gl::drawText2D("chair-sensor: " + cs_string, fonts()[0], cap_col, offset);
    
    // bio feedback sensor
    offset += step;
    gl::drawText2D("bio-sensor (accelo): " + bs_string, fonts()[0], gl::COLOR_WHITE, offset);
    
    // LED device
    offset += step;
    gl::drawText2D("LEDs: " + led_string, fonts()[0], gl::COLOR_WHITE, offset);
    
    offset += gl::vec2(155, 0);
    gl::drawQuad(*m_led_color, gl::vec2(75.f), offset);
    
}

bool MeditationRoom::load_assets()
{
    // asset path
    const std::string asset_base_dir = "~/Desktop/mkb_assets";
    
    if(!is_directory(asset_base_dir)){ return false; }
    
    // background chanting audio
    m_audio.reset(new audio::Fmod_Sound(join_paths(asset_base_dir, "chanting_loop.wav")));
    m_audio->set_loop(true);
    m_audio->play();
    
    // description movie
    m_movie->load(join_paths(asset_base_dir, "mandala.mov"));
    
    return true;
}

void MeditationRoom::output_switch()
{
    if(windows().size() > 1)
    {
        auto tex = textures()[TEXTURE_OUTPUT];
        windows().back()->set_draw_function([this, tex]()
        {
            gl::clear();
            
            static auto mat = gl::Material::create();
            gl::apply_material(mat);
            
            gl::drawTexture(tex, gl::windowDimension());
            
//            m_warp->quad_warp().render_output(tex);
//            m_warp->quad_warp().render_grid();
//            m_warp->quad_warp().render_control_points();
        });
    }
}