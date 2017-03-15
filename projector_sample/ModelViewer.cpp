//
//  ModelViewer.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "ModelViewer.h"
#include "AssimpConnector.h"

using namespace std;
using namespace kinski;
using namespace glm;

/////////////////////////////////////////////////////////////////

void ModelViewer::setup()
{
    ViewerApp::setup();
    
    register_property(m_model_path);
    register_property(m_movie_path);
    observe_properties();
    add_tweakbar_for_component(shared_from_this());
    
    m_light_component = std::make_shared<LightComponent>();
    m_light_component->set_lights(lights());
    add_tweakbar_for_component(m_light_component);
    
    
    gl::Fbo::Format fmt;
//    fmt.setSamples(8);
    fmt.set_num_color_buffers(0);
    
    m_fbos[0] = gl::Fbo(1024, 1024, fmt);
    m_fbos[1] = gl::Fbo(1024, 1024);
    
    // add lights to scene
    for (auto l : lights()){ scene()->add_object(l ); }
    
    m_draw_depth_mat = gl::Material::create();
    m_draw_depth_mat->set_blending();
    m_draw_depth_mat->set_depth_test(false);
    m_draw_depth_mat->set_depth_write();
    
    try
    {
        auto sh = gl::create_shader_from_file("depthmap.vert", "depthmap.frag");
        m_draw_depth_mat->set_shader(sh);
    } catch (Exception &e) { LOG_ERROR << e.what(); }
    
    // load settings
    load_settings();
    
    m_light_component->refresh();
    m_projector = create_camera_from_viewport();
}

/////////////////////////////////////////////////////////////////

void ModelViewer::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    // this renders the shadow map
    gl::render_to_texture(scene(), m_fbos[0], m_projector);
    
    if(m_mesh)
    {
        m_mesh->material()->uniform("u_light_mvp", m_projector->getProjectionMatrix() *
                                    m_projector->getViewMatrix() * m_mesh->global_transform());
    }
    
    if(m_movie->copy_frame_to_texture(textures()[TEXTURE_MOVIE], true))
    {
        auto &t = m_mesh->material()->textures();
        t = {textures().front(), textures()[TEXTURE_MOVIE], m_fbos[0].depth_texture()};
    }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::draw()
{
    gl::set_matrices(camera());
    if(draw_grid()){ gl::draw_grid(50, 50); }
    
    if(m_light_component->draw_light_dummies())
    {
        for (auto l : lights()){ gl::draw_light(l); }
    }
    
    scene()->render(camera());
    
    // draw texture map(s)
    if(displayTweakBar()){ draw_textures(textures()); }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);
    
    switch (e.getCode())
    {
        case GLFW_KEY_P:
            m_projector = create_camera_from_viewport();
            scene()->remove_object(m_projector_mesh);
            m_projector_mesh = gl::create_frustum_mesh(m_projector);
            scene()->add_object(m_projector_mesh);
            
            break;
            
        default:
            break;
    }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files)
    {
        LOG_INFO << f;
        
        // add path to searchpaths
        string dir_part = fs::get_directory_part(f);
        fs::add_search_path(dir_part);
        m_search_paths->value().push_back(dir_part);

        switch (fs::get_file_type(f))
        {
            case fs::FileType::MODEL:
                *m_model_path = f;
                break;
            
            case fs::FileType::IMAGE:
                try
                {
                    textures().push_back(gl::create_texture_from_file(f, true, false));
                    
                    if(m_mesh)
                    {
                        m_mesh->material()->textures().clear();
                        m_mesh->material()->textures().push_back(textures().back());
                    }
                }
                catch (Exception &e) { LOG_WARNING << e.what();}
                if(scene()->pick(gl::calculate_ray(camera(), vec2(e.getPos()))))
                {
                    LOG_INFO << "texture drop on model";
                }
                break;
                
            case fs::FileType::MOVIE:
                *m_movie_path = f;
                break;
            default:
                break;
        }
    }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::teardown()
{
    LOG_PRINT<<"ciao model viewer";
}

/////////////////////////////////////////////////////////////////

void ModelViewer::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
    
    if(theProperty == m_model_path)
    {
        gl::MeshPtr m = gl::AssimpConnector::loadModel(*m_model_path);
        
        if(m)
        {
            m->material()->set_shader(m_draw_depth_mat->shader());
//            m->material()->add_texture(m_fbos[0].getDepthTexture());
            m->createVertexArray();
            
            for(auto &t : m->material()->textures()){ textures()[0] = t; }
            
            scene()->remove_object(m_mesh);
            m_mesh = m;
            scene()->add_object(m_mesh);
            
            auto aabb = m->bounding_box();
            
            float scale_factor = 50.f / aabb.width();
            m->set_scale(scale_factor);
        }
    }
    else if(theProperty == m_movie_path)
    {
        m_movie->load(*m_movie_path, true, true);
        m_movie->set_on_load_callback([&](media::MovieControllerPtr m){ m->set_volume(0); });
    }
}

/////////////////////////////////////////////////////////////////

gl::PerspectiveCamera::Ptr ModelViewer::create_camera_from_viewport()
{
    gl::PerspectiveCamera::Ptr ret = std::make_shared<gl::PerspectiveCamera>();
    *ret = *camera();
    ret->setClippingPlanes(.1, 100.f);
    return ret;
}

glm::mat4 create_projector_matrix(gl::Camera::Ptr eye, gl::Camera::Ptr projector)
{
    return eye->global_transform() * projector->getViewMatrix() * projector->getProjectionMatrix();
}
