// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  ParticleSample.hpp
//
//  Created by Fabian on 29/01/14.

#pragma once

#include "app/ViewerApp.hpp"

// module headers
#include "opencl/ParticleSystem.hpp"

using namespace crocore;

namespace kinski
{
    class ParticleSample : public ViewerApp
    {
    private:

        enum TextureEnum{TEXTURE_PARTICLE = 0};

        Property_<bool>::Ptr m_draw_fps = Property_<bool>::create("draw fps", true);
        Property_<std::string>::Ptr m_texture_path = Property_<std::string>::create("texture path", "");
        
        RangedProperty<int>::Ptr
        m_num_particles = RangedProperty<int>::create("num particles", 100000, 1, 10000000),
        m_emission_rate = RangedProperty<int>::create("spawn rate", 1000, 0, 1000000),
        m_num_burst_particles = RangedProperty<int>::create("num burst particles", 50000, 0, 10000000);
        
        RangedProperty<float>::Ptr m_point_size = RangedProperty<float>::create("point size", 3.f, 1.f, 64.f);
        Property_<gl::Color>::Ptr m_point_color = Property_<gl::Color>::create("point color", gl::COLOR_WHITE);

        Property_<gl::vec3>::Ptr
        m_gravity = Property_<gl::vec3>::create("gravity", gl::vec3(0, -9.81f, 0)),
        m_start_velocity_min = Property_<gl::vec3>::create("start velocity min", gl::vec3(-5, 20, -5)),
        m_start_velocity_max = Property_<gl::vec3>::create("start velocity max", gl::vec3(5, 25, 5));

        RangedProperty<float>::Ptr
        m_lifetime_min = RangedProperty<float>::create("lifetime min", 3.f, 0.f, 100.f),
        m_lifetime_max = RangedProperty<float>::create("lifetime max", 5.f, 0.f, 100.f),
        m_bouncyness = RangedProperty<float>::create("bouncyness", .2f, 0.f, 1.f);

        Property_<bool>::Ptr
        m_debug_life = Property_<bool>::create("debug life", false),
        m_use_contraints = Property_<bool>::create("use contraints", true);

        gl::ParticleSystemPtr m_particle_system = gl::ParticleSystem::create();
        gl::MeshPtr m_particle_mesh;

        void init_particles(uint32_t the_num);

        bool m_needs_init = true;

    public:
        ParticleSample(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
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
    };
}// namespace kinski

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::ParticleSample>(argc, argv);
    LOG_INFO<<"local ip: " << crocore::net::local_ip();
    return theApp->run();
}
