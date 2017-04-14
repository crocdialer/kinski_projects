//
//  InstantDisco.cpp
//
//  Created by Fabian on 11/04/17.
//
//

#include <atomic>
#include <gl/Visitor.hpp>
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
    const std::string g_select_obj_tag = "selection";
    const std::string g_button_tag = "button";
    const uint32_t g_led_pin = 7;
    const uint32_t g_button_pin = 0;
    std::atomic<bool> g_state_changed(true);
}

gl::MeshPtr create_button(const std::string &icon_path)
{
    auto geom = gl::Geometry::create_plane(1, 1);
    vec3 center = geom->bounding_box().center();
    for(auto &v : geom->vertices()){ v -= center; }

    auto select_mesh = gl::Mesh::create(geom, gl::Material::create());
    select_mesh->material()->set_blending();
    select_mesh->material()->set_depth_test(false);
    select_mesh->material()->set_depth_write(false);
    select_mesh->material()->queue_texture_load("selection.png");
    select_mesh->add_tag(g_select_obj_tag);

    auto ret = gl::Mesh::create(geom, gl::Material::create());
    ret->material()->set_blending();
    ret->material()->set_depth_test(false);
    ret->material()->set_depth_write(false);
    ret->material()->queue_texture_load(icon_path);
    ret->add_tag(g_button_tag);
    ret->set_scale(180.f);
    ret->add_child(select_mesh);
    return ret;
}

/////////////////////////////////////////////////////////////////

void InstantDisco::setup()
{
    ViewerApp::setup();
    fonts()[FONT_LARGE].load(fonts()[FONT_NORMAL].path(), 63.f);
    register_property(m_media_path);
    register_property(m_strobo_dmx_values);
    register_property(m_discoball_dmx_values);
    register_property(m_fog_dmx_values);
    register_property(m_timout_strobo);
    register_property(m_timout_discoball);
    register_property(m_timout_fog);
    register_property(m_timout_audio_rewind);
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

    scene()->add_object(m_buttons);

    m_audio_icon = create_button("audio.png");
    m_audio_icon->set_position(gl::vec3(100, 100, 0));
    m_buttons->add_child(m_audio_icon);
    m_action_map[m_audio_icon] = [this](){ *m_audio_enabled = !*m_audio_enabled; };
    
    m_discoball_icon = create_button("disco_ball.png");
    m_discoball_icon->set_position(gl::vec3(300, 100, 0));
    m_buttons->add_child(m_discoball_icon);
    m_action_map[m_discoball_icon] = [this](){ *m_discoball_enabled = !*m_discoball_enabled; };
    
    m_strobo_icon = create_button("stroboscope.png");
    m_strobo_icon->set_position(gl::vec3(500, 100, 0));
    m_buttons->add_child(m_strobo_icon);
    m_action_map[m_strobo_icon] = [this](){ *m_strobo_enabled = !*m_strobo_enabled; };
    
    m_fog_icon = create_button("fog.png");
    m_fog_icon->set_position(gl::vec3(700, 100, 0));
    m_buttons->add_child(m_fog_icon);
    m_action_map[m_fog_icon] = [this](){ *m_fog_enabled = !*m_fog_enabled; };

    // setup timers
    m_timer_strobo = Timer(main_queue().io_service(), [this](){ *m_strobo_enabled = true; });
    m_timer_disco_ball = Timer(main_queue().io_service(), [this](){ *m_discoball_enabled = true; });
    m_timer_fog = Timer(main_queue().io_service(), [this](){ *m_fog_enabled = true; });
    m_timer_audio_restart = Timer(main_queue().io_service(), [this](){ m_media->seek_to_time(0); });
    m_timer_led = Timer(background_queue().io_service(), [this](){ *m_led_enabled = !*m_led_enabled; });
    m_timer_led.set_periodic();
    m_timer_led.expires_from_now(1.0);

    // position buttons
    auto button_aabb = m_buttons->bounding_box();
    m_buttons->set_position(gl::vec3(0, gl::window_dimension().y - button_aabb.height() - 100, 0));

#if defined(KINSKI_RASPI)
    wiringPiSetup();
    pinMode(g_led_pin, OUTPUT);
    pinMode(g_button_pin, INPUT);
    pullUpDnControl(g_button_pin, PUD_UP);
    wiringPiISR(g_button_pin, INT_EDGE_BOTH,  &InstantDisco::button_ISR);
#endif

    load_settings();
}

/////////////////////////////////////////////////////////////////

void InstantDisco::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
#if defined(KINSKI_RASPI)
    if(g_state_changed)
    {
        bool state = !digitalRead(g_button_pin);
        if(*m_button_pressed != state){ *m_button_pressed = state; }
        g_state_changed = false;
    }
#endif
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

    *m_strobo_enabled = false;
    *m_fog_enabled = false;
    *m_discoball_enabled = false;
    *m_led_enabled = false;
}

/////////////////////////////////////////////////////////////////

void InstantDisco::update_property(const Property::ConstPtr &the_property)
{
    ViewerApp::update_property(the_property);
    
    if(the_property == m_audio_enabled)
    {
        LOG_DEBUG << *m_audio_enabled;

        gl::SelectVisitor<gl::Object3D> sv({g_select_obj_tag}, false);
        m_audio_icon->accept(sv);
        for(auto obj : sv.get_objects()){ obj->set_enabled(*m_audio_enabled); }

        if(*m_audio_enabled){ m_media->play(); }
        else{ m_media->pause(); }
    }
    else if(the_property == m_strobo_enabled)
    {
        LOG_DEBUG << *m_strobo_enabled;

        gl::SelectVisitor<gl::Object3D> sv({g_select_obj_tag}, false);
        m_strobo_icon->accept(sv);
        for(auto obj : sv.get_objects()){ obj->set_enabled(*m_strobo_enabled); }

        //TODO: set dmx values
    }
    else if(the_property == m_fog_enabled)
    {
        LOG_DEBUG << *m_fog_enabled;

        gl::SelectVisitor<gl::Object3D> sv({g_select_obj_tag}, false);
        m_fog_icon->accept(sv);
        for(auto obj : sv.get_objects()){ obj->set_enabled(*m_fog_enabled); }

        //TODO: set dmx values
    }
    else if(the_property == m_discoball_enabled)
    {
        LOG_DEBUG << *m_discoball_enabled;

        gl::SelectVisitor<gl::Object3D> sv({g_select_obj_tag}, false);
        m_discoball_icon->accept(sv);
        for(auto obj : sv.get_objects()){ obj->set_enabled(*m_discoball_enabled); }

        //TODO: set dmx values
    }
    else if(the_property == m_led_enabled)
    {
#if defined(KINSKI_RASPI)
	    digitalWrite(g_led_pin, *m_led_enabled);
#endif
    }
    else if(the_property == m_media_path)
    {
        m_media->load(*m_media_path, false, true);
    }
    else if(the_property == m_button_pressed)
    {
        LOG_DEBUG << "button: " << m_button_pressed->value();

        if(*m_button_pressed)
        {
            m_timer_audio_restart.cancel();
            m_timer_led.cancel();
            *m_led_enabled = false;
            *m_audio_enabled = true;

            //TODO: random timing here
            m_timer_strobo.expires_from_now(*m_timout_strobo);
            m_timer_disco_ball.expires_from_now(*m_timout_discoball);
            m_timer_fog.expires_from_now(*m_timout_fog);
        }
        else
        {
            m_timer_audio_restart.expires_from_now(*m_timout_audio_rewind);
            m_timer_strobo.cancel();
            m_timer_disco_ball.cancel();
            m_timer_fog.cancel();
            m_timer_led.expires_from_now(1.0);

            *m_audio_enabled = false;
            *m_strobo_enabled = false;
            *m_fog_enabled = false;
            *m_discoball_enabled = false;
        }
    }
}

/////////////////////////////////////////////////////////////////

void InstantDisco::button_ISR()
{
    g_state_changed = true;
}
