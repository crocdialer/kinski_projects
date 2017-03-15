#include "app/GLFW_App.hpp"
#include "gl/Material.hpp"

#include "cv/CVThread.h"
#include "cv/TextureIO.h"
#include "SalienceNode.h"

#include "gl/SerializerGL.hpp"

using namespace std;
using namespace kinski;
using namespace glm;

class Saliencer : public GLFW_App
{
private:
    
    gl::Texture m_textures[4];
    
    gl::Material::Ptr m_material;
    
    Property_<bool>::Ptr m_activator;
    
    RangedProperty<uint32_t>::Ptr m_imageIndex;
    
    CVThread::Ptr m_cvThread;
    
    CVProcessNode::Ptr m_processNode;
    
public:
    
    void setup()
    {
        setBarColor(vec4(0, 0 ,0 , .5));
        setBarSize(ivec2(250, 500));

        // add 2 empty textures
        m_material = gl::Material::create();
        m_material->add_texture(m_textures[0]);
        m_material->add_texture(m_textures[1]);
        m_material->set_depth_test(false);
        m_material->set_depth_write(false);
        
        m_activator = Property_<bool>::create("processing", true);
        m_imageIndex = RangedProperty<uint32_t>::create("Image Index", 0, 0, 1);
        
        register_property(m_activator);
        register_property(m_imageIndex);
        
        // add component-props to tweakbar
        //addPropertyListToTweakBar(getPropertyList());
        add_tweakbar_for_component(shared_from_this());
        
        // CV stuff
        m_cvThread = CVThread::Ptr(new CVThread());
        m_processNode = CVProcessNode::Ptr(new SalienceNode);
        
        m_cvThread->setProcessingNode(m_processNode);
        
        //m_cvThread->streamVideo("/Users/Fabian/dev/testGround/python/cvScope/scopeFootage/testMovie_00.MOV", true);
        m_cvThread->streamUSBCamera();
        
        if(m_processNode)
        {
            addPropertyListToTweakBar(m_processNode->get_property_list());
            m_processNode->observe_properties();
            LOG_INFO<<"CVProcessNode: \n"<<m_processNode->getDescription();
        }
        
        LOG_INFO<<"CVThread source: \n"<<m_cvThread->getSourceInfo();
        
        try
        {
            m_material->set_shader(gl::create_shader_from_file("applyMap.vert", "applyMap.frag"));
            Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
            
        }catch(Exception &e)
        {
            LOG_WARNING<<e.what();
        }
    }
    
    void teardown()
    {
        m_cvThread->stop();
        Serializer::saveComponentState(shared_from_this(), "config.json", PropertyIO_GL());
        
        LOG_PRINT<<"ciao saliencer";
    }
    
    void update(float timeDelta)
    {
        if(m_cvThread->hasImage())
        {
            m_material->textures().clear();
            vector<cv::Mat> images = m_cvThread->getImages();
            
            for(int i=0;i<images.size();i++)
            {
                gl::TextureIO::updateTexture(m_textures[i], images[i]);
                m_material->textures().push_back(m_textures[i]);
            }
            
            m_imageIndex->set_range(0, images.size() - 1);
        }
        
        // trigger processing
        m_cvThread->setProcessing(*m_activator);
    }
    
    void draw()
    {
        // draw fullscreen image
        if(*m_activator)
            gl::draw_quad(m_material, gl::window_dimension());
        else
            gl::draw_texture(m_material->textures()[*m_imageIndex], gl::window_dimension());
        
        // draw process-results map(s)
        float w = (gl::window_dimension() / 6.f).x;
        glm::vec2 offset(gl::window_dimension().x - w - 10, 10);
        
        for(int i=0;i<m_cvThread->getImages().size();i++)
        {
            if(!m_textures[i]) continue;
            float h = m_textures[i].height() * w / m_textures[i].width();
            glm::vec2 step(0, h + 10);
            
            gl::draw_texture(m_textures[i],
                            glm::vec2(w, h),
                            offset);
            
            offset += step;
        }
    }
    
    void key_press(const KeyEvent &e)
    {
        if(e.getChar() == GLFW_KEY_SPACE)
        {
            displayTweakBar(!displayTweakBar());
        }
    }
};

int main(int argc, char *argv[])
{
    AppPtr theApp(new Saliencer);
    return theApp->run();
}
