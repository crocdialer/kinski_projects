// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  EmptySample.hpp
//
//  Created by Fabian on 29/01/14.

#pragma once

#include "core/Timer.hpp"
#include "app/ViewerApp.hpp"

namespace kinski
{
    class RauricaApplication : public ViewerApp
    {
    private:
        enum FontEnum{FONT_SMALL = 0, FONT_MEDIUM = 1, FONT_LARGE = 2};
        enum SensorEnum{BARRIER_01, BARRIER_02, CAP_SENSOR};
        std::map<SensorEnum, Timer> m_timers;
        
        Property_<float>::Ptr
        m_motion_sensor_timeout = RangedProperty<float>::create("motion sensor: timeout",
                                                                3.f , 0.f, 100.f);
        
        void draw_status();
        
    public:
        RauricaApplication(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
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