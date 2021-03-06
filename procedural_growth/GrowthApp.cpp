//
//  GrowthApp.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "GrowthApp.h"
#include "gl/ShaderLibrary.h"
#include "shaders.h"

using namespace std;
using namespace kinski;
using namespace glm;

/////////////////////////////////////////////////////////////////

bool poop = true;

void GrowthApp::setup()
{
    ViewerApp::setup();
    
    set_precise_selection(false);
    
    register_property(m_draw_fps);
    register_property(m_branch_angles);
    register_property(m_branch_randomness);
    register_property(m_increment);
    register_property(m_increment_randomness);
    register_property(m_diameter);
    register_property(m_diameter_shrink);
    register_property(m_cap_bias);
    register_property(m_num_iterations);
    register_property(m_max_index);
    register_property(m_axiom);
    
    for(auto rule : m_rules)
        register_property(rule);
    
    register_property(m_use_bounding_mesh);
    register_property(m_shader_index);
    
    observe_properties();
    
    try
    {
        m_bounding_mesh = gl::Mesh::create(gl::Geometry::create_box(vec3(50)),
                                           gl::Material::create());
        m_bounding_mesh->position() += m_bounding_mesh->aabb().center();
        
        // some material props
        auto &bound_mat = m_bounding_mesh->material();
        bound_mat->set_diffuse(gl::Color(bound_mat->diffuse().rgb(), .2));
        bound_mat->set_blending();
        bound_mat->set_depth_write(false);
        
        // load shaders
        m_lsystem_shaders[0] = gl::Shader::create(geom_prepass_vert,
                                                  phong_frag,
                                                  gl::g_lines_to_cuboids_geom);
    }
    catch(std::exception &e){LOG_ERROR << e.what();}
    
    // add lights to scene
    for (auto l : lights()){ scene()->add_object(l ); }
    
    load_settings();
}

/////////////////////////////////////////////////////////////////

void GrowthApp::update(float timeDelta)
{
    ViewerApp::update(timeDelta);

    // construct ImGui window for this frame
    if(display_gui())
    {
        gui::draw_component_ui(shared_from_this());
        gui::draw_component_ui(m_light_component);
    }

    if(m_dirty_lsystem && !is_loading()) refresh_lsystem();
}

/////////////////////////////////////////////////////////////////

void GrowthApp::draw()
{
    gl::clear();
    gl::set_matrices(camera());
    if(draw_grid()){ gl::draw_grid(50, 50); }
    
    m_light_component->draw_light_dummies();
    
    // draw our scene
    scene()->render(camera());
    
    // our bounding mesh
//    if(wireframe())
//    {
//        gl::ScopedMatrixPush sp(gl::MODEL_VIEW_MATRIX);
//        gl::mult_matrix(gl::MODEL_VIEW_MATRIX, m_bounding_mesh->global_transform());
//        gl::draw_mesh(m_bounding_mesh);
//    }
    
    if(*m_draw_fps)
    {
        gl::draw_text_2D(to_string(fps(), 1), fonts()[0],
                         glm::mix(gl::COLOR_OLIVE, gl::COLOR_WHITE,
                                  glm::smoothstep(0.f, 1.f, fps() / max_fps())),
                         gl::vec2(10));
    }
    
    // draw texture map(s)
    if(display_gui())
    {
        draw_textures(textures());
    }
    
    if(is_loading())
    {
        gl::draw_text_2D("loading ...", fonts()[0], gl::COLOR_WHITE,
                         gl::vec2(gl::window_dimension().x - 130, 20));
    }
    
    gl::draw_transform(m_lsystem.turtle_transform(), 10);
}

/////////////////////////////////////////////////////////////////

void GrowthApp::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void GrowthApp::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);
    
    if(!display_gui())
    {
        switch (e.code())
        {
            case GLFW_KEY_LEFT:
                *m_num_iterations -= 1;
                break;
            
            case GLFW_KEY_RIGHT:
                *m_num_iterations += 1;
                break;
                
            case GLFW_KEY_1:
                // our lsystem shall draw a dragon curve
                *m_branch_angles = vec3(90);
                *m_axiom = "F";
                *m_num_iterations = 14;
                *m_rules[0] = "F = F - H";
                *m_rules[1] = "H = F + H";
                *m_rules[2] = "";
                *m_rules[3] = "";
                break;
                
            case GLFW_KEY_2:
                // our lsystem shall draw something else ...
                *m_branch_angles = vec3(90);
                *m_num_iterations = 4;
                *m_axiom = "-L";
                *m_rules[0] = "L=LF+RFR+FL-F-LFLFL-FRFR+";
                *m_rules[1] = "R=-LFLF+RFRFR+F+RF-LFL-FR";
                *m_rules[2] = "";
                *m_rules[3] = "";
                break;
                
            case GLFW_KEY_3:
                // our lsystem shall draw something else ...
                *m_branch_angles = vec3(60);
                *m_num_iterations = 4;
                *m_axiom = "F";
                *m_rules[0] = "F=F+G++G-F--FF-G+";
                *m_rules[1] = "G=-F+GG++G+F--F-G";
                *m_rules[2] = "";
                *m_rules[3] = "";
                break;

            case GLFW_KEY_4:
                // our lsystem shall draw something else ...
                *m_branch_angles = vec3(17.55, 20.0, 18.41);
                *m_num_iterations = 8;
                *m_axiom = "FFq";
                *m_rules[0] = "q=Fp[&/+p]F[^\\-p]";
                *m_rules[1] = "F=[--&p]q";
                *m_rules[2] = "p=FF[^^^-q][\\\\+q]";
                *m_rules[3] = "";
                break;
                
            case GLFW_KEY_5:
                // our lsystem shall draw something else ...
                *m_branch_angles = vec3(15.f);
                *m_num_iterations = 10;
                *m_axiom = "FA";
                *m_rules[0] = "A=^F+ F - B\\\\FB&&B";
                *m_rules[1] = "B=[^F/////A][&&A]";
                *m_rules[2] = "";
                *m_rules[3] = "";
                break;
                
            default:
                break;
        }
    }
}

/////////////////////////////////////////////////////////////////

void GrowthApp::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void GrowthApp::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);
}

/////////////////////////////////////////////////////////////////

void GrowthApp::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);
}

/////////////////////////////////////////////////////////////////

void GrowthApp::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void GrowthApp::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void GrowthApp::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
}

/////////////////////////////////////////////////////////////////

void GrowthApp::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{
    
}

/////////////////////////////////////////////////////////////////

void GrowthApp::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{
    
}

/////////////////////////////////////////////////////////////////

void GrowthApp::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{
    
}

/////////////////////////////////////////////////////////////////

void GrowthApp::teardown()
{
    LOG_PRINT<<"ciao procedural growth";
}

/////////////////////////////////////////////////////////////////

void GrowthApp::update_property(const PropertyConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
    
    bool rule_changed = false;
    
    for(auto r : m_rules)
        if (theProperty == r) rule_changed = true;
        
    if(theProperty == m_axiom ||
       rule_changed ||
       theProperty == m_num_iterations ||
       theProperty == m_branch_angles ||
       theProperty == m_branch_randomness ||
       theProperty == m_increment ||
       theProperty == m_increment_randomness ||
       theProperty == m_diameter ||
       theProperty == m_diameter_shrink)
    {
        m_dirty_lsystem = true;
    }
    else if(theProperty == m_max_index)
    {
        if(m_mesh)
        {
            m_mesh->entries().front().enabled = true;
            m_mesh->entries().front().num_indices = *m_max_index;
        }
    }
    else if(theProperty == m_cap_bias)
    {
        if(m_mesh)
        {
            for (auto m : m_mesh->materials())
            {
                m->uniform("u_cap_bias", *m_cap_bias);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////

void GrowthApp::refresh_lsystem()
{
    auto task = Task::create("refresh lsystem mesh");
    m_dirty_lsystem = false;
    m_lsystem.cancel();

    auto finish_cb = [this, task](gl::MeshPtr new_mesh)
    {
        if(!new_mesh){ return; }

        // create a mesh from our lsystem geometry
        scene()->remove_object(m_mesh);
        m_mesh = new_mesh;
        m_entries = m_mesh->entries();
        
        scene()->add_object(m_mesh);
        
        // add our shader
        for (auto &m : m_mesh->materials())
        {
            m->set_shader(m_lsystem_shaders[0]);
            
            //        m->add_texture(m_textures[0]);
            //        m->add_texture(m_textures[1]);
            m->set_blending();
            //        m->setDepthTest(false);
            //        m->setDepthWrite(false);
            
            m->uniform("u_cap_bias", *m_cap_bias);
            
            //TODO: remove this when submaterials are tested well enough
            m->set_diffuse(glm::linearRand(vec4(0,0,.3,.8), vec4(.3,1,1,.9)));
            m->set_point_attenuation(0.1, .0002, 0);
            m->set_roughness(.4);
        }
        
        uint32_t min = 0, max = m_entries.front().num_indices - 1;
        m_max_index->set_range(min, max);
        LOG_DEBUG << "radius: " << glm::length(m_mesh->aabb().halfExtents());
    };

    background_queue().post([this, finish_cb]()
                            {
                                m_lsystem.set_axiom(*m_axiom);
                                m_lsystem.rules().clear();

                                for(auto r : m_rules)
                                    m_lsystem.add_rule(*r);

                                m_lsystem.set_branch_angles(*m_branch_angles);
                                m_lsystem.set_branch_randomness(*m_branch_randomness);
                                m_lsystem.set_increment(*m_increment);
                                m_lsystem.set_increment_randomness(*m_increment_randomness);
                                m_lsystem.set_diameter(*m_diameter);
                                m_lsystem.set_diameter_shrink_factor(*m_diameter_shrink);

                                // iterate
                                m_lsystem.iterate(*m_num_iterations);

                                m_lsystem.set_max_random_tries(20);

                                // add a position check functor
                                if(*m_use_bounding_mesh)
                                {
                                    m_lsystem.set_position_check([=](const glm::vec3 &p) -> bool
                                                                 {
                                                                     return gl::is_point_inside_mesh(p,
                                                                                                     m_bounding_mesh);
                                                                 });
                                }
                                    // add an empty functor (clear position check)
                                else{ m_lsystem.set_position_check(LSystem::PositionCheckFunctor()); }

                                auto new_mesh = m_lsystem.create_mesh();

                                main_queue().post([finish_cb, new_mesh]() { finish_cb(new_mesh); });
                            });
}
