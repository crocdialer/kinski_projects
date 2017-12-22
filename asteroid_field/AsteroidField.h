//
//  AsteroidField.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#pragma once

#include "app/ViewerApp.hpp"
#include "core/Timer.hpp"

namespace kinski
{
    class AsteroidField : public ViewerApp
    {
    private:
        
        enum Mode{MODE_NORMAL = 0, MODE_LIGHTSPEED = 1};

        std::vector<gl::MeshPtr> m_proto_objects;
        std::vector<gl::Texture> m_proto_textures;
        std::vector<gl::MeshPtr> m_objects;
        
        gl::AABB m_aabb;
        
        Timer m_spawn_timer;
        
        Property_<string>::Ptr
        m_model_folder = Property_<string>::create("model folder", "~/asteroid_field/models"),
        m_texture_folder = Property_<string>::create("texture folder", "~/asteroid_field/textures"),
        m_sky_box_path = Property_<string>::create("skybox path", "skybox_01.png");
        
        Property_<glm::vec3>::Ptr
        m_half_extents = Property_<glm::vec3>::create("space half extents", glm::vec3(500.f)),
        m_velocity = Property_<glm::vec3>::create("velocity", glm::vec3(0, 0, 20.f));
        
        Property_<uint32_t>::Ptr
        m_num_objects = Property_<uint32_t>::create("num objects", 150);
        
        RangedProperty<int>::Ptr
        m_mode = RangedProperty<int>::create("lightspeed mode", MODE_NORMAL, 0, 1);
        
        bool m_dirty_flag = true;
        
        void load_assets();
        
        void create_scene(int num_objects);
        
    public:
        
        AsteroidField(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
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
        void file_drop(const MouseEvent &e, const std::vector<std::string> &files) override;
        void teardown() override;
        void update_property(const Property::ConstPtr &theProperty) override;
    };
}// namespace kinski
