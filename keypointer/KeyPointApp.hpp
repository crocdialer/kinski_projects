//
//  ParticleSample.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#pragma once

#include "app/ViewerApp.hpp"
#include "cv/CVThread.h"
#include "cv/TextureIO.h"
#include "KeyPointNode.h"

namespace kinski
{
    class KeyPointApp : public ViewerApp
    {
    private:
        
        Property_<bool>::Ptr
        m_activator = Property_<bool>::create("processing", true);
        
        Property_<std::string>::Ptr
        m_img_path = Property_<std::string>::create("image path", "kinder.jpg");
        
        RangedProperty<uint32_t>::Ptr
        m_imageIndex = RangedProperty<uint32_t>::create("Image Index", 2, 0, 2);
        
        CVThread::Ptr m_cvThread;
        CVProcessNode::Ptr m_processNode;
        
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
