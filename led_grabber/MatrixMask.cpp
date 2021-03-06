//
//  MatrixMask.cpp
//  led_grabber
//
//  Created by Fabian on 17.09.18.
//

#include "MatrixMask.hpp"

namespace kinski::gl
{

MatrixMaskPtr MatrixMask::create(const glm::ivec2 &size)
{
    return MatrixMaskPtr(new MatrixMask(size));
}

MatrixMask::MatrixMask(const glm::ivec2 &the_size) :
        m_size(the_size)
{

}

///////////////////////////////////////////////////////////////////////////////

void MatrixMask::update(float the_delta_time)
{
    // subtract delta_time from lifetimes
    for(auto &c : m_grid_cells){ c.life_time = std::max(c.life_time - the_delta_time, 0.f); }

    m_grid_cells.resize(m_size.x * m_size.y, {0.f});

    // fill accumulator
    m_accumulator += the_delta_time * m_rate;
    uint32_t num_new_cells = m_accumulator;
    m_accumulator -= num_new_cells;

    // generate random timestamps for new cells
    for(uint32_t i = 0; i < num_new_cells; ++i)
    {
        auto cell_index = crocore::random_int<uint32_t>(0, m_grid_cells.size() - 1);
        m_grid_cells[cell_index].life_time = crocore::random(m_lifetime_min, m_lifetime_max);
    }

    // generate image
    uint8_t pixels[m_grid_cells.size()];

    for(uint32_t i = 0; i < m_grid_cells.size(); ++i)
    {
        pixels[i] = m_grid_cells[i].life_time > 0.f ? 255 : 0;
    }

    // generate texture
#if defined(KINSKI_GLES)
    uint32_t color_format = GL_LUMINANCE;
    gl::Texture::Format fmt;
    fmt.internal_format = color_format;
    m_texture = gl::Texture(pixels, color_format, m_size.x, m_size.y, fmt);
#else
    uint32_t color_format = GL_RED;
    m_texture.update(pixels, color_format, m_size.x, m_size.y);
#endif
    m_texture.set_mag_filter(GL_NEAREST);
}

///////////////////////////////////////////////////////////////////////////////

glm::ivec2 MatrixMask::size() const
{
    return m_size;
}

///////////////////////////////////////////////////////////////////////////////

void MatrixMask::set_size(const glm::ivec2 &the_size)
{
    m_size = the_size;
}

///////////////////////////////////////////////////////////////////////////////

gl::Texture MatrixMask::texture() const
{
    return m_texture;
}

///////////////////////////////////////////////////////////////////////////////

}
