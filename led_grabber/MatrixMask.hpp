//
//  MatrixMask.hpp
//  led_grabber
//
//  Created by Fabian on 17.09.18.
//

#pragma once

#include "MaskGenerator.hpp"

namespace kinski::gl
{

DEFINE_CLASS_PTR(MatrixMask)

class MatrixMask : public MaskGenerator
{
public:

    static MatrixMaskPtr create(const glm::ivec2 &size = glm::ivec2(3));

    void update(float the_delta_time) override;

    gl::Texture texture() const override;

    //!
    glm::ivec2 size() const;

    //!
    void set_size(const glm::ivec2 &the_size);

    void set_rate(float the_intensity){ m_rate = the_intensity; }

    float rate() const{ return m_rate; };

    void set_lifetime(float the_min, float the_max)
    {
        m_lifetime_min = the_min;
        m_lifetime_max = the_max;
    }

private:

    explicit MatrixMask(const glm::ivec2 &size);

    struct gridcell_t
    {
        float life_time;
    };

    uint32_t m_dirty_bits = 0;
    glm::ivec2 m_size;
    std::vector<gridcell_t> m_grid_cells;
    gl::Texture m_texture;
    gl::MeshPtr m_overlay_mesh;
    float m_accumulator = 0.f;
    float m_rate = 1.f;
    float m_lifetime_min = 0.f, m_lifetime_max = 1.f;
};

};
