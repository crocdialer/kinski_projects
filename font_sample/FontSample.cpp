//
//  FontSample.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "gl/Visitor.hpp"
#include "FontSample.hpp"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void FontSample::setup()
{
    ViewerApp::setup();
    register_property(m_use_sdf);
    register_property(m_font_size);
    register_property(m_gamma);
    register_property(m_buffer);
    observe_properties();
    add_tweakbar_for_component(shared_from_this());
    
    fonts()[1].load(fonts()[0].path(), 100, true);
    m_text_root = fonts()[1].create_text_obj("Hallo der Onkel,\nwerd schnell gesund pupu.", 1000);
    m_text_root->set_position(-m_text_root->bounding_box().center());
    scene()->addObject(m_text_root);
    
    load_settings();
    
    m_text_root->position() += gl::vec3(gl::window_dimension() / 2.f, 0.f);
}

/////////////////////////////////////////////////////////////////

void FontSample::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void FontSample::draw()
{
//    gl::set_matrices(camera());
    
    gl::set_matrices(gui_camera());
    gl::draw_boundingbox(m_text_root);
    gl::draw_transform(m_text_root->global_transform(), *m_font_size);
    
    scene()->render(gui_camera());
}

/////////////////////////////////////////////////////////////////

void FontSample::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void FontSample::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
}

/////////////////////////////////////////////////////////////////

void FontSample::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void FontSample::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void FontSample::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void FontSample::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void FontSample::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void FontSample::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void FontSample::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void FontSample::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void FontSample::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void FontSample::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void FontSample::tearDown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void FontSample::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
    
    if(theProperty == m_font_size)
    {
        m_text_root->set_scale(*m_font_size / 100.f);
    }
    else if(theProperty == m_use_sdf)
    {
        gl::SelectVisitor<gl::Mesh> v;
        m_text_root->accept(v);
        
        for(auto *m : v.get_objects())
        {
            if(*m_use_sdf)
            {
                m->material()->set_shader(gl::create_shader(gl::ShaderType::SDF_FONT));
                m->material()->textures() = {fonts()[1].sdf_texture()};
            }
            else
            {
                m->material()->set_shader(gl::create_shader(gl::ShaderType::UNLIT));
                m->material()->textures() = {fonts()[1].glyph_texture()};
            }
        }
    }
    else if(theProperty == m_buffer || theProperty == m_gamma)
    {
        gl::SelectVisitor<gl::Mesh> v;
        m_text_root->accept(v);
        
        for(auto *m : v.get_objects())
        {
            m->material()->uniform("u_buffer", *m_buffer);
            m->material()->uniform("u_gamma", *m_gamma);
        }
    }
}
