//
//  InstantDisco.cpp
//
//  Created by Fabian on 11/04/17.
//
//

#include "InstantDisco.hpp"

#if defined(KINSKI_RASPI)
#include <wiringPi.h>
#endif

using namespace std;
using namespace kinski;
using namespace glm;

namespace
{
    const std::string g_text_obj_tag = "text_obj";
    const std::string g_button_tag = "button";
    const uint32_t g_led_pin = 7;
    const uint32_t g_button_pin = 0;
    InstantDisco *g_self = nullptr;
}

gl::MeshPtr create_button(const std::string &icon_path)
{
    auto ret = gl::Mesh::create(gl::Geometry::create_plane(1, 1), gl::Material::create());
    vec3 center = ret->geometry()->bounding_box().center();
    for(auto &v : ret->geometry()->vertices()){ v -= center; }
    ret->material()->set_blending();
    ret->material()->set_depth_test(false);
    ret->material()->set_depth_write(false);
    ret->material()->queue_texture_load(icon_path);
    ret->add_tag(g_button_tag);
    ret->set_scale(180.f);
    return ret;
}

/////////////////////////////////////////////////////////////////

void InstantDisco::setup()
{
    ViewerApp::setup();
    fonts()[FONT_LARGE].load(fonts()[FONT_NORMAL].path(), 63.f);
    register_property(m_media_path);
    register_property(m_audio_enabled);
    register_property(m_strobo_enabled);
    register_property(m_discoball_enabled);
    register_property(m_fog_enabled);
    register_property(m_led_enabled);
    register_property(m_button_pressed);
    observe_properties();
    add_tweakbar_for_component(shared_from_this());

//    m_text_obj = fonts()[FONT_LARGE].create_text_obj("Welcome\nto the\ninstant disco!", 640, gl::Font::Align::CENTER);
//    m_text_obj->set_name("greeting_txt");
//    m_text_obj->add_tag(g_text_obj_tag);
//    m_text_obj->set_position(gl::vec3(450, 300, 0));
//    scene()->add_object(m_text_obj);

    // set ref
    g_self = this;

    scene()->add_object(m_buttons);
    
    auto audio_icon = create_button("audio.png");
    audio_icon->set_position(gl::vec3(100, 100, 0));
    m_buttons->add_child(audio_icon);
    m_action_map[audio_icon] = [this](){ *m_audio_enabled = !*m_audio_enabled; };
    
    auto disco_icon = create_button("disco_ball.png");
    disco_icon->set_position(gl::vec3(300, 100, 0));
    m_buttons->add_child(disco_icon);
    m_action_map[disco_icon] = [this](){ *m_discoball_enabled = !*m_discoball_enabled; };
    
    auto strobo_icon = create_button("stroboscope.png");
    strobo_icon->set_position(gl::vec3(500, 100, 0));
    m_buttons->add_child(strobo_icon);
    m_action_map[strobo_icon] = [this](){ *m_strobo_enabled = !*m_strobo_enabled; };
    
    auto fog_icon = create_button("fog.png");
    fog_icon->set_position(gl::vec3(700, 100, 0));
    m_buttons->add_child(fog_icon);
    m_action_map[fog_icon] = [this](){ *m_fog_enabled = !*m_fog_enabled; };

    // setup timers
    m_timer_strobo = Timer(main_queue().io_service(), [this](){ *m_strobo_enabled = true; });
    m_timer_fog = Timer(main_queue().io_service(), [this](){ *m_fog_enabled = true; });
    m_timer_led = Timer(background_queue().io_service(), [this](){ *m_led_enabled = !*m_led_enabled; });
    m_timer_led.set_periodic();
    m_timer_led.expires_from_now(1.0);

#if defined(KINSKI_RASPI)
    wiringPiSetup();
    pinMode(g_led_pin, OUTPUT);
    pullUpDnControl(g_button_pin, PUD_UP);
    wiringPiISR(g_button_pin, INT_EDGE_BOTH,  &InstantDisco::button_ISR);
#endif

    load_settings();
}

/////////////////////////////////////////////////////////////////

void InstantDisco::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    m_dmx.update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void InstantDisco::draw()
{
    // 2D coords
    gl::set_matrices(gui_camera());
//    gl::draw_boundingbox(m_text_obj);
    
    scene()->render(gui_camera());
}

/////////////////////////////////////////////////////////////////

void InstantDisco::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
    
    // position buttons
    auto button_aabb = m_buttons->bounding_box();
    m_buttons->set_position(gl::vec3(0, h - button_aabb.height() - 100, 0));
}

/////////////////////////////////////////////////////////////////

void InstantDisco::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);
}

/////////////////////////////////////////////////////////////////

void InstantDisco::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void InstantDisco::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);

    auto obj = scene()->pick(gl::calculate_ray(gui_camera(), glm::vec2(e.getX(), e.getY())), false,
                             {g_button_tag});
    
    LOG_DEBUG_IF(obj) << "picked " << obj->name();
    
    auto iter = m_action_map.find(obj);
    if(iter != m_action_map.end()){ iter->second(); }
}

/////////////////////////////////////////////////////////////////

void InstantDisco::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);
}

/////////////////////////////////////////////////////////////////

void InstantDisco::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void InstantDisco::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void InstantDisco::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void InstantDisco::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void InstantDisco::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void InstantDisco::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
}

/////////////////////////////////////////////////////////////////

void InstantDisco::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void InstantDisco::teardown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void InstantDisco::update_property(const Property::ConstPtr &the_property)
{
    ViewerApp::update_property(the_property);
    
    if(the_property == m_audio_enabled)
    {
        LOG_DEBUG << *m_audio_enabled;
    }
    else if(the_property == m_strobo_enabled)
    {
        LOG_DEBUG << *m_strobo_enabled;
    }
    else if(the_property == m_fog_enabled)
    {
        LOG_DEBUG << *m_fog_enabled;
    }
    else if(the_property == m_discoball_enabled)
    {
        LOG_DEBUG << *m_discoball_enabled;
    }
    else if(the_property == m_led_enabled)
    {
//        LOG_DEBUG << "LED: " << m_led_enabled->value();
        kinski::syscall("gpio " + to_string(g_led_pin) + " " + to_string(m_led_enabled->value()));
    }
    else if(the_property == m_media_path)
    {
//        LOG_DEBUG << "LED: " << m_led_enabled->value();
        m_media->load(*m_media_path, false, true);
    }
    else if(the_property == m_button_pressed)
    {
        LOG_DEBUG << "button: " << m_button_pressed->value();
    }
}

/////////////////////////////////////////////////////////////////

void InstantDisco::button_ISR()
{
#if defined(KINSKI_RASPI)
    g_self->m_button_pressed = !digitalRead(g_button_pin);
#endif
}
