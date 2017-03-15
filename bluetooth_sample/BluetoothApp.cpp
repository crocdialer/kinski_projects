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

#define SERIAL_END_CODE '\n'

/////////////////////////////////////////////////////////////////

void BluetoothApp::setup()
{
    ViewerApp::setup();
    fonts()[1].load(fonts()[0].path(), 44);
    observe_properties();
    add_tweakbar_for_component(shared_from_this());
    load_settings();

    m_central->set_peripheral_discovered_cb([this](bluetooth::CentralPtr c,
                                                   bluetooth::PeripheralPtr p)
    {
        LOG_DEBUG << p->name() << " rssi: " << p->rssi();

        if(p->connectable()){ c->connect_peripheral(p); }
    });

    m_central->set_peripheral_connected_cb([this](bluetooth::CentralPtr c,
                                                  bluetooth::PeripheralPtr p)
    {
        m_peripheral = p;
        c->stop_scanning();
        p->discover_services();
    });
    m_central->discover_peripherals();
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::update(float timeDelta)
{
    ViewerApp::update(timeDelta);

}

/////////////////////////////////////////////////////////////////

void BluetoothApp::draw()
{
    gl::draw_text_2D(name(), fonts()[1], gl::COLOR_WHITE, vec2(15));
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);

    switch (e.getCode())
    {
        case Key::_D:
            m_central->disconnect_all();
            m_central->discover_peripherals();
            break;

        case Key::_B:
            // if(m_bt_serial->is_initialized())
            // {
            //     m_bt_serial->write("lalala der obstmostkelterer ist da\n");
            // }
            break;
            
        default:
            break;
    }
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
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

void BluetoothApp::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::teardown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void BluetoothApp::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
}
