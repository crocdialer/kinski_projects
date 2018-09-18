//
//  MatrixMask.hpp
//  led_grabber
//
//  Created by Fabian on 17.09.18.
//

#pragma once
#include "gl/Texture.hpp"

namespace kinski{namespace gl{
   
class MatrixMask
{
public:
    
    MatrixMask(const glm::ivec2 &the_size = glm::ivec2(3));
    
    //! poop dann gehts unten in die hose
    void update(float the_delta_time);
    
    //!
    const glm::ivec2 size() const;
    
    //!
    void set_size(const glm::ivec2 &the_size);
    
    const gl::Texture texture() const;
    
    void set_intensity(float the_intensity){ m_intensity = the_intensity; }
    float intensity() const { return m_intensity; };
    
    void set_lifetime(float the_min, float the_max)
    {
        m_lifetime_min = the_min;
        m_lifetime_max = the_max;
    }
    
private:
    
    struct gridcell_t
    {
        float life_time;
    };
    glm::ivec2 m_size;
    std::vector<gridcell_t> m_grid_cells;
    gl::Texture m_texture;
    float m_accumulator = 0.f;
    float m_intensity = 1.f;
    float m_lifetime_min = 0.f, m_lifetime_max = 1.f;
};
    
}};
