//
//  MeditationRoom.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//
#pragma once

#include "app/ViewerApp.h"
#include "core/Timer.h"
#include "core/Serial.h"

// modules
#include "video/video.h"
#include "cap_sense/cap_sense.h"
#include "audio/audio.h"

namespace kinski
{
    class MeditationRoom : public ViewerApp
    {
    private:
        
        enum class State{IDLE, MANDALA_ILLUMINATED, DESC_MOVIE, MEDITATION};
        State m_current_state = State::MANDALA_ILLUMINATED;
        Timer m_timer_idle;
        
        Property_<float>::Ptr
        m_timeout_idle = Property_<float>::create("timeout idle", 30.f);
        
        Property_<float>::Ptr
        m_shift_angle = Property_<float>::create("shift angle", 0.f),
        m_shift_amount = Property_<float>::create("shift amount", 10.f),
        m_blur_amount = Property_<float>::create("blur amount", 10.f);
        
        Property_<gl::vec2>::Ptr
        m_output_res = Property_<gl::vec2>::create("output resolution", gl::vec2(1280, 720));
        
        Property_<string>::Ptr
        m_cap_sense_dev_name = Property_<string>::create("touch sensor device"),
        m_motion_sense_dev_name = Property_<string>::create("motion sensor device");
        
        Serial m_motion_sense;
        std::vector<uint8_t> m_serial_buf;
        bool m_tmp_motion = false;
        
        gl::MaterialPtr m_mat_rgb_shift;
        std::vector<gl::Fbo> m_fbos;
        
        video::MovieControllerPtr m_movie;
        
        CapacitiveSensor m_cap_sense;
        
        bool change_state(State the_the_state);
        
        bool detect_motion();
        
        void read_belt_sensor();
        
        void set_mandala_leds(const gl::Color &the_color);
        
        //! check if a valid fbo is present and set current resolution, if necessary
        void set_fbo_state();
        
    public:
        
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
        void got_message(const std::vector<uint8_t> &the_message) override;
        void fileDrop(const MouseEvent &e, const std::vector<std::string> &files) override;
        void tearDown() override;
        void update_property(const Property::ConstPtr &theProperty) override;
    };
}// namespace kinski