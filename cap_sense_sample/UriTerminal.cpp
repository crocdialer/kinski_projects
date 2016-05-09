//
//  UriTerminal.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "UriTerminal.h"
#include "bluetooth/Bluetooth_UART.hpp"

using namespace std;
using namespace kinski;
using namespace glm;

/////////////////////////////////////////////////////////////////

void UriTerminal::setup()
{
    ViewerApp::setup();
    register_property(m_touch_device_name);
    register_property(m_img_url);
    observe_properties();
    add_tweakbar_for_component(shared_from_this());
    m_downloader.set_io_service(main_queue().io_service());
    load_settings();
    
    fonts()[FONT_LARGE].load("Courier New Bold.ttf", 72);
    
    auto text_obj = fonts()[FONT_LARGE].create_text_obj("Berühre einen der Tentakel ...", 720.f);
    text_obj->setPosition(gl::vec3(100.f, 250.f, 0.f));
    m_gui_scene.addObject(text_obj);
    
    m_sensor.set_touch_callback(std::bind(&UriTerminal::sensor_touch, this, std::placeholders::_1));
    m_sensor.set_release_callback(std::bind(&UriTerminal::sensor_release, this, std::placeholders::_1));
}

/////////////////////////////////////////////////////////////////

void UriTerminal::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    // read sensor values, trigger callbacks
    m_sensor.update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void UriTerminal::draw()
{
    if(textures()[0]){ gl::draw_texture(textures()[0], gl::window_dimension()); }
    
    m_gui_scene.render(gui_camera());
    
//    else{ gl::drawText2D("Berühre einen der Kristalle ...", fonts()[FONT_LARGE], gl::COLOR_WHITE,
//                         gl::vec2(75, windowSize().y / 2.f));}
    gl::vec2 offset(75), step(50, 0);
    
    for(int i = 0; i < m_sensor.num_touchpads(); i++)
    {
        gl::vec2 sz = m_sensor.is_touched(i) ? gl::vec2(50) : gl::vec2(25);
        
        gl::draw_quad(gl::COLOR_ORANGE, sz, offset - sz / 2.f);
        offset += step;
    }
}

/////////////////////////////////////////////////////////////////

void UriTerminal::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void UriTerminal::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
}

/////////////////////////////////////////////////////////////////

void UriTerminal::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void UriTerminal::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void UriTerminal::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void UriTerminal::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void UriTerminal::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void UriTerminal::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void UriTerminal::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void UriTerminal::tearDown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void UriTerminal::update_property(const Property::ConstPtr &theProperty)
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
    else if(theProperty == m_touch_device_name)
    {
        if(!m_touch_device_name->value().empty()){ m_sensor.connect(*m_touch_device_name); }
        else
        {
            auto uart = std::make_shared<bluetooth::Bluetooth_UART>();
            uart->setup();
            m_sensor.connect(uart);
        }
    }
}

/////////////////////////////////////////////////////////////////

void UriTerminal::sensor_touch(int the_index)
{
    LOG_DEBUG << "touched sensor " << the_index;
}

/////////////////////////////////////////////////////////////////

void UriTerminal::sensor_release(int the_index)
{
    LOG_DEBUG << "released sensor " << the_index;
}
