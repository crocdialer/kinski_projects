// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  3dViewer.hpp
//
//  Created by Fabian Schmidt on 29/01/14.

#pragma once

#include "app/ViewerApp.hpp"

namespace kinski
{
    class ModelViewer : public ViewerApp
    {
    private:
        
        enum TextureEnum{ TEXTURE_OFFSCREEN = 0 };
        
        gl::MeshPtr m_mesh;
        gl::Texture m_cube_map, m_normal_map;
        
        gl::Fbo m_post_process_fbo;
        gl::MaterialPtr m_post_process_mat;
        
        Property_<float>::Ptr
        m_focal_depth = Property_<float>::create("focal depth", 300.f),
        m_focal_length = Property_<float>::create("focal length", 50.f),
        m_fstop = Property_<float>::create("fstop", 1.f);
        
        Property_<bool>::Ptr
        m_debug_focus = Property_<bool>::create("debug focus", false),
        m_use_post_process = Property_<bool>::create("use post process", true);
        
        bool m_dirty_shader = true;
        
        Property_<bool>::Ptr
        m_draw_fps = Property_<bool>::create("draw fps", true),
        m_use_lighting = Property_<bool>::create("use lighting", true),
        m_use_normal_map = Property_<bool>::create("use normal mapping", true),
        m_use_ground_plane = Property_<bool>::create("use ground plane", true);
        
        Property_<std::string>::Ptr
        m_model_path = Property_<std::string>::create("Model path", ""),
        m_normalmap_path = Property_<std::string>::create("normalmap path", ""),
        m_cube_map_folder = Property_<std::string>::create("Cubemap folder", "");
        
        Property_<bool>::Ptr
        m_use_bones = Property_<bool>::create("use bones", true);
        
        Property_<bool>::Ptr
        m_display_bones = Property_<bool>::create("display bones", false);
        
        Property_<uint32_t>::Ptr
        m_animation_index = Property_<uint32_t>::create("animation index", 0);
        
        RangedProperty<float>::Ptr
        m_animation_speed = RangedProperty<float>::create("animation speed", 1.f, -1.5f, 1.5f);
        
        void build_skeleton(gl::BonePtr currentBone, vector<gl::vec3> &points,
                            vector<string> &bone_names);
        
        //! asset loading routine
        gl::MeshPtr load_asset(const std::string &the_path);
        
        //! asynchronous asset loading
        void async_load_asset(const std::string &the_path,
                              std::function<void(gl::MeshPtr)> the_completion_handler);
        
        void update_shader();
        
    public:
        
        ModelViewer(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
        void setup() override;
        void update(float timeDelta) override;
        void draw() override;
        void resize(int w ,int h) override;
        void keyPress(const KeyEvent &e) override;
        void keyRelease(const KeyEvent &e) override;
        void mousePress(const MouseEvent &e) override;
        void mouseRelease(const MouseEvent &e) override;
        void mouseMove(const MouseEvent &e) override;
        void mouseDrag(const MouseEvent &e) override;
        void mouseWheel(const MouseEvent &e) override;
        void touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void fileDrop(const MouseEvent &e, const std::vector<std::string> &files) override;
        void tearDown() override;
        void update_property(const Property::ConstPtr &theProperty) override;
    };
}// namespace kinski