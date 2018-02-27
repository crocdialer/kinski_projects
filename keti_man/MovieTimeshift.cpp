//
//  MovieTimeshift.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "MovieTimeshift.hpp"

using namespace std;
using namespace kinski;
using namespace glm;

namespace
{
#if defined(KINSKI_MAC)
    GLenum g_px_fmt = GL_BGRA;
#else
    GLenum g_px_fmt = GL_RGB;
#endif
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::setup()
{
    ViewerApp::setup();
    
    register_property(m_input_source);
    register_property(m_use_syphon);
    register_property(m_syphon_server_name);
    register_property(m_offscreen_size);
    register_property(m_cam_id);
    register_property(m_syphon_in_index);
    register_property(m_movie_speed);
    register_property(m_movie_path);
    register_property(m_noise_map_size);
    register_property(m_noise_scale_x);
    register_property(m_noise_scale_y);
    register_property(m_noise_velocity);
    register_property(m_noise_seed);
    register_property(m_num_buffer_frames);
    register_property(m_use_bg_substract);
    register_property(m_mog_learn_rate);
    
    observe_properties();
    add_tweakbar_for_component(shared_from_this());
    
    m_movie->set_on_load_callback(bind(&MovieTimeshift::on_movie_load, this));
    
    m_custom_mat = gl::Material::create();
    m_custom_mat->set_blending();
    m_custom_mat->set_depth_test(false);
    m_custom_mat->set_depth_write(false);
    
    try
    {
        auto sh = gl::create_shader_from_file("array_shader.vert", "array_shader.frag");
        m_custom_mat->set_shader(sh);
    } catch (Exception &e) { LOG_ERROR << e.what();}
    
    // init buffer as PixelBuffer
    m_pbo = gl::Buffer(GL_PIXEL_PACK_BUFFER, GL_STREAM_COPY);
    
    for(syphon::ServerDescription &sd : syphon::Input::get_inputs())
    {
        LOG_DEBUG << "Syphon inputs: " << sd.name << ": " << sd.app_name;
    }
    if(!syphon::Input::get_inputs().empty())
    {
        m_syphon_in_index->set_range(0, syphon::Input::get_inputs().size() - 1);
    }
    
    load_settings();
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::update(float timeDelta)
{
    // check if we need to adapt the input source
    if(m_input_source_changed)
    {
        m_input_source_changed = false;
        set_input_source(InputSource(m_input_source->value()));
    }
    
    if(*m_input_source == INPUT_MOVIE && m_needs_movie_refresh)
    {
        m_needs_movie_refresh = false;
        m_movie = media::MediaController::create(*m_movie_path);
        
        Stopwatch t;
        t.start();
        
        if(m_movie->copy_frames_offline(m_array_tex))
        {
            LOG_DEBUG << "copying frames to Arraytexture took: " << t.time_elapsed() << " secs";
            
            m_custom_mat->uniform("u_num_frames", m_array_tex.depth());
            m_custom_mat->set_textures({m_array_tex});
        }
    }

    bool advance_index = false;
    
    // fetch data from camera, if available. then upload to array texture
    if(m_camera && m_camera->is_capturing())
    {
        
        if(m_camera->copy_frame_to_image(m_camera_img))
        {
            textures()[TEXTURE_INPUT] = gl::create_texture_from_image(m_camera_img);
            
            if(m_needs_array_refresh)
            {
                m_needs_array_refresh = false;
                gl::Texture::Format fmt;
                fmt.set_target(GL_TEXTURE_3D);
//                fmt.setInternalFormat(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT);
                m_array_tex = gl::Texture(m_camera_img->width, m_camera_img->height, *m_num_buffer_frames, fmt);
                m_array_tex.set_flipped(true);
                
                m_custom_mat->set_textures({m_array_tex});
                m_custom_mat->uniform("u_num_frames", m_array_tex.depth());
            }
            
            // insert camera raw data into array texture
            insert_image_into_array_texture(m_camera_img, m_array_tex, m_current_index);
            advance_index = true;
        }
    }
    
    // syphon
    if(*m_input_source == INPUT_SYPHON && m_syphon_in.copy_frame(textures()[TEXTURE_INPUT]))
    {
        if(!m_fbo_transfer || m_fbo_transfer.size() != textures()[TEXTURE_INPUT].size())
        {
            m_fbo_transfer = gl::Fbo(textures()[TEXTURE_INPUT].size());
        }
        auto tex = gl::render_to_texture(m_fbo_transfer, [&]()
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            gl::draw_texture(textures()[TEXTURE_INPUT], m_fbo_transfer.size());
        });
        
        textures()[TEXTURE_INPUT] = tex;
        
        auto w = textures()[TEXTURE_INPUT].width();
        auto h = textures()[TEXTURE_INPUT].height();
        
        if(m_needs_array_refresh)
        {
            m_needs_array_refresh = false;
            
            gl::Texture::Format fmt;
            fmt.set_target(GL_TEXTURE_3D);
            m_array_tex = gl::Texture(w, h, *m_num_buffer_frames, fmt);
//            m_array_tex.set_flipped(!*m_flip_image);
            
            m_custom_mat->set_textures({m_array_tex});
            m_custom_mat->uniform("u_num_frames", m_array_tex.depth());
        }
        
        insert_texture_into_array_texture(textures()[TEXTURE_INPUT], m_array_tex, m_current_index);
        advance_index = true;
    }
    
    if(advance_index)
    {
        // advance index and set uniform variable
        m_current_index = (m_current_index + 1) % *m_num_buffer_frames;
        m_custom_mat->uniform("u_current_index", m_current_index);
    }
    
    // update procedural noise texture
    textures()[TEXTURE_NOISE] = m_noise.simplex(get_application_time() * *m_noise_velocity + *m_noise_seed);
    m_custom_mat->set_textures({m_array_tex, textures()[TEXTURE_NOISE]});
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::draw()
{
    gl::clear();

    if((*m_use_warping || *m_use_syphon) && m_offscreen_fbo)
    {
        textures()[TEXTURE_OUTPUT] = gl::render_to_texture(m_offscreen_fbo, [this]()
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            gl::draw_quad(gl::window_dimension(), m_custom_mat);
        });
        KINSKI_CHECK_GL_ERRORS();
        
        if(*m_use_warping)
        {
            for(uint32_t i = 0; i < m_warp_component->num_warps(); i++)
            {
                if(m_warp_component->enabled(i))
                {
                    m_warp_component->render_output(i, textures()[TEXTURE_OUTPUT]);
                    KINSKI_CHECK_GL_ERRORS();
                }
            }
        }
        else{ gl::draw_texture(textures()[TEXTURE_OUTPUT], gl::window_dimension()); }
        
        if(*m_use_syphon)
        {
            m_syphon_out.publish_texture(textures()[TEXTURE_OUTPUT]);
        }
    }
    else
    {
        gl::draw_quad(gl::window_dimension(), m_custom_mat);
    }
    
    if(display_tweakbar())
    {
        draw_textures(textures());
        gl::draw_text_2D(m_input_source_names[InputSource(m_input_source->value())], fonts()[0]);
    }
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::set_fullscreen(bool b, int monitor_index)
{
    ViewerApp::set_fullscreen(b, monitor_index);
    main_queue().submit([this]()
    {
        m_noise = gl::Noise(gl::vec2(m_noise_scale_x->value(), m_noise_scale_y->value()),
                            gl::ivec2(m_noise_map_size->value()));
        m_offscreen_size->notify_observers();
    });
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);
    
    switch (e.getCode())
    {
        default:
            break;
    }
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    *m_movie_path = files.back();
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::teardown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::on_movie_load()
{
    m_needs_movie_refresh = true;
    m_movie->set_rate(*m_movie_speed);
}

/////////////////////////////////////////////////////////////////

bool MovieTimeshift::insert_image_into_array_texture(const ImagePtr &the_image,
                                                     gl::Texture &the_array_tex,
                                                     uint32_t the_index)
{
    // sanity check
    if(!the_array_tex || !the_image || the_image->width != the_array_tex.width() ||
       the_image->height != the_array_tex.height() || the_index >= the_array_tex.depth())
    {
        return false;
    }
    
    // upload to pbo
    m_pbo.set_data(the_image->data, the_image->num_bytes());
    
    // bind pbo for reading
    m_pbo.bind(GL_PIXEL_UNPACK_BUFFER);

    // upload new frame to array texture
    the_array_tex.bind();
    glTexSubImage3D(the_array_tex.target(), 0, 0, 0, the_index, the_image->width, the_image->height, 1,
                    g_px_fmt, GL_UNSIGNED_BYTE, nullptr);
    the_array_tex.unbind();
    m_pbo.unbind(GL_PIXEL_UNPACK_BUFFER);
    return true;
}

/////////////////////////////////////////////////////////////////

bool MovieTimeshift::insert_texture_into_array_texture(const gl::Texture &the_texture,
                                                       gl::Texture &the_array_tex,
                                                       uint32_t the_index)
{
    // sanity check
    if(!the_texture || !the_array_tex || the_texture.size() != the_array_tex.size() ||
       the_index >= the_array_tex.depth())
    {
        return false;
    }
    
    // reserve size
    const uint32_t bytes_per_pixel = 4;
    size_t num_bytes = the_texture.width() * the_texture.height() * bytes_per_pixel;
    if(m_pbo.num_bytes() != num_bytes)
    {
        m_pbo.set_data(nullptr, num_bytes);
    }
    
    // download texture to pbo
    
    // bind pbo for writing
    m_pbo.bind(GL_PIXEL_PACK_BUFFER);
    
    the_texture.bind();
    glGetTexImage(the_texture.target(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    the_texture.unbind();
    m_pbo.unbind(GL_PIXEL_PACK_BUFFER);
    
    // bind pbo for reading
    m_pbo.bind(GL_PIXEL_UNPACK_BUFFER);
    
    // upload new frame from pbo to array texture
    the_array_tex.bind();
    glTexSubImage3D(the_array_tex.target(), 0, 0, 0, the_index, the_texture.width(),
                    the_texture.height(), 1, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    the_array_tex.unbind();
    m_pbo.unbind(GL_PIXEL_UNPACK_BUFFER);
    
    return true;
}

/////////////////////////////////////////////////////////////////

bool MovieTimeshift::set_input_source(InputSource the_src)
{
    m_needs_array_refresh = true;
    m_movie.reset();
    m_camera.reset();
    m_camera_img.reset();
    m_syphon_in = syphon::Input();
    
    switch (the_src)
    {
        case INPUT_NONE:
            break;
            
        case INPUT_MOVIE:
            m_needs_movie_refresh = true;
            break;
            
        case INPUT_CAMERA:
            m_camera = media::CameraController::create(*m_cam_id);
            m_camera->start_capture();
            break;
            
        case INPUT_SYPHON:
            try { m_syphon_in = syphon::Input(*m_syphon_in_index); }
            catch (syphon::SyphonInputOutOfBoundsException &e) { LOG_ERROR << e.what(); }
            break;
    }
    LOG_DEBUG << "input source changed to: " << m_input_source_names.at(the_src);
    return true;
}

/////////////////////////////////////////////////////////////////

//cv::UMat MovieTimeshift::create_foreground_image(std::vector<uint8_t> &the_data, int width, int height)
//{
//    static cv::Ptr<cv::BackgroundSubtractorMOG2> g_mog2 = cv::createBackgroundSubtractorMOG2();
//    static cv::UMat fg_mat;
//    
//    cv::UMat ret;
//    cv::UMat m = cv::Mat(height, width, CV_8UC4, &the_data[0]).getUMat(cv::ACCESS_RW);
//    g_mog2->apply(m, fg_mat, *m_mog_learn_rate);
//    blur(fg_mat, fg_mat, cv::Size(11, 11));
//    std::vector<cv::UMat> channels;
//    split(m, channels);
//    channels[3] = fg_mat;
//    merge(channels, ret);
//    return ret;
//}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
    
    if(theProperty == m_input_source)
    {
        m_input_source_changed = true;
    }
    else if(theProperty == m_movie_path)
    {
        m_input_source_changed = true;
    }
    else if(theProperty == m_movie_speed)
    {
        if(m_movie){ m_movie->set_rate(*m_movie_speed); }
    }
    else if(theProperty == m_num_buffer_frames)
    {
        m_needs_array_refresh = true;
    }
    else if(theProperty == m_noise_map_size || theProperty == m_noise_scale_x ||
            theProperty == m_noise_scale_y)
    {
        m_noise.set_tex_size(gl::ivec2(m_noise_map_size->value()));
        m_noise.set_scale(vec2(m_noise_scale_x->value(), m_noise_scale_y->value()));
    }
    else if(theProperty == m_offscreen_size)
    {
        gl::vec2 tmp = *m_offscreen_size;
        
        if(tmp.x <= 0 || tmp.y <= 0){ tmp = gl::window_dimension(); }
        
        m_offscreen_fbo = gl::Fbo(tmp);
    }
    else if(theProperty == m_use_syphon)
    {
        m_syphon_out = *m_use_syphon ? syphon::Output(*m_syphon_server_name) : syphon::Output();
    }
    else if(theProperty == m_syphon_server_name)
    {
        try{m_syphon_out.setName(*m_syphon_server_name);}
        catch(syphon::SyphonNotRunningException &e){LOG_WARNING<<e.what();}
    }
    else if(theProperty == m_cam_id)
    {
        m_input_source_changed = true;
    }
}
