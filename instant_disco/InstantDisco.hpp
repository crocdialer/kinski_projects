// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2017, Fabian Schmidt <crocdialer@googlemail.com>
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  InstantDisco.hpp
//
//  Created by Fabian on 11/04/17.

#pragma once

#include "app/ViewerApp.hpp"

// modules
#include "media/media.h"
#include "dmx/dmx.h"

namespace kinski
{
    class InstantDisco : public ViewerApp
    {
    private:
        enum FontEnum{FONT_NORMAL = 0, FONT_LARGE = 1};
//        enum TextureEnum{TEXTURE_MUSIC = 0, TEXTURE_STROBO = 1, TEXTURE_FOG = 2};

        gl::Object3DPtr m_buttons = gl::Object3D::create();
        gl::Object3DPtr m_audio_icon, m_strobo_icon, m_discoball_icon, m_fog_icon;

        std::map<gl::Object3DPtr, std::function<void()>> m_action_map;

        media::MediaControllerPtr m_media = media::MediaController::create();
        dmx::DMXController m_dmx{background_queue().io_service()};

        Timer m_timer_strobo, m_timer_disco_ball, m_timer_fog, m_timer_led, m_timer_audio_restart;

        uint32_t m_num_button_presses = 0;
        Stopwatch m_stop_watch;

        Property_<std::string>::Ptr
        m_media_path = Property_<std::string>::create("media path", ""),
        m_strobo_dmx_values = Property_<std::string>::create("stroboscope dmx values", ""),
        m_discoball_dmx_values = Property_<std::string>::create("discoball dmx values", ""),
        m_fog_dmx_values = Property_<std::string>::create("fog dmx values", "");

        Property_<bool>::Ptr
        m_audio_enabled = Property_<bool>::create("audio enabled", false),
        m_strobo_enabled = Property_<bool>::create("strobo enabled", false),
        m_discoball_enabled = Property_<bool>::create("discoball enabled", false),
        m_fog_enabled = Property_<bool>::create("fog enabled", false),
        m_led_enabled = Property_<bool>::create("LED enabled", false),
        m_button_pressed = Property_<bool>::create("button pressed", false);

        Property_<float>::Ptr
        m_timout_strobo = Property_<float>::create("strobo timeout", 2.f),
        m_timout_discoball = Property_<float>::create("discoball timeout", 4.f),
        m_timout_fog = Property_<float>::create("strobo timeout", 8.f),
        m_timout_audio_rewind = Property_<float>::create("audio rewind timeout", 30.f);

        static void button_ISR();

    public:
        InstantDisco(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
        void setup() override;
        void update(float timeDelta) override;
        void draw() override;
        void resize(int w ,int h) override;
        void key_press(const KeyEvent &e) override;
        void key_release(const KeyEvent &e) override;
        void mouse_press(const MouseEvent &e) override;
        void mouse_release(const MouseEvent &e) override;
        void mouse_move(const MouseEvent &e) override;
        void mouse_drag(const MouseEvent &e) override;
        void mouse_wheel(const MouseEvent &e) override;
        void touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void file_drop(const MouseEvent &e, const std::vector<std::string> &files) override;
        void teardown() override;
        void update_property(const Property::ConstPtr &theProperty) override;
    };
}// namespace kinski

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::InstantDisco>(argc, argv);
    LOG_INFO<<"local ip: " << kinski::net::local_ip();
    return theApp->run();
}
