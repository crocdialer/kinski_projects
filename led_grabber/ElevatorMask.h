//
// Created by crocdialer on 9/21/19.
//

#pragma once

#include "gl/Fbo.hpp"
#include "gl/Scene.hpp"
#include "gl/Noise.hpp"
#include "MaskGenerator.hpp"

namespace kinski::gl
{

DEFINE_CLASS_PTR(ElevatorMask)

class ElevatorMask : public MaskGenerator
{
public:

    static ElevatorMaskPtr create(const glm::vec2 &res);

    void update(float delta_time) override;

    gl::Texture texture() const override;

    void set_resolution(const glm::vec2 &res);

    void set_speed(float s){ m_speed = s; }

    void set_spawn_frequency(float f){ m_spawn_frequency = f; }

private:

    struct line_t
    {
        float position;
        float thickness;
    };

    ElevatorMask() = default;

    gl::FboPtr m_fbo;

    glm::vec2 m_resolution = glm::vec2(640, 480);

    float m_speed = .05f;

    float m_spawn_frequency = 2.f;
    float m_next_spawn_time = 0.f;
    float m_time_accum = 0.0;

    std::deque<line_t> m_lines =
            {
                    {.2f, .05f},
                    {.7f, .125f},
            };

//    std::normal_distribution
};

}

