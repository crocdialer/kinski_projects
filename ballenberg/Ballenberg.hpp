//
//  Ballenberg.h
//  gl
//
//  Created by Fabian on 16/03/16.
//
//
#pragma once

#include "app/ViewerApp.h"
#include "app/WarpComponent.h"
#include "core/Timer.hpp"
#include "core/Serial.hpp"

// modules
#include "video/video.h"
#include "cap_sense/cap_sense.h"
//#include "audio/audio.h"

namespace kinski
{
    class Ballenberg : public ViewerApp
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
        
        Property_<string>::Ptr
        m_asset_base_dir = Property_<string>::create("asset base directory", "~/Desktop/ballenberg_assets"),
        m_cap_sense_dev_name = Property_<string>::create("touch sensor device"),
        m_motion_sense_dev_name = Property_<string>::create("motion sensor device");
        
        Serial m_motion_sense;
        std::vector<uint8_t> m_serial_buf;
        bool m_motion_detected = false;
        
        Property_<float>::Ptr
        m_volume = Property_<float>::create("volume", 1.f);
        
        //! ouput warping
        WarpComponent::Ptr m_warp;
        
        // our content
//        audio::SoundPtr m_audio;
        
        CapacitiveSensor m_cap_sense;
        
        bool change_state(State the_the_state, bool force_change = false);
        
        //! read our motion sensor, update m_motion_detected member, start timer to reset val
        void detect_motion();
        
        //! display sensor and application state
        void draw_status_info();
        
        bool load_assets();
        
    public:
        
        Ballenberg(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
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