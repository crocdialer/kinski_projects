//
//  KeyPointApp.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include <opencv2/core/mat.hpp>
#include "KeyPointApp.hpp"
#include "media/media.h"

using namespace std;
using namespace kinski;
using namespace glm;

class CameraSource : public CVSourceNode
{
public:
    CameraSource(){ m_camera->start_capture(); };
    
    bool getNextImage(cv::Mat &img) override
    {
        int w, h;
        
        if(m_camera->copy_frame(m_img_buffer, &w, &h))
        {
            img = cv::Mat(h, w, CV_8UC3, &m_img_buffer[0]);
        }
        return true;
    }
    
private:
    media::CameraControllerPtr m_camera = media::CameraController::create();
    std::vector<uint8_t> m_img_buffer;
};

/////////////////////////////////////////////////////////////////

void KeyPointApp::setup()
{
    ViewerApp::setup();

    register_property(m_activator);
    register_property(m_img_path);
    register_property(m_imageIndex);
    
    add_tweakbar_for_component(shared_from_this());
    observe_properties();
    
    // CV stuff
    m_cvThread = std::make_shared<CVThread>();
    m_processNode = std::make_shared<KeyPointNode>(/*cv::imread(fs::search_file("kinder.jpg"))*/);
    
    // trigger observer callbacks
    m_processNode->observe_properties();
    add_tweakbar_for_component(m_processNode);
    
    m_cvThread->setProcessingNode(m_processNode);
//    m_cvThread->streamUSBCamera();
    
    m_cvThread->setSourceNode(std::make_shared<CameraSource>());
    m_cvThread->start();
    
    if(m_processNode)
    {
        addPropertyListToTweakBar(m_processNode->get_property_list());
        LOG_INFO<<"CVProcessNode: "<<m_processNode->getDescription();
    }
    
    LOG_INFO<<"CVThread source: "<<m_cvThread->getSourceInfo();
    
    load_settings();
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    auto width = gl::window_dimension().x;
    
    if(m_cvThread->hasImage())
    {
        vector<cv::Mat> images = m_cvThread->getImages();
        
        float imgAspect = images.front().cols/(float)images.front().rows;
        
        set_window_size( vec2(width, width / imgAspect) );
        
        
        for(uint32_t i = 0; i < images.size(); i++)
            gl::TextureIO::updateTexture(m_textures[i], images[i]);
        
        m_imageIndex->set_range(0, images.size() - 1);
    }
    
    // trigger processing
    m_cvThread->setProcessing(*m_activator);
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::draw()
{
    // draw fullscreen image
    gl::draw_texture(textures()[*m_imageIndex], gl::window_dimension());
    
    if(display_tweakbar()){ draw_textures(textures()); }
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    ViewerApp::file_drop(e, files);
    
    for(const string &f : files)
    {
        LOG_DEBUG << f;
        
        switch (fs::get_file_type(f))
        {
            case fs::FileType::IMAGE:
                *m_img_path = f;
                break;
            default:
                break;
        }
    }
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::teardown()
{
    m_cvThread->stop();
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
    
    if(theProperty == m_img_path)
    {
        if(!m_img_path->value().empty() && m_processNode)
        {
            try
            {

                auto kp_node = std::dynamic_pointer_cast<KeyPointNode>(m_processNode);
                kp_node->setReferenceImage(cv::imread(fs::search_file(*m_img_path), cv::IMREAD_GRAYSCALE));
            }
            catch(Exception &e){ LOG_WARNING << e.what(); }

        }
    }
}
