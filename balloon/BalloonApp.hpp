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

using namespace crocore;

namespace kinski
{
    class BalloonApp : public ViewerApp
    {
    public:
        enum class GamePhase{UNDEFINED, IDLE, FLOATING, CRASHED};
        
    private:
        
        enum AnimationEnum
        {
            ANIM_FOREGROUND_IN = 0, ANIM_FOREGROUND_OUT = 1,
            ANIM_ZED_DROP = 2, ANIM_ZED_DROP_RECOVER = 3,
            ANIM_ZED_IN = 4, ANIM_ZED_OUT = 5,
            ANIM_TITLE_IN = 6, ANIM_TITLE_OUT = 7,
            ANIM_TOMBSTONE_IN = 8, ANIM_TOMBSTONE_OUT = 9,
        };
        
        GamePhase m_game_phase = GamePhase::UNDEFINED;
        
        syphon::Output m_syphon_out;

        gl::CameraPtr m_2d_cam = gl::OrthoCamera::create(-1.f, 1.f, -1.f, 1.f, -100.f, 100.f);

        int m_current_sprite_index = -1;
        float m_current_sprite_frame = 0, m_current_pow_frame = 0;
        float m_pow_fps = 1.f, m_current_sprite_fps = 1.f;

        // balloon explode movie
        std::vector<media::MediaControllerPtr> m_balloon_pow_sounds;

        // zed sounds
        std::vector<media::MediaControllerPtr> m_zed_sounds;

        // corpse movie
        media::MediaControllerPtr m_corpse_movie = media::MediaController::create();

        // background wind audio
        media::MediaControllerPtr m_background_audio = media::MediaController::create();

        // start audio
        media::MediaControllerPtr m_start_audio = media::MediaController::create();

        gl::Texture m_sprite_texture, m_corpse_texture, m_pow_texture, m_tombstone_texture;
        std::vector<gl::FboPtr> m_blur_fbos;

        std::vector<gl::Texture> m_parallax_textures, m_balloon_textures, m_sprite_arrays;
        std::vector<gl::MeshPtr> m_parallax_meshes;
        gl::MeshPtr m_sprite_mesh, m_pow_mesh, m_bg_mesh, m_fg_mesh, m_balloon_lines_mesh, m_corpse_mesh,
            m_title_mesh, m_tombstone_template, m_tomb_balloon;

        gl::Object3DPtr m_credits_handle = gl::Object3D::create("credits");

        gl::Font m_tombstone_font;

        gl::FboPtr m_offscreen_fbo;

        gl::ShaderPtr m_array_shader;

        // animated values
        uint32_t m_current_num_balloons = 0;
        float m_current_float_speed = 0.f;
        glm::vec2 m_zed_offset;

        bool m_assets_loaded = false;
        bool m_dirty_tombs = true, m_dirty_balloon_cloud = true, m_dirty_animations = true;
        
        // timer objects;
        Timer m_balloon_timer;
        
        Property_<uint32_t>::Ptr
        m_max_num_balloons = Property_<uint32_t>::create("max num balloons", 10),
        m_max_num_tombstones = Property_<uint32_t>::create("max num tombstones", 100),
        m_num_dead = Property_<uint32_t>::create("number of deaths", 0),
        m_num_button_pressed = Property_<uint32_t>::create("number of button presses", 0);

        Property_<std::string>::Ptr
        m_asset_dir = Property_<std::string>::create("asset directory");

        Property_<glm::vec2>::Ptr
        m_title_position = Property_<glm::vec2>::create("title position", glm::vec2(0.f, .35f)),
        m_sprite_size = Property_<glm::vec2>::create("sprite size", glm::vec2(160, 300)),
        m_balloon_noise_intensity = Property_<glm::vec2>::create("balloon noise intensity", glm::vec2(80, 110)),
        m_balloon_noise_speed = Property_<glm::vec2>::create("balloon noise speed", glm::vec2(.2f, .3f)),
        m_balloon_line_length = Property_<glm::vec2>::create("balloon line length", glm::vec2(1.f, 1.5f)),
        m_tombstone_scale = Property_<glm::vec2>::create("tombstone scale", glm::vec2(1.5f, 1.6f));

        RangedProperty<float>::Ptr
        m_float_speed_mutliplier = RangedProperty<float>::create("float multiplier", 1.f, 0.f, 10.f),
        m_float_speed = RangedProperty<float>::create("float speed", 1.f, -1.f, 1.f),
        m_parallax_factor = RangedProperty<float>::create("parallax factor", 5.5f, 1.f, 10.f),
        m_motion_blur = RangedProperty<float>::create("motion blur", 1.f, 0.f, 1.f),
        m_timeout_balloon_explode = RangedProperty<float>::create("timeout balloon explode", 3.f, 0.f, 5.f),
        m_balloon_scale = RangedProperty<float>::create("balloon scale", 1.5f, 0.f, 5.f);

        Property_<glm::ivec2>::Ptr
        m_offscreen_res = Property_<glm::ivec2>::create("offscreen resolution", glm::ivec2(1920, 1080));

        Property_<bool>::Ptr
        m_use_syphon = Property_<bool>::create("use syphon", false),
        m_show_credits = Property_<bool>::create("show credits", true),
        m_auto_save = Property_<bool>::create("auto save", false);

        Property_<std::vector<glm::vec2>>::Ptr
        m_crash_sites = Property_<std::vector<glm::vec2>>::create("crash sites");
        
        gl::MeshPtr create_sprite_mesh(const gl::Texture &t = gl::Texture());
        
        void create_scene();
        
        void create_balloon_cloud();

        void create_tombstones();
        
        gl::Color random_balloon_color();
        
        void rumble_balloons();
        
        void create_timers();
        
        void create_animations();

        void create_tombstone_animations(const gl::MeshPtr &the_tombstone_mesh, float duration, float delay);

        void update_balloon_cloud(float the_delta_time);
        
        void explode_balloon();

        gl::MeshPtr add_random_tombstone();
        
        bool change_gamephase(GamePhase the_next_phase);
        
        bool is_state_change_valid(GamePhase the_phase, GamePhase the_next_phase);
        
        gl::MeshPtr create_tombstone_mesh(uint32_t the_index, const glm::vec2 &the_pos);
        
        struct balloon_particle_t
        {
            glm::vec2 position;
            glm::vec2 force;
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
        void update_property(const PropertyConstPtr &the_property) override;
    };
}// namespace kinski

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::BalloonApp>(argc, argv);
    LOG_INFO<<"local ip: " << crocore::net::local_ip();
    return theApp->run();
}
