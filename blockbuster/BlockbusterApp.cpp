//
//  BlockbusterApp.cpp
//  gl
//
//  Created by Fabian on 17/03/15.
//
//

#include "BlockbusterApp.hpp"
#include "gl/ShaderLibrary.h"

using namespace std;
using namespace kinski;
using namespace glm;

namespace
{

struct param_t
{
    int num_cols, num_rows, mirror, border;
    float depth_min, depth_max, multiplier;
    float smooth_fall, smooth_rise;
    float min_size, max_size;
};

}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::setup()
{
    ViewerApp::setup();
    
    register_property(m_view_type);
    register_property(m_media_path);
    register_property(m_use_syphon);
    register_property(m_syphon_server_name);
    register_property(m_fbo_cam_pos);
    register_property(m_fbo_cam_fov);
    register_property(m_fbo_resolution);
    register_property(m_num_tiles_x);
    register_property(m_num_tiles_y);
    register_property(m_spacing_x);
    register_property(m_spacing_y);
    register_property(m_block_length);
    register_property(m_block_width);
    register_property(m_block_width_multiplier);
    register_property(m_border);
    register_property(m_mirror_img);
    register_property(m_use_shadows);
    register_property(m_depth_min);
    register_property(m_depth_max);
    register_property(m_depth_multiplier);
    register_property(m_depth_smooth_fall);
    register_property(m_depth_smooth_rise);
    register_property(m_poisson_radius);
    observe_properties();

    // init our application specific shaders
//    init_shaders();

    // create our remote interface
    remote_control().set_components({ shared_from_this(), m_light_component, m_warp_component });

    m_opencl.init();

    try
    {
        m_opencl.set_sources("kernels.cl");
        m_cl_kernel_update = cl::Kernel(m_opencl.program(), "update_mesh");
    }
    catch(cl::Error &error){ LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")"; }

    // depth sensor
    m_freenect = Freenect::create();

    if(m_freenect->num_devices())
    {
        LOG_INFO << "found kinect -> connecting ...";
        m_kinect_device = m_freenect->create_device(0);
        m_kinect_device->start_depth();
        m_kinect_device->set_led(LED_RED);
    }else{ LOG_WARNING << "no kinect found ... "; }

//    scene()->set_renderer(gl::SceneRenderer::create());
    load_settings();
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::update(float timeDelta)
{
    ViewerApp::update(timeDelta);

    // construct ImGui window for this frame
    if(display_tweakbar())
    {
        gui::draw_component_ui(shared_from_this());
        gui::draw_component_ui(m_light_component);
        if(*m_use_warping){ gui::draw_component_ui(m_warp_component); }
    }

    if(m_dirty)
    {
        scene()->remove_object(m_mesh);
        m_mesh = create_mesh();
        init_opencl_buffers(m_mesh);
        scene()->add_object(m_mesh);
        m_dirty = false;
    }

    update_cl(timeDelta);
    
    if(m_movie && m_movie->copy_frame_to_texture(textures()[TEXTURE_MOVIE], true))
    {
        m_has_new_texture = true;
    }

    // grab a depth image
    if(m_kinect_device && m_kinect_device->copy_frame_depth(m_depth_data))
    {
        //tmp: crunch data to float
        constexpr float factor = 1 / 2047.f;
        constexpr uint32_t width = 640, height = 480;

//        std::vector<float> float_vals(2 * m_depth_data.size());
//        float *dst = float_vals.data();
//        uint16_t *src = reinterpret_cast<uint16_t*>(m_depth_data.data()), *src_end = src + (width * height);
//
//        for(; src < src_end; ++src){ *dst++ = *src * factor; }
//
//        gl::Texture::Format fmt;
//        fmt.internal_format = GL_RED;
//        fmt.datatype = GL_FLOAT;
//        auto tex = gl::Texture(float_vals.data(), GL_RED, 640, 480, fmt);

        // create depth texture
        gl::Texture::Format fmt;
        fmt.internal_format = GL_RED;
        fmt.datatype = GL_UNSIGNED_SHORT;
        auto tex = gl::Texture(m_depth_data.data(), GL_RED, width, height, fmt);

        textures()[TEXTURE_DEPTH] = tex;
        m_has_new_texture = true;
    }
    
//    if(m_has_new_texture)
//    {
//        m_has_new_texture = false;
//
//        if(textures()[TEXTURE_MOVIE] && textures()[TEXTURE_DEPTH])
//        {
//            m_psystem.texture_input_alt(textures()[TEXTURE_DEPTH], textures()[TEXTURE_MOVIE]);
//        }
//        else if(textures()[TEXTURE_DEPTH])
//        {
//            m_psystem.texture_input(textures()[TEXTURE_DEPTH]);
//        }
//    }
    
//    m_psystem->update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::update_cl(float the_time_delta)
{
    param_t p;
    p.mirror = *m_mirror_img;
    p.border = *m_border;
    p.num_cols = *m_num_tiles_x;
    p.num_rows = *m_num_tiles_y;
    p.depth_min = *m_depth_min;
    p.depth_max = *m_depth_max;
    p.multiplier = *m_depth_multiplier;
    p.smooth_fall = *m_depth_smooth_fall;
    p.smooth_rise = *m_depth_smooth_rise;
    p.min_size = *m_block_width;
    p.max_size = *m_block_width * *m_block_width_multiplier;

    try
    {
        vector<cl::Memory> gl_buffers = {m_cl_buffer_vertex, m_cl_buffer_color, m_cl_buffer_pointsize};

        // Make sure OpenGL is done using our VBOs
        glFinish();

        // map OpenGL buffer object for writing from OpenCL
        // this passes in the vector of VBO buffer objects (position and color)
        m_opencl.queue().enqueueAcquireGLObjects(&gl_buffers);

        size_t num_vertices = m_mesh->geometry()->vertices().size();

        // upload params to CL buffer
        m_opencl.queue().enqueueWriteBuffer(m_cl_buffer_params, false, 0, sizeof(p), &p, nullptr);

        // setup update kernel
        m_cl_kernel_update.setArg(0, m_cl_buffer_vertex);
        m_cl_kernel_update.setArg(1, m_cl_buffer_color);
        m_cl_kernel_update.setArg(2, m_cl_buffer_pointsize);
        m_cl_kernel_update.setArg(3, m_cl_buffer_position);
        m_cl_kernel_update.setArg(4, the_time_delta);
        m_cl_kernel_update.setArg(5, m_cl_buffer_params);

        // execute update kernel
        m_opencl.queue().enqueueNDRangeKernel(m_cl_kernel_update,
                                              cl::NullRange,
                                              cl::NDRange(num_vertices));
        m_opencl.queue().finish();

        // Release the VBOs again
        m_opencl.queue().enqueueReleaseGLObjects(&gl_buffers, NULL);
        m_opencl.queue().finish();
    }
    catch(cl::Error &error)
    {
        LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")";
    }
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::draw()
{
    gl::clear();

    // draw the output texture
    if(m_fbos[0] && m_fbo_cam && *m_use_syphon)
    {
        auto tex = gl::render_to_texture(m_fbos[0],[&]
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            scene()->render(m_fbo_cam);
        });
        textures()[TEXTURE_SYPHON] = tex;
        m_syphon.publish_texture(tex);
    }
    
    switch(*m_view_type)
    {
        case VIEW_DEBUG:
            gl::set_matrices(camera());
            if(draw_grid()){ gl::draw_grid(50, 50); }
            
//            m_light_component->draw_light_dummies();

            
            scene()->render(camera());
            break;

        case VIEW_OUTPUT:
            gl::draw_texture(textures()[TEXTURE_SYPHON], gl::window_dimension());
            break;

        default:
            break;
    }

//    m_light_component->draw_light_dummies();
    if(display_tweakbar()){ draw_textures(textures());}
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);
//    m_user_positions = { click_pos_on_ground(vec2(e.getPos())) };
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
//    m_user_positions = { click_pos_on_ground(vec2(e.getPos())) };
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files)
    {
        LOG_DEBUG << f;
        
        auto ft = fs::get_file_type(f);
        if(ft == fs::FileType::IMAGE || ft == fs::FileType::MOVIE)
        {
            *m_media_path = f;
        }
    }
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::teardown()
{
    LOG_PRINT<<"ciao " << name();
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
    
    if(theProperty == m_media_path)
    {
        m_movie = media::MediaController::create(*m_media_path, true, true);
        textures()[TEXTURE_MOVIE] = gl::Texture();
    }
    else if(theProperty == m_block_length)
    {
        if(m_mesh)
        {
            m_mesh->material()->uniform("u_length", *m_block_length);
        }
    }
//    else if(theProperty == m_block_width)
//    {
//        if(m_mesh)
//        {
//            m_mesh->material()->uniform("u_width", *m_block_width);
//        }
//    }
    else if(theProperty == m_num_tiles_x ||
            theProperty == m_num_tiles_y ||
            theProperty == m_spacing_x ||
            theProperty == m_spacing_y ||
            theProperty == m_use_shadows)
    {
        m_dirty = true;
    }
    else if(theProperty == m_fbo_resolution ||
            theProperty == m_fbo_cam_pos ||
            theProperty == m_fbo_cam_fov)
    {
        gl::Fbo::Format fmt;
        fmt.num_samples = 4;
        m_fbos[0] = gl::Fbo::create(m_fbo_resolution->value().x, m_fbo_resolution->value().y, fmt);
        float aspect = m_fbos[0]->aspect_ratio();
        m_fbo_cam = gl::PerspectiveCamera::create(aspect, *m_fbo_cam_fov, 5.f, 2000.f);
        m_fbo_cam->position() = *m_fbo_cam_pos;

    }
    else if(theProperty == m_use_syphon)
    {
        m_syphon = *m_use_syphon ? syphon::Output(*m_syphon_server_name) : syphon::Output();
    }
    else if(theProperty == m_syphon_server_name)
    {
        try{m_syphon.setName(*m_syphon_server_name);}
        catch(syphon::SyphonNotRunningException &e){LOG_WARNING<<e.what();}
    }
    else if(theProperty == m_poisson_radius)
    {
        if(m_mesh)
            m_mesh->material()->uniform("u_poisson_radius", *m_poisson_radius);
    }
}

/////////////////////////////////////////////////////////////////

gl::MeshPtr BlockbusterApp::create_mesh()
{
    gl::MeshPtr ret;
    gl::GeometryPtr geom = gl::Geometry::create();
    geom->set_primitive_type(GL_POINTS);
    geom->vertices().resize(*m_num_tiles_x * *m_num_tiles_y);
    geom->normals().resize(*m_num_tiles_x * *m_num_tiles_y, vec3(0, 0, 1));
    geom->point_sizes().resize(*m_num_tiles_x * *m_num_tiles_y, 1.f);
    geom->colors().resize(*m_num_tiles_x * *m_num_tiles_y, gl::COLOR_WHITE);
    vec2 step(m_spacing_x->value(), m_spacing_y->value());
    vec2 offset = -vec2(m_num_tiles_x->value(), m_num_tiles_y->value()) * step / 2.f;
    
    auto &verts = geom->vertices();
    
    for(int y = 0; y < *m_num_tiles_y; y++)
    {
        for(int x = 0; x < *m_num_tiles_x; x++)
        {
            verts[y * *m_num_tiles_x + x] = vec3(offset + vec2(x, y) * step, 0.f);
        }
    }
    geom->compute_aabb();

    auto mat = gl::Material::create(*m_use_shadows ? m_block_shader_shadows :
                                    m_block_shader);
    ret = gl::Mesh::create(geom);
    ret->material()->uniform("u_length", *m_block_length);
//    ret->material()->uniform("u_width", *m_block_width);
//    ret->material()->setBlending();

    return ret;
}

/////////////////////////////////////////////////////////////////

glm::vec3 BlockbusterApp::click_pos_on_ground(const glm::vec2 click_pos)
{
    gl::Plane ground_plane(vec3(0), vec3(0, 1, 0));
    auto ray = gl::calculate_ray(camera(), click_pos);
    auto intersect = ground_plane.intersect(ray);
    vec3 ret = ray * intersect.distance;
    return ret;
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::init_shaders()
{
    m_block_shader = gl::create_shader(gl::ShaderType::UNLIT);
//    m_block_shader->load_from_data(fs::read_file("geom_prepass.vert"),
//                                   phong_frag,
//                                   fs::read_file("points_to_cubes.geom"));

    m_block_shader_shadows = gl::create_shader(gl::ShaderType::UNLIT);
//    m_block_shader_shadows->load_from_data(fs::read_file("geom_prepass.vert"),
//                                           phong_shadows_frag,
//                                           fs::read_file("points_to_cubes_shadows.geom"));
}

void BlockbusterApp::init_opencl_buffers(gl::MeshPtr the_mesh)
{
    if(!the_mesh){ return; }
    auto geom = the_mesh->geometry();
    geom->create_gl_buffers();

    m_cl_buffer_params = cl::Buffer(m_opencl.context(), CL_MEM_READ_WRITE,
                                    sizeof(param_t));

    // spawn positions
    m_cl_buffer_position = cl::Buffer(m_opencl.context(), CL_MEM_READ_WRITE,
                                      geom->vertex_buffer().num_bytes());

    // generate spawn positions from original positions
    const uint8_t *vert_buf = geom->vertex_buffer().map();
    m_opencl.queue().enqueueWriteBuffer(m_cl_buffer_position, CL_TRUE, 0,
                                        geom->vertex_buffer().num_bytes(),
                                        vert_buf);
    geom->vertex_buffer().unmap();

    // vertices
    m_cl_buffer_vertex = cl::BufferGL(m_opencl.context(), CL_MEM_READ_WRITE,
                                      geom->vertex_buffer().id());

    // colors
    m_cl_buffer_color = cl::BufferGL(m_opencl.context(), CL_MEM_READ_WRITE,
                                     geom->color_buffer().id());
    // pointsizes
    m_cl_buffer_pointsize = cl::BufferGL(m_opencl.context(), CL_MEM_READ_WRITE,
                                         geom->point_size_buffer().id());
}