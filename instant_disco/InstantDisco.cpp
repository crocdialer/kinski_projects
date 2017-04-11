//
//  InstantDisco.cpp
//
//  Created by Fabian on 11/04/17.
//
//

#include "InstantDisco.hpp"

using namespace std;
using namespace kinski;
using namespace glm;

namespace
{
    std::string g_text_obj_tag = "text_obj";
}
/////////////////////////////////////////////////////////////////

void InstantDisco::setup()
{
    ViewerApp::setup();
    fonts()[FONT_LARGE].load(fonts()[FONT_NORMAL].path(), 63.f);
    observe_properties();
    add_tweakbar_for_component(shared_from_this());
    load_settings();

    m_text_obj = fonts()[FONT_LARGE].create_text_obj("Welcome\nto the\ninstant disco!", 640, gl::Font::Align::CENTER);
    m_text_obj->set_name("greeting_txt");
    m_text_obj->add_tag(g_text_obj_tag);
//    m_text_obj->set_position(m_text_obj->position() + gl::vec3(0, m_text_obj->bounding_box().height(), 0));
    m_text_obj->set_position(gl::vec3(450, 300, 0));
    scene()->add_object(m_text_obj);
}

/////////////////////////////////////////////////////////////////

void InstantDisco::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void InstantDisco::draw()
{
    // 2D coords
    gl::set_matrices(gui_camera());
    gl::draw_boundingbox(m_text_obj);
    scene()->render(gui_camera());
}

/////////////////////////////////////////////////////////////////

void InstantDisco::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
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

    auto obj = scene()->pick(gl::calculate_ray(gui_camera(), glm::vec2(e.getX(), e.getY())), false);

    if(obj)
    {
        LOG_DEBUG << "picked " << obj->name();
    }
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

void InstantDisco::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
}
