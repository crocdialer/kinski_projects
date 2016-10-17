//
//  CapSenseMonitor.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "CapSenseMonitor.hpp"
#include "core/Serial.hpp"
#include "bluetooth/Bluetooth_UART.hpp"

using namespace std;
using namespace kinski;
using namespace glm;

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::setup()
{
    ViewerApp::setup();
    fonts()[FONT_MEDIUM].load(fonts()[FONT_SMALL].path(), 34);
    fonts()[FONT_LARGE].load(fonts()[FONT_SMALL].path(), 54);
    register_property(m_broadcast_udp_port);
    register_property(m_broadcast_frequency);
    register_property(m_device_prefix);
    register_property(m_cap_sense_thresh_touch);
    register_property(m_cap_sense_thresh_release);
    register_property(m_cap_sense_charge_current);
    register_property(m_cap_sense_proxi_multiplier);
    register_property(m_img_url);
    observe_properties();
    
    add_tweakbar_for_component(shared_from_this());
    remote_control().set_components({shared_from_this()});
    
    m_downloader.set_io_service(main_queue().io_service());
    m_broadcast_timer = Timer(main_queue().io_service(),
                              std::bind(&CapSenseMonitor::send_udp_broadcast, this));
    m_broadcast_timer.set_periodic(true);
    
    load_settings();
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    if(m_needs_sensor_reset){ reset_sensors(); m_needs_sensor_reset = false; }
    
    // read sensor values, trigger callbacks
    for(auto &s : m_sensors){ s.update(timeDelta); }
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::draw()
{
    if(textures()[0]){ gl::draw_texture(textures()[0], gl::window_dimension()); }
    
    const int offset_x = 40;
    const gl::vec2 sz = gl::vec2(60, 35);
    
    gl::draw_text_2D(name(), fonts()[FONT_LARGE], gl::COLOR_WHITE, gl::vec2(25, 20));

    gl::vec2 offset(offset_x, 90), step = sz * 1.2f;
    
    for(auto &s : m_sensors)
    {
        for(int i = 0; i < 13; i++)
        {
            auto color = s.is_touched(i) ? gl::COLOR_ORANGE : gl::COLOR_GRAY;
            
            float proxi_val = clamp(*m_cap_sense_proxi_multiplier * s.proximity_values()[12 - i] / 100.f,
                                    0.f, 1.f);
            
            vec2 pos = offset - sz / 2.f,
            sz_frac = s.is_touched(i) ? vec2(sz) : sz * vec2(1.f, proxi_val);
            
            
            gl::draw_quad(gl::COLOR_ORANGE, sz, pos, false);
            gl::draw_quad(color, sz_frac, pos + vec2(0.f, sz.y - sz_frac.y));
            
            if(i == 12)
            {
                gl::draw_text_2D(to_string(int(std::round(proxi_val * 100.f))) + "%", fonts()[FONT_MEDIUM],
                                 gl::COLOR_WHITE, offset - sz / 3.f);
            }
            offset.x += step.x;
        }
        offset.y += step.y;
        offset.x = offset_x;
    }
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::tearDown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
    
    if(theProperty == m_img_url)
    {
        if(!m_img_url->value().empty())
        {
            m_downloader.async_get_url(*m_img_url, [&](net::Downloader::ConnectionInfo c,
                                                       const std::vector<uint8_t> &data)
            {
                textures()[0] = gl::create_texture_from_data(data);
            });
        }
        else{ textures()[0].reset(); }
    }
    else if(theProperty == m_device_prefix)
    {
        m_needs_sensor_reset = true;
    }
    else if(theProperty == m_cap_sense_thresh_touch ||
            theProperty == m_cap_sense_thresh_release)
    {
        for(auto &s : m_sensors)
        {
            if(s.is_initialized())
            {
                s.set_thresholds(*m_cap_sense_thresh_touch, *m_cap_sense_thresh_release);
            }
        }
    }
    else if(theProperty == m_cap_sense_charge_current)
    {
        for(auto &s : m_sensors)
        {
            if(s.is_initialized())
            {
                s.set_charge_current(*m_cap_sense_charge_current);
            }
        }
    }
    else if(theProperty == m_broadcast_frequency)
    {
        if(*m_broadcast_frequency > 0.f)
        {
            m_broadcast_timer.expires_from_now(1.f / *m_broadcast_frequency);
        }
    }
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::sensor_touch(int the_sensor_index, int the_pad_index)
{
    LOG_TRACE << "touched sensor " << the_sensor_index << ": " << the_pad_index;
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::sensor_release(int the_sensor_index, int the_pad_index)
{
    LOG_TRACE << "released sensor " << the_sensor_index << ": " << the_pad_index;
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::send_udp_broadcast()
{
    string str;
    int i = 0;
    
    for(auto &s : m_sensors)
    {
        float vals[3];
        for(int j = 0; j < 3; ++j)
        { vals[j] = clamp(s.proximity_values()[j] * *m_cap_sense_proxi_multiplier / 100.f, 0.f, 1.f); }
        
        str += to_string(i++) + ": " + to_string(s.touch_state()) + " " +
        to_string(vals[0], 3) + " " + to_string(vals[1], 3) + " " + to_string(vals[2], 3) + "\n";
    }
    if(!str.empty())
    {
//        LOG_TRACE_2 << "udp-broadcast (" << *m_broadcast_udp_port << "): " << str.substr(0, str.size() - 1);
        net::async_send_udp_broadcast(background_queue().io_service(), str,
                                      *m_broadcast_udp_port);
    }
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::connect_sensor(UART_Ptr the_uart)
{
    CapacitiveSensor cs;
    auto sensor_index = m_sensors.size();
    cs.set_thresholds(*m_cap_sense_thresh_touch, *m_cap_sense_thresh_release);
    cs.set_charge_current(*m_cap_sense_charge_current);
    cs.set_touch_callback(std::bind(&CapSenseMonitor::sensor_touch, this,
                                    sensor_index, std::placeholders::_1));
    cs.set_release_callback(std::bind(&CapSenseMonitor::sensor_release, this,
                                      sensor_index, std::placeholders::_1));
    cs.set_timeout_reconnect(5.f);
    
    if(cs.connect(the_uart)){ m_sensors.push_back(cs); }
}

/////////////////////////////////////////////////////////////////

void CapSenseMonitor::reset_sensors()
{
    m_sensors.clear();
    auto device_names = fs::get_directory_entries("/dev");
    device_names.erase(std::remove_if(device_names.begin(), device_names.end(),
                                      [this](const std::string &str)
                                      {
                                          return str.find(*m_device_prefix) == string::npos;
                                      }),
                       device_names.end());
    
    
//    if(!m_uart || !m_uart->is_initialized())
    {
        auto blue_uart = bluetooth::Bluetooth_UART::create();
        blue_uart->set_connect_cb([this](bluetooth::Bluetooth_UART_Ptr p)
        {
            connect_sensor(p);
        });
        blue_uart->setup();
        m_uart = blue_uart;
    }
    
    for(const auto &d : device_names)
    {
        auto serial_uart = kinski::Serial::create();
        serial_uart->setup(d, 57600);
        connect_sensor(serial_uart);
    }
}
