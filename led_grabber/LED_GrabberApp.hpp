// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  LED_GrabberApp.hpp
//
//  Created by Fabian on 29/01/14.

#pragma once

#include "app/ViewerApp.hpp"
#include "gl/Texture.hpp"

#include "media/media.h"
#include "LED_Grabber.hpp"

namespace kinski
{
    class LED_GrabberApp : public ViewerApp
    {
    private:

        enum TextureEnum{TEXTURE_INPUT = 0, TEXTURE_CAM_INPUT = 1, TEXTURE_OUTPUT = 2,
            TEXTURE_LEDS = 3};
        
        std::vector<gl::vec2> m_points;
        LED_GrabberPtr m_led_grabber = LED_Grabber::create();
        ImagePtr m_image_input;
        gl::Fbo m_fbo_downsample;
        Timer m_led_update_timer;
        
        media::MediaControllerPtr m_media = media::MediaController::create();
        media::CameraControllerPtr m_camera = media::CameraController::create();
        
        bool m_reload_media = false, m_needs_redraw = true;
        int m_is_syncing = 0;
        Timer m_broadcast_timer, m_sync_timer, m_sync_off_timer, m_scan_media_timer, m_check_ip_timer;
        
        net::udp_server m_udp_server;
        std::unordered_map<std::string, float> m_ip_timestamps;
        std::unordered_map<std::string, CircularBuffer<double>> m_ip_roundtrip;
        
        std::vector<string> m_playlist;
        uint32_t m_current_playlist_index = 0;
        
        std::string m_ip_adress;
        
        // properties
        Property_<string>::Ptr m_media_path = Property_<string>::create("media path", "");
        Property_<bool>::Ptr
        m_scale_to_fit = Property_<bool>::create("scale_to_fit", false),
        m_loop = Property_<bool>::create("loop", false),
        m_auto_play = Property_<bool>::create("autoplay", true),
        m_force_audio_jack = Property_<bool>::create("force 3.5mm audio-jack", false),
        m_use_discovery_broadcast = Property_<bool>::create("use discovery broadcast", true),
        m_is_master = Property_<bool>::create("is master", false);
        
        Property_<float>::Ptr
        m_playback_speed = Property_<float>::create("playback speed", 1.f),
        m_volume = RangedProperty<float>::create("volume", 1.f, 0.f , 1.f),
        m_brightness = RangedProperty<float>::create("brightness", 1.f, 0.f , 2.f);
        
        Property_<uint32_t>::Ptr
        m_broadcast_port = Property_<uint32_t>::create("discovery broadcast port", 55555);
        
        Property_<uint32_t>::Ptr
        m_cam_index = Property_<uint32_t>::create("camera index", 0),
        m_calibration_thresh = RangedProperty<uint32_t>::create("calibration thresh", 245, 0, 255);
        
        Property_<gl::Color>::Ptr
        m_led_channels = Property_<gl::Color>::create("LED channel brightness",
                                                     gl::Color(0.4f, 0.4f, 0.4f, 0.2f)),
        m_led_calib_color = Property_<gl::Color>::create("LED calibration color",
                                                         gl::Color(0.f, 0.f, 0.f, 0.7f));
        
        Property_<gl::vec2>::Ptr
        m_led_res = Property_<gl::vec2>::create("LED resolution", gl::vec2(58, 7)),
        m_downsample_res = Property_<gl::vec2>::create("downsample resolution", gl::vec2(320, 240));
        
        Property_<std::vector<gl::vec2>>::Ptr
        m_calibration_points = Property_<std::vector<gl::vec2>>::create("calibration points");
        
        std::string secs_to_time_str(float the_secs) const;
        void setup_rpc_interface();
        void reload_media();
        void sync_media_to_timestamp(double the_timestamp);
        void remove_dead_ip_adresses();
        void begin_network_sync();
        void send_sync_cmd();
        void send_network_cmd(const std::string &the_cmd);
        void ping_delay(const std::string &the_ip);
        void create_playlist(const std::string &the_base_dir);
        
    public:

        LED_GrabberApp(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
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
        
        bool needs_redraw() const override;

    };
}// namespace kinski

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::LED_GrabberApp>(argc, argv);
    return theApp->run();
}
