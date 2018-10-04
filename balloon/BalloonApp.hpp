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

namespace kinski
{
    class BalloonApp : public ViewerApp
    {
    private:
        std::vector<gl::Texture> m_bg_textures;

        gl::Texture m_balloon_texture;

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

        gl::MeshPtr create_sprite_mesh(const gl::Texture &t);
        void create_background_mesh();

        void create_scene();

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
