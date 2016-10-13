//
//  MeditationRoom.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//
#pragma once

#include "app/ViewerApp.hpp"
#include "core/Timer.hpp"
#include "core/Serial.hpp"

// modules
#include "media/media.h"
#include "sensors/sensors.h"
#include "gl_post_process/WarpComponent.hpp"

namespace kinski
{
    class MeditationRoom : public ViewerApp
    {
    private:
        
        enum class State{IDLE, MANDALA_ILLUMINATED, DESC_MOVIE, MEDITATION};
        
        enum AnimationEnum{ AUDIO_FADE_IN = 0, AUDIO_FADE_OUT = 1, LIGHT_FADE_IN = 2,
            LIGHT_FADE_OUT = 3};
        enum TextureEnum{ TEXTURE_BLANK = 0, TEXTURE_OUTPUT = 1};
        
        std::map<State, std::string> m_state_string_map =
        {
            {State::IDLE, "Idle"},
            {State::MANDALA_ILLUMINATED, "Mandala Illuminated"},
            {State::DESC_MOVIE, "Description Movie"},
            {State::MEDITATION, "Meditation"}
        };
        
        State m_current_state = State::IDLE;
        Timer m_timer_idle, m_timer_motion_reset, m_timer_movie_start;
        bool m_show_movie = true;
        
        Property_<float>::Ptr
        m_timeout_idle = Property_<float>::create("timeout idle", 30.f),
        m_timeout_movie_start = Property_<float>::create("timeout movie start", 1.f),
        m_timeout_fade = Property_<float>::create("timeout fade audio/light", 2.f);
        
        Property_<float>::Ptr
        m_shift_angle = Property_<float>::create("shift angle", 0.f),
        m_shift_amount = Property_<float>::create("shift amount", 10.f),
        m_shift_velocity = Property_<float>::create("shift velocity", 2.5f),
        m_circle_radius = Property_<float>::create("circle radius", 95.f),
        m_blur_amount = Property_<float>::create("blur amount", 10.f),
        m_bio_sensitivity = Property_<float>::create("bio sensitivity", 20.f),
        m_bio_thresh = Property_<float>::create("bio threshold", 0.4f);
        float m_current_circ_radius;
        
        Property_<gl::vec2>::Ptr
        m_output_res = Property_<gl::vec2>::create("output resolution", gl::vec2(1280, 720));
        
        Property_<string>::Ptr
        m_cap_sense_dev_name = Property_<string>::create("touch sensor device"),
        m_led_dev_name = Property_<string>::create("led device"),
        m_motion_sense_dev_name = Property_<string>::create("motion sensor device"),
        m_bio_sense_dev_name = Property_<string>::create("bio sensor device");
        
        SerialPtr
        m_motion_sense = Serial::create(),
        m_bio_sense = Serial::create(), m_led_device = Serial::create();
        
        std::vector<uint8_t> m_serial_buf;
        bool m_motion_detected = false;
        
        Property_<gl::Color>::Ptr
        m_led_color = Property_<gl::Color>::create("LED color", gl::COLOR_BLACK);
        
        Property_<float>::Ptr
        m_volume = Property_<float>::create("volume", 1.f),
        m_volume_max = Property_<float>::create("volume max", 1.f),
        m_bio_score = Property_<float>::create("bio score", 0.f);
        
        gl::MaterialPtr m_mat_rgb_shift;
        std::vector<gl::Fbo> m_fbos;
        
        //! ouput warping
        WarpComponent::Ptr m_warp;
        void output_switch();
        
        // our content
        media::MediaControllerPtr
        m_movie = media::MediaController::create(),
        m_audio = media::MediaController::create();
        
        CapacitiveSensor m_cap_sense;
        
        bool change_state(State the_the_state, bool force_change = false);
        
        //! read our motion sensor, update m_motion_detected member, start timer to reset val
        void detect_motion();
        
        void read_bio_sensor();
        
        void update_bio_visuals();
        
        void set_led_color(const gl::Color &the_color);
        
        //! check if a valid fbo is present and set current resolution, if necessary
        void set_fbo_state();
        
        //! display sensor and application state
        void draw_status_info();
        
        bool load_assets();
        
        void create_animations();
        
    public:
        MeditationRoom(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
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
    };
}// namespace kinski

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::MeditationRoom>(argc, argv);
    LOG_INFO << "local ip: " << kinski::net::local_ip();
    return theApp->run();
}
