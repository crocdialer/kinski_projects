//
//  ModelViewer.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "core/Image.hpp"
#include "ModelViewer.h"
#include "AssimpConnector.h"
#include "gl/ShaderLibrary.h"

using namespace std;
using namespace kinski;
using namespace glm;

namespace
{
    const std::string tag_ground_plane = "ground_plane";
};

/////////////////////////////////////////////////////////////////

void ModelViewer::setup()
{
    ViewerApp::setup();

    register_property(m_draw_fps);
    register_property(m_model_path);
    register_property(m_use_lighting);
    register_property(m_use_post_process);
    register_property(m_offscreen_resolution);
    register_property(m_use_normal_map);
    register_property(m_use_ground_plane);
    register_property(m_use_bones);
    register_property(m_display_bones);
    register_property(m_animation_index);
    register_property(m_animation_speed);
    register_property(m_normalmap_path);
    register_property(m_skybox_path);
    
    register_property(m_focal_depth);
    register_property(m_focal_length);
    register_property(m_fstop);
    register_property(m_gain);
    register_property(m_fringe);
    register_property(m_circle_of_confusion_sz);
    register_property(m_auto_focus);
    register_property(m_debug_focus);
    
    observe_properties();

    // create our UI
    add_tweakbar_for_component(shared_from_this());
    add_tweakbar_for_component(m_light_component);

    // add lights to scene
    gl::Object3DPtr light_root = gl::Object3D::create();
    light_root->set_update_function([this, light_root](float time_delta)
    {
        light_root->transform() = gl::rotate(light_root->transform(), 0.1f * time_delta, gl::Y_AXIS);
    });
    
    // rearrange standard lights as carussel
    for(auto l : lights())
    {
        scene()->removeObject(l);
        light_root->add_child(l);
    }
    scene()->addObject(light_root);

    // add groundplane
    auto ground_mesh = gl::Mesh::create(gl::Geometry::create_plane(400, 400),
                                        gl::Material::create(gl::create_shader(gl::ShaderType::PHONG_SHADOWS)));
    ground_mesh->transform() = glm::rotate(mat4(), -glm::half_pi<float>(), gl::X_AXIS);
    ground_mesh->add_tag(tag_ground_plane);
    
    scene()->addObject(ground_mesh);

    load_settings();
}

/////////////////////////////////////////////////////////////////

void ModelViewer::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    update_shader();
    
    // check fbo
    gl::vec2 sz = m_offscreen_resolution->value() != gl::vec2() ?
    m_offscreen_resolution->value() : gl::window_dimension();
    
    if(*m_use_warping)
    {
        if(!m_offscreen_fbo || m_offscreen_fbo.size() != sz)
        {
            gl::Fbo::Format fmt;
            try{ m_offscreen_fbo = gl::Fbo(sz, fmt); }
            catch(Exception &e){ LOG_WARNING << e.what(); }
        }
    }
    if(*m_use_post_process)
    {
        if(!m_post_process_fbo || m_post_process_fbo.size() != sz)
        {
            gl::Fbo::Format fmt;
            try{ m_post_process_fbo = gl::Fbo(sz, fmt); }
            catch(Exception &e){ LOG_WARNING << e.what(); }
        }
        
        // check material
        if(!m_post_process_mat)
        {
            try
            {
                gl::Shader shader = gl::create_shader(gl::ShaderType::DEPTH_OF_FIELD);
                m_post_process_mat = gl::Material::create(shader);
                m_post_process_mat->set_depth_write(false);
                m_post_process_mat->set_depth_test(false);
            }catch(Exception &e){ LOG_WARNING << e.what(); }
        }
        
        camera()->setClippingPlanes(0.1f, 1000.f);
        m_post_process_mat->uniform("u_window_dimension", gl::window_dimension());
        m_post_process_mat->uniform("u_znear", camera()->near());
        m_post_process_mat->uniform("u_zfar", camera()->far());
        m_post_process_mat->uniform("u_focal_depth", *m_focal_depth);
        m_post_process_mat->uniform("u_focal_length", *m_focal_length);
        m_post_process_mat->uniform("u_fstop", *m_fstop);
        m_post_process_mat->uniform("u_gain", *m_gain);
        m_post_process_mat->uniform("u_fringe", *m_fringe);
        m_post_process_mat->uniform("u_debug_focus", *m_debug_focus );
        m_post_process_mat->uniform("u_auto_focus", *m_auto_focus );
        m_post_process_mat->uniform("u_circle_of_confusion_sz", *m_circle_of_confusion_sz);
        
    }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::draw()
{
    gl::set_matrices(camera());
    if(draw_grid()){ gl::draw_grid(50, 50); }

    if(*m_use_post_process)
    {
        auto tex = gl::render_to_texture(scene(), m_post_process_fbo, camera());
        
        // draw texture with post-processing
        m_post_process_mat->textures() =
        {
            m_post_process_fbo.texture(),
            m_post_process_fbo.depth_texture()
        };
        textures()[TEXTURE_OFFSCREEN] = tex;
    }
    if(*m_use_warping)
    {
        if(*m_use_post_process)
        {
            textures()[TEXTURE_OUTPUT] = gl::render_to_texture(m_offscreen_fbo, [this]()
            {
                gl::draw_quad(m_post_process_mat, gl::window_dimension());
            });
        }
        else
        {
            textures()[TEXTURE_OUTPUT] = gl::render_to_texture(scene(), m_offscreen_fbo, camera());
        }
        
        for(uint32_t i = 0; i < m_warp_component->num_warps(); i++)
        {
            if(m_warp_component->enabled(i))
            {
                m_warp_component->render_output(i, textures()[TEXTURE_OUTPUT]);
            }
        }
    }
    else
    {
        if(*m_use_post_process){ gl::draw_quad(m_post_process_mat, gl::window_dimension()); }
        else{ scene()->render(camera()); }
    }
    
    if(m_light_component->draw_light_dummies())
    {
        gl::set_matrices(camera());
        for (auto l : lights()){ gl::draw_light(l); }
    }
    
    if(*m_draw_fps)
    {
        gl::draw_text_2D(to_string(fps(), 1), fonts()[0],
                         glm::mix(gl::COLOR_OLIVE, gl::COLOR_WHITE,
                                  glm::smoothstep(0.f, 1.f, fps() / max_fps())),
                         gl::vec2(10));
    }

    if(m_mesh && *m_display_bones) // slow!
    {
        // crunch bone data
        vector<vec3> skel_points;
        vector<string> bone_names;
        build_skeleton(m_mesh->rootBone(), skel_points, bone_names);
        gl::load_matrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix() * m_mesh->global_transform());

        // draw bone data
        gl::draw_lines(skel_points, gl::COLOR_DARK_RED, 5.f);

        for(const auto &p : skel_points)
        {
            vec3 p_trans = (m_mesh->global_transform() * vec4(p, 1.f)).xyz();
            vec2 p2d = gl::project_point_to_screen(p_trans, camera());
            gl::draw_circle(p2d, 5.f, false);
        }
    }

    if(is_loading())
    {
        gl::draw_text_2D("loading ...", fonts()[0], gl::COLOR_WHITE,
                         gl::vec2(gl::window_dimension().x - 130, 20));
    }

    // draw texture map(s)
    if(displayTweakBar())
    {
        if(m_mesh)
        {
            std::vector<gl::Texture> comb_texs = textures();

            for(auto &mat : m_mesh->materials())
            {
                comb_texs = concat_containers<gl::Texture>(mat->textures(), comb_texs);
            }
            draw_textures(comb_texs);
        }
    }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{
    
}

/////////////////////////////////////////////////////////////////

void ModelViewer::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{
    
}

/////////////////////////////////////////////////////////////////

void ModelViewer::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{
    
}

/////////////////////////////////////////////////////////////////

void ModelViewer::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    std::vector<gl::Texture> dropped_textures;

    for(const string &f : files)
    {
        LOG_INFO << f;

        switch (fs::get_file_type(f))
        {
            case fs::FileType::MODEL:
            case fs::FileType::IMAGE:
                *m_model_path = f;
//                if(scene().pick(gl::calculate_ray(camera(), vec2(e.getX(), e.getY()))))
//                {
//                    LOG_INFO << "texture drop on model";
//                }
                break;
            default:
                break;
        }
    }
    if(m_mesh && !dropped_textures.empty()){ m_mesh->material()->textures() = dropped_textures; }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::tearDown()
{
    LOG_PRINT<<"ciao " << name();
}

/////////////////////////////////////////////////////////////////

void ModelViewer::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);

    if(theProperty == m_model_path)
    {
        fs::add_search_path(fs::get_directory_part(*m_model_path));
        async_load_asset(*m_model_path, [this](gl::MeshPtr m)
        {
            if(m)
            {
                scene()->removeObject(m_mesh);
                m_mesh = m;
                scene()->addObject(m_mesh);
                m_dirty_shader = true;
            }
        });
    }
    else if(theProperty == m_normalmap_path)
    {
        if(!m_normalmap_path->value().empty())
        {
            m_normal_map.reset();
            
            async_load_texture(*m_normalmap_path, [this](const gl::Texture &t)
            {
                m_normal_map = t;
                m_dirty_shader = true;
            }, true, true, 8.f);
        }
    }
    else if(theProperty == m_use_normal_map){ m_dirty_shader = true; }
    else if(theProperty == m_use_bones){ m_dirty_shader = true; }
    else if(theProperty == m_use_lighting){ m_dirty_shader = true; }
    else if(theProperty == m_wireframe)
    {
        if(m_mesh)
        {
            for(auto &mat : m_mesh->materials())
            {
                mat->set_wireframe(*m_wireframe);
            }
        }
    }
    else if(theProperty == m_animation_index)
    {
        if(m_mesh)
        {
            m_mesh->set_animation_index(*m_animation_index);
        }
    }
    else if(theProperty == m_animation_speed)
    {
        if(m_mesh)
        {
            m_mesh->set_animation_speed(*m_animation_speed);
        }
    }
    else if(theProperty == m_skybox_path)
    {
        async_load_texture(*m_skybox_path, [this](const gl::Texture &t){ scene()->set_skybox(t); });
    }
    else if(theProperty == m_use_ground_plane)
    {
        auto objs = scene()->get_objects_by_tag(tag_ground_plane);

        for(auto &o : objs)
        {
            o->set_enabled(*m_use_ground_plane);
        }
    }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::build_skeleton(gl::BonePtr currentBone, vector<vec3> &points,
                                 vector<string> &bone_names)
{
    if(!currentBone) return;

    // read out the bone names
    bone_names.push_back(currentBone->name);

    for (auto child_bone : currentBone->children)
    {
        mat4 globalTransform = currentBone->worldtransform;
        mat4 childGlobalTransform = child_bone->worldtransform;
        points.push_back(globalTransform[3].xyz());
        points.push_back(childGlobalTransform[3].xyz());

        build_skeleton(child_bone, points, bone_names);
    }
}

/////////////////////////////////////////////////////////////////

gl::MeshPtr ModelViewer::load_asset(const std::string &the_path)
{
    gl::MeshPtr m;

    auto asset_dir = fs::get_directory_part(the_path);
    fs::add_search_path(asset_dir);

    switch (fs::get_file_type(the_path))
    {
        case fs::FileType::DIRECTORY:
            for(const auto &p : fs::get_directory_entries(the_path)){ load_asset(p); }
            break;

        case fs::FileType::MODEL:
            m = gl::AssimpConnector::loadModel(the_path);
            break;

        case fs::FileType::IMAGE:
            
        {
            auto geom = gl::Geometry::create_plane(1.f, 1.f, 100, 100);
            auto mat = gl::Material::create();
            m = gl::Mesh::create(geom, mat);
            m->transform() = rotate(mat4(), 90.f, gl::Y_AXIS);
            
            async_load_texture(the_path, [this, m](const gl::Texture &t)
            {
                m->material()->add_texture(t);
                m->material()->set_two_sided();
                gl::vec3 s = m->scale();
                m->set_scale(gl::vec3(s.x * t.aspect_ratio(), s.y, 1.f));
                m->position().y += m->bounding_box().height() / 2.f;
            });
        }
            break;

        default:
            break;
    }

    if(m)
    {
        // apply scaling
        auto aabb = m->bounding_box();
        float scale_factor = 50.f / length(aabb.halfExtents());
        m->set_scale(scale_factor);

        // look for animations for this mesh
        auto animation_folder = fs::join_paths(asset_dir, "animations");

        for(const auto &f : get_directory_entries(animation_folder, fs::FileType::MODEL))
        {
            gl::AssimpConnector::add_animations_to_mesh(f, m);
        }
        m->set_animation_speed(*m_animation_speed);
        m->set_animation_index(*m_animation_index);
    }
    return m;
}

/////////////////////////////////////////////////////////////////

void ModelViewer::async_load_asset(const std::string &the_path,
                                   std::function<void(gl::MeshPtr)> the_completion_handler)
{
    background_queue().submit([this, the_completion_handler]()
    {
        // publish our task
        inc_task();
        
        // load model on worker thread
        auto m = load_asset(*m_model_path);

        std::map<gl::MaterialPtr, std::vector<ImagePtr>> mat_img_map;

        if(m)
        {
            // load and decode images on worker thread
            for(auto &mat : m->materials())
            {
                std::vector<ImagePtr> tex_imgs;

                for(auto &p : mat->texture_paths())
                {
                    try
                    {
                        auto img = create_image_from_file(p.first);
                        
                        if(img)
                        {
                            tex_imgs.push_back(img);
                            p.second = gl::Material::AssetLoadStatus::LOADED;
                        }
                    }
                    catch(Exception &e)
                    {
                        LOG_WARNING << e.what();
                        p.second = gl::Material::AssetLoadStatus::NOT_FOUND;
                    }
                }
                mat_img_map[mat] = tex_imgs;
            }
        }

        // work on this thread done, now queue texture creation on main queue
        main_queue().submit([this, m, mat_img_map, the_completion_handler]()
        {
            if(m)
            {
                for(auto &mat : m->materials())
                {
                    for(const ImagePtr &img : mat_img_map.at(mat))
                    {
                        gl::Texture tex = gl::create_texture_from_image(img, true, true);
                        mat->add_texture(tex);
                    }
                }
            }
            the_completion_handler(m);
            
            // task done
            dec_task();
        });
    });
}

/////////////////////////////////////////////////////////////////

void ModelViewer::update_shader()
{
    if(m_mesh && m_dirty_shader)
    {
        m_dirty_shader  = false;
        
        bool use_bones = m_mesh->geometry()->has_bones() && *m_use_bones;
        bool use_normal_map = *m_use_normal_map && *m_use_lighting && m_normal_map;
        gl::Shader shader;
        
        try
        {
            if(use_bones)
            {
                shader = gl::create_shader(*m_use_lighting ? gl::ShaderType::PHONG_SKIN_SHADOWS :
                                           gl::ShaderType::UNLIT_SKIN, false);
            }
            else
            {
                shader = gl::create_shader(*m_use_lighting ? gl::ShaderType::PHONG_SHADOWS :
                                           gl::ShaderType::UNLIT, false);
            }
            
            if(use_normal_map)
            {
                LOG_DEBUG << "adding normalmap: '" << m_normalmap_path->value() << "'";
                shader = gl::create_shader(gl::ShaderType::PHONG_NORMALMAP);
            }
        }
        catch (Exception &e){ LOG_ERROR << e.what(); }
        
        for(auto &mat : m_mesh->materials())
        {
            if(shader){ mat->set_shader(shader); }
            mat->set_blending();
            if(use_normal_map && mat->textures().size() < 2)
            {
                mat->textures().push_back(m_normal_map);
                mat->set_specular(gl::COLOR_WHITE);
                mat->set_shinyness(15.f);
            }
            else if(!mat->textures().empty()){ mat->textures().resize(1); }
        }
    }
}
