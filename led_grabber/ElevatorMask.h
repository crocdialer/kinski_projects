//
// Created by crocdialer on 9/21/19.
//

#pragma once

#include "gl/Fbo.hpp"
#include "gl/Scene.hpp"

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

private:

    struct line_t
    {
        float position;
        float thickness;
    };

    ElevatorMask() = default;

    float m_velocity = 1.2f;
    gl::FboPtr m_fbo;

    std::vector<line_t> m_lines =
            {
                    {.2f, .05f},
            };
};

}

