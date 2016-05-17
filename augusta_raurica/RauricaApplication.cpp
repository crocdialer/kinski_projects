//
//  RauricaApplication.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "RauricaApplication.hpp"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void RauricaApplication::setup()
{
    ViewerApp::setup();
    fonts()[FONT_MEDIUM].load(fonts()[FONT_SMALL].path(), 34);
    fonts()[FONT_LARGE].load(fonts()[FONT_SMALL].path(), 54);
    
    register_property(m_motion_sensor_timeout);
    observe_properties();
    add_tweakbar_for_component(shared_from_this());
    
    m_timers[BARRIER_01] = Timer(main_queue().io_service());
    m_timers[BARRIER_02] = Timer(main_queue().io_service());
    m_timers[CAP_SENSOR] = Timer(main_queue().io_service());
    
    load_settings();
}

/////////////////////////////////////////////////////////////////

void RauricaApplication::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void RauricaApplication::draw()
{
    draw_status();
}

/////////////////////////////////////////////////////////////////

void RauricaApplication::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void RauricaApplication::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
    
    switch (e.getCode())
    {
        case Key::_M:
            m_timers[BARRIER_01].expires_from_now(*m_motion_sensor_timeout);
            break;
            
        default:
            break;
    }
}

/////////////////////////////////////////////////////////////////

void RauricaApplication::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void RauricaApplication::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void RauricaApplication::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void RauricaApplication::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void RauricaApplication::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void RauricaApplication::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void RauricaApplication::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void RauricaApplication::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void RauricaApplication::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void RauricaApplication::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void RauricaApplication::tearDown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void RauricaApplication::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
}

/////////////////////////////////////////////////////////////////

void RauricaApplication::draw_status()
{
    auto color = gl::COLOR_WHITE - clear_color(); color.a = 1.f;
    const auto offset = gl::vec2(100);
    
    // draw app name
    gl::draw_text_2D(name(), fonts()[FONT_LARGE], color, gl::vec2(25));
    
    // draw catwalk
    const gl::vec2 cat_walk_sz = gl::vec2(380, 165);
    gl::draw_quad(color, cat_walk_sz, offset, false);
    
    // draw sensor inputs
    const gl::vec2 sensor_sz = gl::vec2(10, 125);
    gl::draw_quad(gl::COLOR_OLIVE, sensor_sz, offset + gl::vec2(95, 20), !m_timers[BARRIER_01].has_expired());
    gl::draw_quad(gl::COLOR_OLIVE, sensor_sz, offset + gl::vec2(220, 20), !m_timers[BARRIER_02].has_expired());
    gl::draw_quad(gl::COLOR_OLIVE, sensor_sz, offset + gl::vec2(390, 20), false);
    
    // draw projectors / dmx
}
