//
//  BlockbusterApp.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#ifndef __gl__BlockbusterApp__
#define __gl__BlockbusterApp__

#include "app/ViewerApp.hpp"
#include "app/RemoteControl.hpp"
#include "app/LightComponent.hpp"

// module
#include "opencl/cl_context.h"
#include "media/media.h"
#include "freenect/KinectDevice.h"
#include "syphon/SyphonConnector.h"

using namespace crocore;

namespace kinski
{
    class BlockbusterApp : public ViewerApp
    {
    private:
        
        enum ViewType{VIEW_NOTHING = 0, VIEW_DEBUG = 1, VIEW_OUTPUT = 2};
        enum TextureEnum{TEXTURE_DEPTH = 0, TEXTURE_MOVIE = 1, TEXTURE_SYPHON = 2};

        // kinect (re-)connection timer
        float m_timeout_kinect = 3.f, m_timeout_license = 1200.f;
        Timer m_timer_kinect, m_timer_license;

        // kinect assets
        FreenectPtr m_freenect;
        KinectDevicePtr m_kinect_device;
        std::vector<uint8_t> m_depth_data;

        // opencl assets
        cl_context m_opencl;
        cl::Kernel m_cl_kernel_update, m_cl_kernel_img;
        cl::BufferGL m_cl_buffer_vertex, m_cl_buffer_color, m_cl_buffer_pointsize;
        cl::Buffer m_cl_buffer_position, m_cl_buffer_params;
        cl::Event m_cl_event;

        // movie playback
        media::MediaControllerPtr m_movie = media::MediaController::create();
        
        gl::MeshPtr m_mesh;
        gl::Texture m_texture_input;
        gl::ShaderPtr m_block_shader, m_block_shader_shadows;

        gl::DeferredRendererPtr m_deferred_renderer;

        bool m_dirty_mesh = true, m_dirty_cl_context = true;
        bool m_has_new_texture = false;

        Property_<std::string>::Ptr
        m_media_path = Property_<std::string>::create("media path", "");

        // fbo / syphon stuff
        std::vector<gl::FboPtr> m_fbos{2};
        gl::CameraPtr m_fbo_cam;
        Property_<float>::Ptr
        m_fbo_cam_pos = RangedProperty<float>::create("fbo camera z-position", 220, 0.f, 1000.f);
        
        Property_<float>::Ptr
        m_fbo_cam_fov = RangedProperty<float>::create("fbo camera fov", 45.f, 0.f, 90.f);
        
        Property_<glm::vec2>::Ptr
        m_fbo_resolution = Property_<glm::vec2>::create("Fbo resolution", glm::vec2(1920, 1080));
        
        Property_<int>::Ptr
        m_view_type = RangedProperty<int>::create("view type", VIEW_OUTPUT, 0, 2);
        
        // output via Syphon
        syphon::Output m_syphon;
        Property_<bool>::Ptr m_use_syphon = Property_<bool>::create("Use syphon", true);
        Property_<std::string>::Ptr m_syphon_server_name =
        Property_<std::string>::create("Syphon server name", "blockbuster");
        
        Property_<int>::Ptr
        m_num_tiles_x = RangedProperty<int>::create("num tiles x", 128, 1, 1000),
        m_num_tiles_y = RangedProperty<int>::create("num tiles y", 100, 1, 1000),
        m_border = RangedProperty<int>::create("border", 0, 0, 10);
        
        Property_<float>::Ptr
        m_spacing_x = RangedProperty<float>::create("spacing x", 1.f, 0, 100),
        m_spacing_y = RangedProperty<float>::create("spacing y", 1.f, 0, 100),
        m_block_length = RangedProperty<float>::create("block length", 25.f, 1.f, 100.f),
        m_block_width = Property_<float>::create("block width", 1.f),
        m_depth_min = RangedProperty<float>::create("depth min", 1.f, 0.f, 6.f),
        m_depth_max = RangedProperty<float>::create("depth max", 3.f, 0.f, 6.f),
        m_z_min = RangedProperty<float>::create("min elevation", 0.f, 0.f, 100.f),
        m_z_max = RangedProperty<float>::create("max elevation", 50.f, 0.f, 100.f),
        m_depth_smooth_fall = RangedProperty<float>::create("depth smooth falling", .95f, 0.f, 1.f),
        m_depth_smooth_rise = RangedProperty<float>::create("depth smooth rising", .7f, 0.f, 1.f),
        m_input_depth = RangedProperty<float>::create("depth input", 1.f, 0.f, 1.f),
        m_input_color = RangedProperty<float>::create("color input", 1.f, 0.f, 1.f);

        Property_<gl::Color>::Ptr
        m_color_min = Property_<gl::Color>::create("color min", gl::COLOR_WHITE),
        m_color_max = Property_<gl::Color>::create("color max", gl::COLOR_WHITE);

        Property_<bool>::Ptr
        m_mirror_img = Property_<bool>::create("mirror image", true),
        m_enable_block_shader = Property_<bool>::create("enable block shader", true),
        m_enable_deferred_render = Property_<bool>::create("enable deferred render", false);

        Property_<std::string>::Ptr
        m_license = Property_<std::string>::create("license");

        void setup_cl();
        void init_opencl_buffers(gl::MeshPtr the_mesh);
        void update_cl(float the_time_delta);
        void apply_texture_cl(gl::Texture the_texture, bool is_depth_img);

        void init_shaders();
        gl::MeshPtr create_mesh();
        glm::vec3 click_pos_on_ground(const glm::vec2 click_pos);
        
    public:
        
        BlockbusterApp(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
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
        void file_drop(const MouseEvent &e, const std::vector<std::string> &files) override;
        void teardown() override;
        void update_property(const PropertyConstPtr &theProperty) override;

        void set_fullscreen(bool b, int monitor_index) override;
    };
}// namespace kinski

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::BlockbusterApp>(argc, argv);
    LOG_INFO<<"local ip: " << crocore::net::local_ip();
    return theApp->run();
}

#endif /* defined(__gl__BlockbusterApp__) */
