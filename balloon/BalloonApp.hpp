// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  balloon.hpp
//
//  Created by Fabian on 03/10/18.

#pragma once

#include "app/ViewerApp.hpp"
#include "media/media.h"
#include "syphon/SyphonConnector.h"

namespace kinski
{
    class BalloonApp : public ViewerApp
    {
    public:
        enum class GamePhase{IDLE, FLOATING, CRASHED};
        
    private:
        
        enum AnimationEnum
        {
            ANIM_FOREGROUND_IN = 0, ANIM_FOREGROUND_OUT = 1,
            ANIM_ZED_DROP = 2, ANIM_ZED_DROP_RECOVER = 3
        };
        
        GamePhase m_game_phase = GamePhase::IDLE;
        
        syphon::Output m_syphon_out;

        gl::CameraPtr m_2d_cam = gl::OrthoCamera::create(-1.f, 1.f, -1.f, 1.f, -100.f, 100.f);
        
        media::MediaControllerPtr m_sprite_movie = media::MediaController::create();
        gl::Texture m_sprite_texture;
        std::vector<gl::FboPtr> m_blur_fbos;

        std::vector<gl::Texture> m_parallax_textures, m_balloon_textures;
        std::vector<gl::MeshPtr> m_parallax_meshes;
        gl::MeshPtr m_sprite_mesh, m_bg_mesh, m_fg_mesh, m_balloon_lines_mesh;

        gl::FboPtr m_offscreen_fbo;
        
        // animated values
        uint32_t m_current_num_balloons = 0;
        float m_current_float_speed = 0.f;
        glm::vec2 m_zed_offset;
        
        bool m_dirty_scene = true;
        
        // timer objects;
        Timer m_balloon_timer;
        
        Property_<uint32_t>::Ptr
        m_max_num_balloons = Property_<uint32_t>::create("max num balloons", 10);

        Property_<string>::Ptr
        m_asset_dir = Property_<string>::create("asset directory");

        Property_<glm::vec2>::Ptr
        m_sprite_size = Property_<glm::vec2>::create("sprite size", glm::vec2(160, 300)),
        m_balloon_noise_intensity = Property_<glm::vec2>::create("balloon noise intensity", glm::vec2(80, 100)),
        m_balloon_noise_speed = Property_<glm::vec2>::create("balloon noise speed", glm::vec2(.6f, 1.f));

        Property_<float>::Ptr
        m_float_speed = RangedProperty<float>::create("float speed", 1.f, -1.f, 1.f),
        m_parallax_factor = RangedProperty<float>::create("parallax factor", 1.618f, 1.f, 10.f),
        m_motion_blur = RangedProperty<float>::create("motion blur", 0.f, 0.f, 1.f),
        m_timeout_balloon_explode = RangedProperty<float>::create("timeout balloon explode", 3.f, 0.f, 5.f),
        m_balloon_scale = RangedProperty<float>::create("balloon scale", .5f, 0.f, 5.f);

        Property_<glm::ivec2>::Ptr
        m_offscreen_res = Property_<glm::ivec2>::create("offscreen resolution", glm::ivec2(1920, 1080));

        Property_<bool>::Ptr
        m_use_syphon = Property_<bool>::create("use syphon", false);

        gl::MeshPtr create_sprite_mesh(const gl::Texture &t = gl::Texture());
        
        void create_scene();
        
        void create_balloon_cloud();
        
        void rumble_balloons();
        
        void create_timers();
        
        void create_animations();
        
        void update_balloon_cloud(float the_delta_time);
        
        void explode_balloon();
        
        bool change_gamephase(GamePhase the_next_phase);
        
        bool is_state_change_valid(GamePhase the_phase, GamePhase the_next_phase);
        
        struct balloon_particle_t
        {
            glm::vec2 position;
            glm::vec2 velocity;
            float radius;
            float string_length;
        };
        std::deque<balloon_particle_t> m_balloon_particles;

    public:
        
        BalloonApp(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
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
        void update_property(const Property::ConstPtr &the_property) override;
    };
}// namespace kinski

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::BalloonApp>(argc, argv);
    LOG_INFO<<"local ip: " << kinski::net::local_ip();
    return theApp->run();
}
