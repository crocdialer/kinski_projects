//
//  MeditationRoom.h
//  gl
//
//  Created by Fabian on 29/01/16.
//
//
#pragma once

#include "app/ViewerApp.hpp"
#include "core/Timer.hpp"
#include "core/Serial.hpp"

// modules
#include "media/media.h"
#include "sensors/sensors.h"
#include "dmx/dmx.h"
#include "gl_post_process/WarpComponent.hpp"

namespace kinski
{
    class MeditationRoom : public ViewerApp
    {
    private:
        
        enum class State{IDLE, WELCOME, MANDALA_ILLUMINATED, DESC_MOVIE, MEDITATION};
        
        enum AnimationEnum{ AUDIO_FADE_IN = 0, AUDIO_FADE_OUT = 1, LIGHT_FADE_IN = 2,
            LIGHT_FADE_OUT = 3, PROJECTION_FADE_IN = 4, PROJECTION_FADE_OUT = 5,
            SPOT_01_FADE_IN = 6, SPOT_01_FADE_OUT = 7, SPOT_02_FADE_IN = 8, SPOT_02_FADE_OUT = 9};
        enum TextureEnum{ TEXTURE_BLANK = 0, TEXTURE_OUTPUT = 1};
        
        enum AudioEnum{ AUDIO_WELCOME = 0, AUDIO_CHANTING = 1, AUDIO_WIND = 2 };
        
        std::map<State, std::string> m_state_string_map =
        {
            {State::IDLE, "Idle"},
            {State::WELCOME, "Welcome"},
            {State::MANDALA_ILLUMINATED, "Mandala Illuminated"},
            {State::DESC_MOVIE, "Description Movie"},
            {State::MEDITATION, "Meditation"}
        };
        
        State m_current_state = State::IDLE;
        Timer m_timer_idle, m_timer_audio_start, m_timer_motion_reset, m_timer_movie_pause,
            m_timer_meditation_cancel, m_timer_sensor_update, m_timer_cap_trigger;
        
        float m_last_sensor_reading = 0.f, m_sensor_timeout = 5.f;
        CircularBuffer<float> m_bio_acceleration, m_bio_elongation;
        std::vector<uint8_t> m_serial_accumulator, m_serial_read_buf;
        
        double m_sensor_read_timestamp = getApplicationTime();
        
        Property_<float>::Ptr
        m_timeout_idle = Property_<float>::create("timeout idle", 30.f),
        m_timeout_movie_pause = Property_<float>::create("timeout movie pause", 5.f),
        m_timeout_audio = Property_<float>::create("timeout for audio start", 10.f),
        m_timeout_meditation_cancel = Property_<float>::create("timeout for meditation cancel", 30.f),
        m_duration_fade = Property_<float>::create("duration fade audio/video/light", 2.f);
        
        Property_<float>::Ptr
        m_shift_angle = Property_<float>::create("shift angle", 0.f),
        m_shift_amount = Property_<float>::create("shift amount", 10.f),
        m_shift_velocity = Property_<float>::create("shift velocity", 2.5f),
        m_circle_radius = Property_<float>::create("circle radius", 95.f),
        m_blur_amount = Property_<float>::create("blur amount", 10.f),
        m_bio_sensitivity_elong = Property_<float>::create("bio sensitivity (elongation)", 1.f),
        m_bio_sensitivity_accel = Property_<float>::create("bio sensitivity (accelaration)", 20.f),
        m_bio_thresh = Property_<float>::create("bio threshold", 0.4f);
        float m_current_circ_radius;
        
        Property_<gl::vec2>::Ptr
        m_output_res = Property_<gl::vec2>::create("output resolution", gl::vec2(1280, 720));
        Property_<uint32_t>::Ptr
        m_output_crop = Property_<uint32_t>::create("output crop", 0);
        
        Property_<string>::Ptr
        m_asset_dir = Property_<string>::create("asset base directory"),
        m_cap_sense_dev_name = Property_<string>::create("touch sensor device"),
        m_led_dev_name = Property_<string>::create("led device"),
        m_motion_sense_dev_name = Property_<string>::create("motion sensor device"),
        m_bio_sense_dev_name = Property_<string>::create("bio sensor device");
        
        SerialPtr
        m_bio_sense = Serial::create(), m_led_device = Serial::create();
        
        DistanceSensor m_motion_sensor;
        bool m_motion_detected = false;
        
        DMXController m_dmx;
        bool m_dmx_needs_refresh = true;
        
        Property_<gl::Color>::Ptr
        m_led_full_bright = Property_<gl::Color>::create("LED full brightness", gl::Color(1.f, 0, 0, 1.f)),
        m_led_color = Property_<gl::Color>::create("LED color", gl::COLOR_BLACK),
        m_spot_color_01 = Property_<gl::Color>::create("spot 1", gl::COLOR_BLACK),
        m_spot_color_02 = Property_<gl::Color>::create("spot 2", gl::COLOR_BLACK);
        
        Property_<float>::Ptr
        m_volume = Property_<float>::create("volume", 1.f),
        m_volume_max = Property_<float>::create("volume max", 1.f),
        m_bio_score = Property_<float>::create("bio score", 0.f),
        m_cap_thresh = Property_<float>::create("cap thresh", 20.f);
        
        gl::MaterialPtr m_mat_rgb_shift;
        std::vector<gl::Fbo> m_fbos;
        
        // current brightness
        float m_brightness = 0.f;
        
        //! ouput warping
        WarpComponent::Ptr m_warp;
        
        // our content
        media::MediaControllerPtr
        m_movie = media::MediaController::create(),
        m_audio = media::MediaController::create();
        
        std::vector<std::string> m_audio_paths;
        bool m_assets_found = false;
        
        CapacitiveSensor m_cap_sense;
        bool m_cap_sense_activated = false;
        
        bool change_state(State the_the_state, bool force_change = false);
        
        void read_bio_sensor(float time_delta);
        
        void update_bio_visuals(float accel, float elong);
        
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
