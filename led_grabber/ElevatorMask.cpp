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
    // TODO: spawn / destroy objects

//    gl::ScopedMatrixPush mv(gl::MODEL_VIEW_MATRIX), proj(gl::PROJECTION_MATRIX);
//
//    gl::load_identity(gl::MODEL_VIEW_MATRIX);
//    gl::load_matrix(glm::ortho(0.f, 1.f,), PROJECTION_MATRIX)
    if(!m_fbo){ set_resolution({640, 480}); }

    for(auto &l : m_lines)
    {
        l.position += m_velocity * delta_time;
        if(l.position > 2.f){ l.position = -1.f; }
    }

    gl::render_to_texture(m_fbo, [this]
    {
        gl::clear();

        for(const auto line : m_lines)
        {
            auto line_scaled = line;
            line_scaled.position *= m_fbo->height();
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
    gl::Fbo::Format fmt = {};
#ifdef KINSKI_GLES_2
    fmt.color_internal_format = GL_LUMINANCE;
#else
    fmt.color_internal_format = GL_R8;
#endif

    m_fbo = gl::Fbo::create(res, fmt);
}

}