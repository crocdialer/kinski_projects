//
//  3dViewer.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#pragma once

#include "app/ViewerApp.hpp"
#include "app/LightComponent.hpp"
#include "gl/Fbo.hpp"

#include "media/media.h"

namespace kinski
{
    glm::mat4 create_projector_matrix(gl::Camera::Ptr eye, gl::Camera::Ptr projector);
    
    class ModelViewer : public ViewerApp
    {
    private:
        
        enum TextureEnum{TEXTURE_DIFFUSE = 0, TEXTURE_SHADOWMAP = 1, TEXTURE_MOVIE = 2};
        
        gl::MeshPtr m_mesh;
        
        LightComponentPtr m_light_component;
        
        std::vector<gl::Fbo> m_fbos{4};
        
        gl::PerspectiveCamera::Ptr m_projector;
        gl::MeshPtr m_projector_mesh;
        media::MovieControllerPtr m_movie = media::MovieController::create();
        
        gl::MaterialPtr m_draw_depth_mat;
        
        gl::PerspectiveCamera::Ptr create_camera_from_viewport();
        
        Property_<std::string>::Ptr m_model_path = Property_<std::string>::create("Model path", "");
        Property_<std::string>::Ptr m_movie_path = Property_<std::string>::create("Movie path", "");
        
    public:
        
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
