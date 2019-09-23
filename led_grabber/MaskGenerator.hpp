//
// Created by crocdialer on 9/21/19.
//

#pragma once

#include <gl/Texture.hpp>

namespace kinski::gl
{

class MaskGenerator
{
    virtual void update(float delta_time) = 0;

    virtual gl::Texture texture() const = 0;
};

}
