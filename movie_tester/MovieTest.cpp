//
//  MovieTest.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "MovieTest.h"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void MovieTest::setup()
{
    ViewerApp::setup();
    set_window_title("movie tester");

    register_property(m_movie_speed);
    register_property(m_movie_path);
    observe_properties();
    add_tweakbar_for_component(shared_from_this());

//    m_movie->set_on_load_callback(bind(&MovieTest::on_movie_load, this));

    // warp component
    m_warp = std::make_shared<WarpComponent>();
    m_warp->observe_properties();
    add_tweakbar_for_component(m_warp);

    remote_control().set_components({ shared_from_this(), m_warp });
    load_settings();
}

/////////////////////////////////////////////////////////////////

void MovieTest::update(float timeDelta)
{
    if(m_camera_control && m_camera_control->is_capturing())
        m_camera_control->copy_frame_to_texture(textures()[TEXTURE_INPUT]);
    else
        m_movie->copy_frame_to_texture(textures()[TEXTURE_INPUT]);
}

/////////////////////////////////////////////////////////////////

void MovieTest::draw()
{
    // gl::draw_texture(textures()[TEXTURE_INPUT], gl::window_dimension());
    m_warp->quad_warp().render_output(textures()[TEXTURE_INPUT]);
    m_warp->quad_warp().render_grid();

    if(displayTweakBar())
    {
        gl::draw_text_2D(m_movie->get_path() + " : " +
                       kinski::as_string(m_movie->current_time(), 2) + " / " +
                       kinski::as_string(m_movie->duration(), 2),
                       fonts()[0]);
        draw_textures(textures());
    }
}

/////////////////////////////////////////////////////////////////

void MovieTest::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);

    switch (e.getCode())
    {
        case Key::_C:
            if(m_camera_control->is_capturing())
                m_camera_control->stop_capture();
            else
                m_camera_control->start_capture();
            break;

        case Key::_P:
            m_movie->isPlaying() ? m_movie->pause() : m_movie->play();
            break;

        case Key::_LEFT:
            m_movie->seek_to_time(m_movie->current_time() - 5);
            break;

        case Key::_RIGHT:
            m_movie->seek_to_time(m_movie->current_time() + 5);
            break;
        case Key::_UP:
            m_movie->set_volume(m_movie->volume() + .1f);
            break;

        case Key::_DOWN:
            m_movie->set_volume(m_movie->volume() - .1f);
            break;

        case Key::_O:
        {
            // auto new_window = GLFW_Window::create(640, 480, "output", false, 0, windows().back()->handle());
            // add_window(new_window);
            // new_window->set_draw_function([this]()
            // {
            //     static auto mat = gl::Material::create();
            //     gl::apply_material(mat);
            //
            //     gl::clear();
            //     m_warp->quad_warp().render_output(textures()[0]);
            //     m_warp->quad_warp().render_grid();
            //     m_warp->quad_warp().render_control_points();
            // });
        }
            break;

        default:
            break;
    }
}

/////////////////////////////////////////////////////////////////

void MovieTest::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void MovieTest::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void MovieTest::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void MovieTest::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void MovieTest::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void MovieTest::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void MovieTest::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void MovieTest::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    *m_movie_path = files.back();
}

/////////////////////////////////////////////////////////////////

void MovieTest::tearDown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void MovieTest::on_movie_load()
{
    m_movie->play();
}

/////////////////////////////////////////////////////////////////

void MovieTest::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);

    if(theProperty == m_movie_path)
    {
//        glfwMakeContextCurrent(windows().front()->handle());
        m_movie->load(*m_movie_path, true, true);
    }
    else if(theProperty == m_movie_speed)
    {
        m_movie->set_rate(*m_movie_speed);
    }
}
