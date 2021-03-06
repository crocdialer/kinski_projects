// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  MediaPlayer.hpp
//
//  Created by Fabian on 29/01/14.

#pragma once

#include "app/ViewerApp.hpp"
#include "gl/Texture.hpp"

#include "media/media.h"

using namespace crocore;

namespace kinski
{
class MediaPlayer : public ViewerApp
{
private:

    enum TextureEnum{TEXTURE_INPUT = 0, TEXTURE_OUTPUT = 1, TEXTURE_MASK = 2};

    media::MediaControllerPtr m_media = media::MediaController::create();
    media::CameraControllerPtr m_camera_control = media::CameraController::create();
    bool m_reload_media = false, m_needs_redraw = true;
    int m_is_syncing = 0;
    Timer m_broadcast_timer, m_sync_timer, m_sync_off_timer, m_scan_media_timer, m_check_ip_timer;

    net::udp_server m_udp_server;
//    std::unordered_map<std::std::string, float> m_ip_timestamps;
//    std::unordered_map<std::std::string, CircularBuffer<double>> m_ip_roundtrip;

    std::string m_ip_adress;

    // properties
    Property_<std::string>::Ptr
    m_media_path = Property_<std::string>::create("media path", ""),
    m_text_overlay = Property_<std::string>::create("text overlay", "");

    Property_<bool>::Ptr
    m_scale_to_fit = Property_<bool>::create("scale_to_fit", false),
    m_loop = Property_<bool>::create("loop", false),
    m_auto_play = Property_<bool>::create("autoplay", true),
    m_force_audio_jack = Property_<bool>::create("force 3.5mm audio-jack", false),
    m_use_discovery_broadcast = Property_<bool>::create("use discovery broadcast", true),
    m_is_master = Property_<bool>::create("is master", false);

    Property_<float>::Ptr
    m_current_delay = Property_<float>::create("current delay", 0.f),
    m_playback_speed = Property_<float>::create("playback speed", 1.f),
    m_volume = RangedProperty<float>::create("volume", 1.f, 0.f , 1.f),
    m_brightness = RangedProperty<float>::create("brightness", 1.f, 0.f , 2.f),
    m_gamma = RangedProperty<float>::create("gamma", 1.f, 0.f , 4.f);

    Property_<int>::Ptr
    m_broadcast_port = Property_<int>::create("discovery broadcast port", 55555);

    Property_<std::vector<std::string>>::Ptr
    m_playlist = Property_<std::vector<std::string>>::create("playlist"),
    m_ip_adresses = Property_<std::vector<std::string>>::create("ip adresses", {"reibach.local"});

    Property_<std::vector<float>>::Ptr
    m_delays = Property_<std::vector<float>>::create("delays");

    Property_<int>::Ptr
    m_playlist_index = Property_<int>::create("playlist index", -1);

    std::string secs_to_time_str(float the_secs) const;
    float time_str_to_secs(const std::string &the_str) const;

    void setup_rpc_interface();
    void reload_media();
    void sync_media_to_timestamp(double the_timestamp);
//    void remove_dead_ip_adresses();
    void begin_network_sync();
    void send_sync_cmd();
    void send_network_cmd(const std::string &the_cmd);
//    void ping_delay(const std::std::string &the_ip);

    void create_playlist(const std::string &the_base_dir);
    void playlist_next();
    void playlist_prev();
    void playlist_track(size_t the_index);
    const std::vector<std::string>& playlist() const { return m_playlist->value(); }
    void set_playlist(const std::vector<std::string> &the_playlist){ m_playlist->set(the_playlist); };

public:

    MediaPlayer(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
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
    void update_property(const PropertyConstPtr &theProperty) override;

    bool needs_redraw() const override;

};
}// namespace kinski

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::MediaPlayer>(argc, argv);
    return theApp->run();
}
