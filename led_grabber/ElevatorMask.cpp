//
// Created by crocdialer on 9/21/19.
//

#include "ElevatorMask.h"

namespace kinski::gl
{

ElevatorMaskPtr ElevatorMask::create(const glm::vec2 &res)
{
    auto ret = kinski::gl::ElevatorMaskPtr(new ElevatorMask());
    return ret;
}

void ElevatorMask::update(float delta_time)
{
    // init if necessary
    if(!m_fbo){ set_resolution(m_resolution); }

    // increment accum
    m_time_accum += delta_time;
    m_next_spawn_time -= delta_time;

    std::deque<line_t> new_lines;

    if(m_next_spawn_time <= 0.f && m_speed != 0.f)
    {
        line_t l = {};
        l.thickness = 0.05;
        l.position = m_speed > 0.f ? -l.thickness / 2.f : 1.f + l.thickness / 2.f;

        new_lines.push_back(l);

        // randomize
        m_next_spawn_time = std::min(1.f / m_spawn_frequency, 10.f);
    }

    // spawn / destroy objects
    auto line_it = m_lines.begin();
    for(; line_it != m_lines.end(); ++line_it)
    {
        auto &l = *line_it;

        l.position += m_speed * delta_time;

        if(m_speed >= 0.f && l.position > 1.f + l.thickness / 2.f){ continue; }
        if(m_speed < 0.f && l.position < -l.thickness / 2.f){ continue; }

        new_lines.push_back(l);
    }
//    m_lines.erase(line_it, m_lines.end());
    m_lines = std::move(new_lines);

    gl::render_to_texture(m_fbo, [this]
    {
        gl::reset_state();
        gl::clear();

        for(const auto line : m_lines)
        {
            auto line_scaled = line;
            line_scaled.position = (1.f - line_scaled.position) * m_fbo->height();
            line_scaled.thickness *= m_fbo->height();

            gl::draw_quad({m_fbo->width() * 3, line_scaled.thickness}, gl::COLOR_WHITE,
                          {-m_fbo->width(), line_scaled.position - line_scaled.thickness / 2.f});
        }
    });
}

gl::Texture ElevatorMask::texture() const
{
    if(m_fbo){ return m_fbo->texture(); }
    return gl::Texture();
}

void ElevatorMask::set_resolution(const glm::vec2 &res)
{
    m_resolution = res;

    gl::Fbo::Format fmt = {};
#ifdef KINSKI_GLES_2
    fmt.color_internal_format = GL_LUMINANCE;
#else
    fmt.color_internal_format = GL_R8;
#endif

    m_fbo = gl::Fbo::create(m_resolution, fmt);
    m_fbo->texture().set_flipped();
}

}