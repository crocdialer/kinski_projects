//
//  BluetoothApp.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "BluetoothApp.h"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void BluetoothApp::setup()
{
    ViewerApp::setup();
    observe_properties();
    add_tweakbar_for_component(shared_from_this());
    load_settings();
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::draw()
{
    gl::set_matrices(camera());
    gl::draw_grid(50, 50);
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
    
    switch (e.getCode())
    {
        case GLFW_KEY_D:
            m_central.scan_for_peripherals();
            break;
            
        default:
            break;
    }
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{
    
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{
    
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{
    
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::tearDown()
{
    LOG_PRINT<<"ciao empty sample";
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
}
