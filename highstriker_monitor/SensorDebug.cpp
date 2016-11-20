//
//  SensorDebug.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "SensorDebug.hpp"

using namespace std;
using namespace kinski;
using namespace glm;

#define SERIAL_END_CODE '\n'

/////////////////////////////////////////////////////////////////

void SensorDebug::setup()
{
    ViewerApp::setup();

    auto font_path = fonts()[0].path();

    fonts()[FONT_MEDIUM].load(font_path, 32);
    fonts()[FONT_LARGE].load(font_path, 64);

    register_property(m_device_name_sensor);
    register_property(m_device_name_nixie);
    register_property(m_device_name_dmx);
    register_property(m_score_min);
    register_property(m_score_max);
    register_property(m_range_min_max);
    register_property(m_force_multiplier);
    register_property(m_timeout_game_ready);
    register_property(m_duration_impact_measure);
    register_property(m_duration_count_score);
    register_property(m_duration_display_score);
    register_property(m_sensor_hist_size);
    register_property(m_sensor_refresh_rate);

    observe_properties();
    add_tweakbar_for_component(shared_from_this());

    // buffer incoming bytes from serial connection
    m_serial_read_buf.resize(2048);
    
    // use this for line drawing
    m_line_mesh = gl::Mesh::create(gl::Geometry::create(),
                                   gl::Material::create(gl::create_shader(gl::ShaderType::LINES_2D)));
    m_line_mesh->geometry()->set_primitive_type(GL_LINES);
    m_line_mesh->material()->set_depth_test(false);
//    m_line_mesh->material()->setTwoSided();
    
    // setup a recurring timer for sensor-refresh-rate measurement
    m_timer_sensor_refresh = Timer(main_queue().io_service(), [this]()
    {
        *m_sensor_refresh_rate = m_sensor_refresh_count;
        m_sensor_refresh_count = 0;
    });
    m_timer_sensor_refresh.set_periodic();
    m_timer_sensor_refresh.expires_from_now(1.f);

    m_timer_game_ready = Timer(main_queue().io_service(), [this](){ change_gamestate(IDLE); });
    m_timer_impact_measure = Timer(main_queue().io_service(), [this]()
    {
        m_current_value = m_sensor_last_max;
        change_gamestate(IMPACT);
    });

    if(!load_settings()){ save_settings(); }
    
    remote_control().set_components({shared_from_this()});
    
    // start in IDLE state
    change_gamestate(IDLE);
}

/////////////////////////////////////////////////////////////////

void SensorDebug::update(float timeDelta)
{
    ViewerApp::update(timeDelta);

    // fetch new measurements
    update_sensor_values(timeDelta);

    if(m_timer_impact_measure.has_expired()){ process_impact(m_sensor_last_max); }

    m_dmx.update(timeDelta);
    
    // send current value to nixies
    update_score_display();
}

/////////////////////////////////////////////////////////////////

void SensorDebug::draw()
{
    gl::set_matrices_for_window();
    
    // draw debug UI
    vec2 offset(55, 110), step(0, 90);

    float h = 80.f, w = gl::window_dimension().x - 2.f * offset.x;

    for(size_t i = 0; i < m_measurements.size(); i++)
    {
        float val = m_measurements[i].back();

        // rectangle for current value
        gl::draw_quad(gl::COLOR_GRAY, vec2(val * w, h), offset);

        gl::draw_text_2D(to_string(100.f * val, 1) + "%", fonts()[FONT_MEDIUM], gl::COLOR_WHITE,
                         offset + vec2(val * w, 0));

        ////////////////////////////// measure history ///////////////////////////

        // generate point array for bars
        auto &verts = m_line_mesh->geometry()->vertices();
        auto &colors = m_line_mesh->geometry()->colors();
        
        verts.resize(m_measurements[i].size() * 2);
        colors.resize(verts.size(), gl::COLOR_WHITE);
        m_line_mesh->geometry()->tex_coords().resize(verts.size(), gl::vec2(0));
        
        for (size_t j = 0, sz = verts.size(); j < sz; j += 2)
        {
            float x_val = offset.x + j / (float) sz * w;
            float y_val = gl::window_dimension().y - offset.y - h;
            
            float hist_val = m_measurements[i][j / 2];
            verts[sz - 1 - j] = vec3(x_val, y_val, 0.f);
            verts[sz - 2 - j] = vec3(x_val, y_val + std::max(h * hist_val, 1.f), 0.f);
            colors[sz - 1 - j] = colors[sz - 2 - j] =
            hist_val < m_range_min_max->value().x ? gl::COLOR_WHITE : glm::mix(gl::COLOR_GREEN,
                                                                               gl::COLOR_RED,
                                                                               hist_val);
        }
        const auto line_thickness = 12.f;
        m_line_mesh->material()->uniform("u_window_size", gl::window_dimension());
        m_line_mesh->material()->uniform("u_line_thickness", line_thickness);
        m_line_mesh->material()->set_line_width(line_thickness);
        gl::draw_mesh(m_line_mesh);
        offset += step;
    }

    // global average
    gl::draw_text_2D(to_string(100.f * m_current_value, 1) + "%", fonts()[FONT_LARGE],
                     gl::COLOR_WHITE, vec2(45));


    gl::draw_text_2D("score: " + to_string(final_score(m_display_value)), fonts()[FONT_LARGE], gl::COLOR_RED,
                     vec2(310, 45));

    if(final_score(m_current_value) != *m_score_min){}
    else
    {
        auto color_ready = gl::COLOR_GREEN; color_ready.a = .3f;
        gl::draw_quad(color_ready, vec2(75), vec2(gl::window_dimension().x - 100, 25));
    }
}

/////////////////////////////////////////////////////////////////

void SensorDebug::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void SensorDebug::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
}

/////////////////////////////////////////////////////////////////

void SensorDebug::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);

    switch (e.getCode())
    {
        case Key::_X:
        {
            float rnd_val = random(m_range_min_max->value().x, m_range_min_max->value().y);
            process_impact(rnd_val);
        }
            break;

        default:
            break;
    }
}

/////////////////////////////////////////////////////////////////

void SensorDebug::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void SensorDebug::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void SensorDebug::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void SensorDebug::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void SensorDebug::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void SensorDebug::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{
    displayTweakBar(!displayTweakBar());
}

/////////////////////////////////////////////////////////////////

void SensorDebug::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void SensorDebug::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void SensorDebug::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void SensorDebug::tearDown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void SensorDebug::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);

    if(theProperty == m_device_name_sensor)
    {
        m_current_value = m_sensor_last_max = 0.f;
        
        if(!m_device_name_sensor->value().empty())
        {
            auto serial = Serial::create(background_queue().io_service());
            
            if(serial->open(*m_device_name_sensor))
            {
                m_serial_sensor = serial;
            }
        }
    }
    else if(theProperty == m_sensor_hist_size)
    {
        m_measurements.clear();
    }
    else if(theProperty == m_device_name_nixie)
    {
        if(!m_device_name_nixie->value().empty())
        {
            auto serial = Serial::create(background_queue().io_service());
            if(serial->open(*m_device_name_nixie))
            {
                m_serial_nixie = serial;
            }
        }
    }
    else if(theProperty == m_device_name_dmx)
    {
        if(!m_device_name_dmx->value().empty())
        {
            m_dmx.connect(*m_device_name_dmx);
        }
    }
}

/////////////////////////////////////////////////////////////////

void SensorDebug::update_sensor_values(float time_delta)
{
    m_last_sensor_reading += time_delta;
    std::string reading_str;
    
    // parse sensor input
    if(m_serial_sensor->is_open())
    {
        size_t bytes_to_read = std::min(m_serial_sensor->available(), m_serial_read_buf.size());
        uint8_t *buf_ptr = &m_serial_read_buf[0];

        m_serial_sensor->read_bytes(&m_serial_read_buf[0], bytes_to_read);
        if(bytes_to_read){ m_last_sensor_reading = 0.f; m_sensor_refresh_count++;}
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
            
            if(m_measurements.size() < splits.size())
            {
                m_measurements.resize(std::min<size_t>(splits.size(), 4),
                                      CircularBuffer<float>(*m_sensor_hist_size));
            }
            
            size_t max_index = std::min(m_measurements.size(), splits.size());
            
            for(size_t i = 0; i < max_index; i++)
            {
                auto v = string_to<float>(splits[i]);
                v = clamp(*m_force_multiplier * v / 16.f, 0.f, 1.f);
                m_measurements[i].push_back(v);
                m_sensor_last_max = std::max(m_sensor_last_max, v);
            }
        }
    }

    // reconnect
    if((m_last_sensor_reading > m_sensor_timeout) && (m_sensor_timeout > 0.f))
    {
        m_last_sensor_reading = 0.f;
        m_device_name_sensor->notify_observers();
    }
}

/////////////////////////////////////////////////////////////////

void SensorDebug::update_score_display()
{
    if(m_serial_nixie->is_open())
    {
        auto out_str = to_string(final_score(m_display_value)) + "\n";

        // send our current value
        m_serial_nixie->write(out_str);
    }
}

/////////////////////////////////////////////////////////////////

void SensorDebug::update_dmx()
{
//    // IDLE: [0 - 85] - IMPACT: [86 - 170] - SCORE: [171 - 255]
//    m_dmx[1] = (m_current_gamestate * 255 / 3) + 1;
//
//    // current score
//    m_dmx[2] = (uint8_t)(m_current_value * 255);
    
    for(int i = 1; i < 512; ++i){ m_dmx[i] = 0; }
    
    // determine channel
    int ch = m_current_value * 100 + 1;
    if(ch){ m_dmx[ch] = 255; }
}

/////////////////////////////////////////////////////////////////

void SensorDebug::process_impact(float val)
{
    if(m_current_gamestate != IDLE){ return; }

    auto score = final_score(val);
    if(score != *m_score_min)
    {
        LOG_DEBUG << "hit: " << to_string(val, 2) << " - score: " << final_score(val);
        m_timer_impact_measure.expires_from_now(*m_duration_impact_measure);
    }

}

/////////////////////////////////////////////////////////////////

bool SensorDebug::change_gamestate(GameState the_state)
{
    if(the_state == m_current_gamestate){ return false; }

    bool ret = false;

    switch (the_state)
    {
        case IDLE:
            m_current_value = m_display_value = m_sensor_last_max = 0.f;
            ret = true;
            break;

        case IMPACT:
            if(m_current_gamestate == IDLE)
            {
                // play sound here
                m_current_gamestate = IMPACT;
                LOG_DEBUG << "new state: " << m_gamestate_names[m_current_gamestate];
                return change_gamestate(SCORE);
            }
            break;

        case SCORE:
            if(m_current_gamestate == IMPACT)
            {
                auto score = final_score(m_current_value);

                if(score != *m_score_min)
                {
                    m_animations[ANIMATION_SCORE] = animation::create<float>(&m_display_value,
                                                                      0.f, m_current_value,
                                                                      *m_duration_count_score);

                    m_animations[ANIMATION_SCORE]->set_ease_function(animation::EaseOutCirc());
                    m_animations[ANIMATION_SCORE]->set_finish_callback([this]()
                    {
                        m_timer_game_ready.expires_from_now(*m_duration_display_score);
                    });

                    m_animations[ANIMATION_SCORE]->start();
                    ret = true;
                }
            }
            break;
    }
    if(ret)
    {
        m_current_gamestate = the_state;
        update_dmx();
        LOG_DEBUG << "new state: " << m_gamestate_names[m_current_gamestate];
    }
    return ret;
}
