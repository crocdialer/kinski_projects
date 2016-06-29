//
//  EmptySample.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#pragma once

#include "app/ViewerApp.hpp"

// module headers
//#include "bluetooth/bluetooth.hpp"
#include "bluetooth/Bluetooth_UART.hpp"

namespace kinski
{
    class BluetoothApp : public ViewerApp
    {
    private:
        
        bluetooth::Bluetooth_UART_Ptr m_bt_serial = bluetooth::Bluetooth_UART::create();
        std::vector<uint8_t> m_accumulator;
        
        bluetooth::CentralPtr m_central = bluetooth::Central::create();
        bluetooth::PeripheralPtr m_peripheral;
        
    public:
        BluetoothApp(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
        void setup() override;
        void update(float timeDelta) override;
        void draw() override;
        void resize(int w ,int h) override;
        void keyPress(const KeyEvent &e) override;
        void keyRelease(const KeyEvent &e) override;
        void mousePress(const MouseEvent &e) override;
        void mouseRelease(const MouseEvent &e) override;
        void mouseMove(const MouseEvent &e) override;
        void mouseDrag(const MouseEvent &e) override;
        void mouseWheel(const MouseEvent &e) override;
        void touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void fileDrop(const MouseEvent &e, const std::vector<std::string> &files) override;
        void tearDown() override;
        void update_property(const Property::ConstPtr &theProperty) override;
    };
}// namespace kinski