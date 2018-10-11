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
    private:
        syphon::Output m_syphon_out;

        gl::CameraPtr m_2d_cam = gl::OrthoCamera::create(-1.f, 1.f, -1.f, 1.f, -100.f, 100.f);

        gl::Texture m_sprite_texture;
        std::vector<gl::FboPtr> m_blur_fbos;

        std::vector<gl::Texture> m_bg_textures;
        std::vector<gl::MeshPtr> m_bg_meshes;
        gl::MeshPtr m_sprite_mesh;

        gl::FboPtr m_offscreen_fbo;

        uint32_t m_current_num_balloons;

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
        m_parallax_factor = RangedProperty<float>::create("parallax factor", 1.618f, 1.f, 10.f);

        Property_<glm::ivec2>::Ptr
        m_offscreen_res = Property_<glm::ivec2>::create("offscreen resolution", glm::ivec2(1920, 1080));

        Property_<bool>::Ptr
        m_use_syphon = Property_<bool>::create("use syphon", false);

        gl::MeshPtr create_sprite_mesh(const gl::Texture &t = gl::Texture());

        void create_scene();

        void explode_balloon();

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
