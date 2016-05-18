//
//  EmptySample.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//
#pragma once

#include "app/ViewerApp.hpp"
#include "core/Timer.hpp"

// modules
#include "curl/Downloader.h"
#include "sensors/CapacitiveSensor.hpp"

namespace kinski
{
    class UriTerminal : public ViewerApp
    {
    private:
        
        enum State{STATE_IDLE = 0, STATE_IMAGE = 1};
        enum FontEnum{FONT_NORMAL = 0, FONT_LARGE = 1};
        
        CapacitiveSensor m_sensor;
        
        //! used for http-requests
        net::Downloader m_downloader;
        
        //! reconnection timer for sensors
        Timer m_timer_reconnect;
        
        gl::Scene m_gui_scene;
        
        Property_<string>::Ptr
        m_touch_device_name = Property_<string>::create("touch sensor device"),
        m_img_url = Property_<string>::create("image url");
        
    public:
        
        UriTerminal(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
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
        void fileDrop(const MouseEvent &e, const std::vector<std::string> &files) override;
        void tearDown() override;
        void update_property(const Property::ConstPtr &theProperty) override;
        
        void sensor_touch(int the_index);
        void sensor_release(int the_index);
    };
}// namespace kinski