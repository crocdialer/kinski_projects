//
//  ParticleSample.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#ifndef __gl__GrowthApp__
#define __gl__GrowthApp__

#include "crocore/Animation.hpp"
#include "app/ViewerApp.hpp"
#include "l_system/LSystem.h"

using namespace crocore;

namespace kinski
{
    class GrowthApp : public ViewerApp
    {
    private:
        
        gl::MeshPtr m_mesh, m_bounding_mesh;
        
        // our Lindemayer-System object
        LSystem m_lsystem;
        
        //! holds some shader programs, containing geomtry shaders for drawing the lines
        // created by a lsystem as more complex geometry
        //
        std::vector<gl::ShaderPtr> m_lsystem_shaders{4};
        
        //! needs to recalculate
        bool m_dirty_lsystem = false;
        
        std::vector<gl::Mesh::Entry> m_entries;
        
        //! animations
        std::vector<Animation> m_animations{10};
        
        // Properties
        RangedProperty<uint32_t>::Ptr m_max_index = RangedProperty<uint32_t>::create("max index",
                                                                                     0, 0, 2000000);
        
        Property_<uint32_t>::Ptr m_num_iterations = Property_<uint32_t>::create("num iterations", 2);
        Property_<glm::vec3>::Ptr m_branch_angles = Property_<glm::vec3>::create("branch angles",
                                                                                     glm::vec3(90));
        Property_<glm::vec3>::Ptr m_branch_randomness =
            Property_<glm::vec3>::create("branch randomness",
                                         glm::vec3(0));
        RangedProperty<float>::Ptr m_increment = RangedProperty<float>::create("growth increment",
                                                                                1.f, 0.f, 1000.f);
        RangedProperty<float>::Ptr m_increment_randomness =
            RangedProperty<float>::create("growth increment randomness",
                                          0.f, 0.f, 1000.f);
        RangedProperty<float>::Ptr m_diameter = RangedProperty<float>::create("diameter",
                                                                              1.f, 0.f, 100.f);
        RangedProperty<float>::Ptr m_diameter_shrink =
            RangedProperty<float>::create("diameter shrink factor",
                                          1.f, 0.f, 5.f);
        
        RangedProperty<float>::Ptr m_cap_bias = RangedProperty<float>::create("cap bias",
                                                                              0.f, 0.f, 100.f);
        
        Property_<std::string>::Ptr m_axiom = Property_<std::string>::create("Axiom", "f");
        std::vector<Property_<std::string>::Ptr> m_rules =
        {
            Property_<std::string>::create("Rule 1", "f = f - h"),
            Property_<std::string>::create("Rule 2", "h = f + h"),
            Property_<std::string>::create("Rule 3", ""),
            Property_<std::string>::create("Rule 4", "")
        };
        
        Property_<bool>::Ptr
        m_draw_fps = Property_<bool>::create("draw fps", true),
        m_use_bounding_mesh = Property_<bool>::create("use bounding mesh", false);
        
        Property_<uint32_t>::Ptr m_shader_index = Property_<uint32_t>::create("shader index", 0);
        
        void refresh_lsystem();
        
    public:
        GrowthApp(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
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
        void teardown() override;
        void update_property(const PropertyConstPtr &theProperty) override;
    };
}// namespace kinski

#endif /* defined(__gl__EmptySample__) */
