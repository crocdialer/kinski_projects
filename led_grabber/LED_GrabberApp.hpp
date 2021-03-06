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
#include "MatrixMask.hpp"
#include "ElevatorMask.h"

using namespace crocore;

namespace kinski
{
class LED_GrabberApp : public ViewerApp
{
private:

    enum TextureEnum
    {
        TEXTURE_INPUT = 0, TEXTURE_CAM_INPUT = 1, TEXTURE_OUTPUT = 2,
        TEXTURE_DOWNSAMPLE = 3, TEXTURE_MASK = 4, TEXTURE_WHITE = 5
    };

    enum RunMode
    {
        MODE_DEFAULT, MODE_MANUAL_CALIBRATION
    };

    RunMode m_runmode = MODE_DEFAULT;

    size_t m_current_calib_segment = 0;

    gl::vec2 m_last_calib_click;

    std::vector<gl::vec2> m_points;
    LED_GrabberPtr m_led_grabber = LED_Grabber::create();
    ImagePtr m_image_input;
    gl::FboPtr m_fbo_downsample;
    Timer m_led_update_timer, m_device_scan_timer, m_timer_config_reset;

    gl::MatrixMaskPtr m_matrix_mask = gl::MatrixMask::create();
    gl::ElevatorMaskPtr m_elevator_mask = gl::ElevatorMask::create({640, 480});
    gl::Noise m_noise;
    crocore::ConnectionPtr m_lora_gateway;

    media::MediaControllerPtr m_media = media::MediaController::create();
    media::CameraControllerPtr m_camera = media::CameraController::create();

    bool m_reload_media = false, m_needs_redraw = true;
    int m_is_syncing = 0;
    Timer m_broadcast_timer, m_sync_timer, m_sync_off_timer, m_scan_media_timer, m_check_ip_timer;

    net::udp_server m_udp_server;
    std::unordered_map<std::string, float> m_ip_timestamps;
    std::unordered_map<std::string, CircularBuffer<double>> m_ip_roundtrip;

    std::vector<std::string> m_playlist;
    uint32_t m_current_playlist_index = 0;

    std::string m_ip_adress;

    // properties
    Property_<std::string>::Ptr
            m_media_path = Property_<std::string>::create("media path", ""),
            m_calib_image_path = Property_<std::string>::create("calib image path", "");

    Property_<bool>::Ptr
            m_scale_to_fit = Property_<bool>::create("scale_to_fit", false),
            m_loop = Property_<bool>::create("loop", false),
            m_auto_play = Property_<bool>::create("autoplay", true),
            m_force_audio_jack = Property_<bool>::create("force 3.5mm audio-jack", false),
            m_use_discovery_broadcast = Property_<bool>::create("use discovery broadcast", true),
            m_is_master = Property_<bool>::create("is master", false),
            m_show_cam_overlay = Property_<bool>::create("show camera overlay", false);

    Property_<float>::Ptr
            m_playback_speed = Property_<float>::create("playback speed", 1.f),
            m_volume = RangedProperty<float>::create("volume", 1.f, 0.f, 1.f),
            m_brightness = RangedProperty<float>::create("brightness", 1.f, 0.f, 2.f);

    Property_<int>::Ptr
            m_broadcast_port = RangedProperty<int>::create("discovery broadcast port", 55555, 0, 65535);

    Property_<int>::Ptr
            m_cam_index = RangedProperty<int>::create("camera index", 0, 0, 9),
            m_calibration_thresh = RangedProperty<int>::create("calibration thresh", 245, 0, 255);

    Property_<gl::Color>::Ptr
            m_led_channels = Property_<gl::Color>::create("LED channel brightness",
                                                          gl::Color(0.4f, 0.4f, 0.4f, 0.2f)),
            m_led_calib_color = Property_<gl::Color>::create("LED calibration color",
                                                             gl::Color(0.f, 0.f, 0.f, 0.7f));

    Property_<gl::vec2>::Ptr
            m_led_res = Property_<gl::vec2>::create("LED resolution", gl::vec2(58, 14)),
            m_led_unit_res = Property_<gl::vec2>::create("LED unit resolution", gl::vec2(58, 14)),
            m_downsample_res = Property_<gl::vec2>::create("downsample resolution", gl::vec2(320, 240));

    Property_<std::vector<gl::vec2>>::Ptr
            m_calibration_points = Property_<std::vector<gl::vec2>>::create("calibration points");

    Property_<std::string>::Ptr
            m_led_proxy_ip = Property_<std::string>::create("led proxy ip", "");

    // overlay color
    Property_<gl::Color>::Ptr
            m_overlay_color = Property_<gl::Color>::create("overlay color", gl::COLOR_WHITE),
            m_overlay_color_alt = Property_<gl::Color>::create("overlay color alternative", gl::COLOR_WHITE);

    // mask generation properties
    Property_<bool>::Ptr
            m_use_masking = Property_<bool>::create("use masking", false),
            m_use_noise = Property_<bool>::create("use noise", true),
            m_elevator_button = Property_<bool>::create("elevator button", false);

    Property_<glm::ivec2>::Ptr
            m_mask_grid_size = Property_<glm::ivec2>::create("mask gridsize", {4, 2});

    Property_<glm::vec2>::Ptr
            m_mask_lifetime = Property_<glm::vec2>::create("mask lifetime", {1.f, 2.5f});

    Property_<float>::Ptr
            m_mask_rate = Property_<float>::create("mask rate", 2.f),
            m_noise_speed = Property_<float>::create("noise speed", 0.3f),
            m_elevator_speed = RangedProperty<float>::create("elevator speed", 0.3f, -1.f, 1.f),
            m_elevator_spawn_frequency = RangedProperty<float>::create("elevator spawn frequency", 2.3f, .1f, 10.f),
            m_elevator_lines_thickness = RangedProperty<float>::create("elevator line thickness", 0.05f, 0.f, .5f),
            m_elevator_tilt = RangedProperty<float>::create("elevator tilt", 0.f, -45.f, 45.f);

    Property_<glm::vec2>::Ptr
            m_noise_scale = Property_<glm::vec2>::create("noise scale", glm::vec2(0.05f));

    Property_<std::string>::Ptr
            m_lora_gateway_url = Property_<std::string>::create("lora gateway url", "localhost");

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

    void playlist_next();

    void playlist_prev();

    void playlist_track(size_t the_index);

    // led calib
    void set_runmode(RunMode);

    void process_calib_click(const gl::vec2 &the_click_pos);

    void search_devices();

    void lora_gateway_receive(net::tcp_connection_ptr con, const std::vector<uint8_t> &data);

public:

    explicit LED_GrabberApp(int argc = 0, char *argv[] = nullptr) : ViewerApp(argc, argv){};

    void setup() override;

    void update(float delta_time) override;

    void draw() override;

    void resize(int w, int h) override;

    void key_press(const KeyEvent &e) override;

    void key_release(const KeyEvent &e) override;

    void mouse_press(const MouseEvent &e) override;

    void mouse_release(const MouseEvent &e) override;

    void mouse_move(const MouseEvent &e) override;

    void mouse_drag(const MouseEvent &e) override;

    void mouse_wheel(const MouseEvent &e) override;

    void touch_begin(const MouseEvent &e, const std::set<const Touch *> &the_touches) override;

    void touch_end(const MouseEvent &e, const std::set<const Touch *> &the_touches) override;

    void touch_move(const MouseEvent &e, const std::set<const Touch *> &the_touches) override;

    void file_drop(const MouseEvent &e, const std::vector<std::string> &files) override;

    void teardown() override;

    void update_property(const PropertyConstPtr &theProperty) override;

    bool needs_redraw() const override;

};
}// namespace kinski

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::LED_GrabberApp>(argc, argv);
    return theApp->run();
}
