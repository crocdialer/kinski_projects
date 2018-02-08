//
//  3dViewer.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#pragma once

#include "core/CircularBuffer.hpp"
#include "app/ViewerApp.hpp"
#include "app/LightComponent.hpp"
#include "app/RemoteControl.hpp"

#include "gl/Noise.hpp"

// modules
#include "syphon/SyphonConnector.h"
#include "opencl/ParticleSystem.hpp"
#include "audio/audio.h"
#include "rtmidi/RtMidi.h"

namespace kinski
{
    
    // audio enum and type defs
    enum FrequencyBand{FREQ_LOW = 0, FREQ_MID_LOW = 1, FREQ_MID_HIGH = 2, FREQ_HIGH = 3};
    typedef std::map<FrequencyBand, std::pair<float, float>> FrequencyRangeMap;
    typedef std::map<FrequencyBand, float> FrequencyVolumeMap;
    
    class ModelViewer : public ViewerApp
    {
    private:
        
        gl::MeshPtr m_mesh, m_select_indicator;
        gl::ScenePtr m_debug_scene = gl::Scene::create();
        gl::Texture m_cube_map;
        gl::Noise m_noise;
        
        enum ShaderEnum{SHADER_UNLIT = 0, SHADER_UNLIT_SKIN = 1, SHADER_PHONG = 2,
            SHADER_PHONG_SKIN = 3, SHADER_UNLIT_DISPLACE = 4, SHADER_UNLIT_SKIN_DISPLACE = 5};
        
        enum TextureEnum{TEXTURE_OUTPUT = 0, TEXTURE_NOISE = 1, TEXTURE_SELECTED = 2};
        
        enum JoystickEnum{JOY_CROSS_LEFT = 2, JOY_CROSS_DOWN = 1, JOY_CROSS_RIGHT = 3,
            JOY_CROSS_UP = 0, JOY_START = 4, JOY_BACK = 5, JOY_RIGHT = 9, JOY_LEFT = 8,
            JOY_X = 13, JOY_Y = 14, JOY_A = 11, JOY_B = 12};
        
        std::vector<gl::ShaderPtr> m_shaders{10};

        Property_<std::string>::Ptr
        m_model_path = Property_<std::string>::create("model path", "");
        
        Property_<bool>::Ptr
        m_use_bones = Property_<bool>::create("use bones", true);
        
        Property_<bool>::Ptr
        m_display_bones = Property_<bool>::create("display bones", true);
        
        Property_<uint32_t>::Ptr
        m_animation_index = Property_<uint32_t>::create("animation index", 0);
        
        Property_<std::string>::Ptr
        m_cube_map_folder = Property_<std::string>::create("cubemap folder", "");
        
        /////////////////////////////////////////////////////////////////////////
        
        // our beamer setup
        std::vector<gl::PerspectiveCamera::Ptr> m_offscreen_cams{4};
        std::vector<gl::MeshPtr> m_offscreen_meshes{4};
        std::vector<gl::Color>
        m_cam_colors = {gl::COLOR_RED, gl::COLOR_GREEN, gl::COLOR_BLUE, gl::COLOR_YELLOW,
        gl::COLOR_ORANGE, gl::COLOR_PURPLE};
        gl::Fbo m_offscreen_fbo;
        
        Property_<uint32_t>::Ptr
        m_num_screens = Property_<uint32_t>::create("num beamers", 4);
        
        Property_<glm::vec2>::Ptr
        m_offscreen_size = Property_<glm::vec2>::create("offscreen size", glm::vec2(1024, 768));
        
        RangedProperty<float>::Ptr
        m_offscreen_fov = RangedProperty<float>::create("offscreen fov", 90.f, 0.f, 120.f),
        m_offscreen_manual_angle = RangedProperty<float>::create("offscreen manual angle", 90.f, 0.f, 360.f);
        
        Property_<bool>::Ptr
        m_use_offscreen_manual_angle = Property_<bool>::create("offscreen use manual angle", false);
        
        Property_<glm::vec3>::Ptr
        m_offscreen_offset = Property_<glm::vec3>::create("offscreen offset", glm::vec3());
        
        // output via Syphon
        syphon::Output m_syphon_out;
        Property_<bool>::Ptr m_use_syphon = Property_<bool>::create("use syphon", false);
        Property_<bool>::Ptr m_use_test_pattern = Property_<bool>::create("use test pattern", false);
        Property_<std::string>::Ptr m_syphon_server_name =
        Property_<std::string>::create("syphon server name", "lethargy 2025");
        
        void setup_offscreen_cameras(int num_cam, bool as_circle = true);
        
        gl::Texture create_offscreen_texture();
        
        bool m_needs_cam_refresh = true;

        //////////////////////////////// Scene Control /////////////////////////////////////////
        
        std::vector<gl::Object3DPtr> m_roots;
        
        std::vector<std::vector<gl::MeshPtr>> m_proto_meshes;
        
        RangedProperty<float>::Ptr
        m_displace_res = RangedProperty<float>::create("displacement resolution", 0.05f, 0.f, 0.3f),
        m_displace_velocity = RangedProperty<float>::create("displacement velocity", 0.1f, 0.f, 10.f);
        
        RangedProperty<float>::Ptr
        m_animation_speed = RangedProperty<float>::create("animation speed", 1.f, -1.5f, 1.5f),
        m_displace_factor = RangedProperty<float>::create("displacement factor", 0.f, 0.f, 30.f);
        
        RangedProperty<float>::Ptr
        m_rotation_lvl_1 = RangedProperty<float>::create("rotation lvl 1", 0.f, -100.f, 100.f),
        m_rotation_lvl_2 = RangedProperty<float>::create("rotation lvl 2", 0.f, -100.f, 100.f),
        m_rotation_lvl_3 = RangedProperty<float>::create("rotation lvl 3", 0.f, -100.f, 100.f),
        m_distance_lvl_1 = RangedProperty<float>::create("distance lvl 1", 75.f, 0.f, 500.f),
        m_distance_lvl_2 = RangedProperty<float>::create("distance lvl 2", 110.f, 0.f, 500.f),
        m_distance_lvl_3 = RangedProperty<float>::create("distance lvl 3", 190.f, 0.f, 500.f);
        
        Property_<uint32_t>::Ptr
        m_num_objects_lvl_1 = Property_<uint32_t>::create("num objects lvl 1", 1),
        m_num_objects_lvl_2 = Property_<uint32_t>::create("num objects lvl 2", 2),
        m_num_objects_lvl_3 = Property_<uint32_t>::create("num objects lvl 3", 3);
        
        Property_<bool>::Ptr
        m_obj_wire_frame = Property_<bool>::create("obj wireframe", false),
        m_obj_texturing = Property_<bool>::create("obj texturing", false);
        
        RangedProperty<float>::Ptr
        m_obj_scale = RangedProperty<float>::create("obj scale", 1.f, 0.01f, 5.f),
        m_obj_audio_react_low = RangedProperty<float>::create("obj audio_react low", 0.f, 0.f, 1.f),
        m_obj_audio_react_hi = RangedProperty<float>::create("obj audio_react hi", 0.f, 0.f, 3.f),
        m_obj_audio_auto_rotate = RangedProperty<float>::create("obj auto rotate", 0.f, 0.f, 3.f);
        
        Timer m_timer_select_enable;
        
        void process_joystick_input(float time_delta);
        
        void update_select_indicator();
        
        void reset_lvl(size_t the_lvl);
        
        void toggle_selection(int the_inc = 1);
        
        ////////////////////////////////// Midi section ////////////////////////////////////////
        
        std::shared_ptr<RtMidiIn> m_midi_in = std::make_shared<RtMidiIn>();
        Property_<uint32_t>::Ptr
        m_midi_port_nr = Property_<uint32_t>::create("midi port #", 0);
        
        ////////////////////////////////// Audio section ////////////////////////////////////////
        
        // experimental voice values
        FrequencyRangeMap m_frequency_map =
        {
            {FREQ_LOW, std::pair<float, float>(20.f, 200.f)},
            {FREQ_MID_LOW, std::pair<float, float>(200.f, 400.f)},
            {FREQ_MID_HIGH, std::pair<float, float>(400.f, 1200.f)},
            {FREQ_HIGH, std::pair<float, float>(1200.f, 22050.f)}
        };
        
        // recording assets
        audio::SoundPtr m_sound_recording;
        std::vector<float> m_sound_spectrum;
        std::vector<kinski::CircularBuffer<float>> m_sound_values{256};
        
        // last measured values
        std::vector<float> m_last_volumes;
        float m_last_volume;
        
        // frequency values
        RangedProperty<float>::Ptr
        m_freq_low = RangedProperty<float>::create("frequency low", 200, 0.f, 22050.f),
        m_freq_mid_low = RangedProperty<float>::create("frequency mid low", 400, 0.f, 22050.f),
        m_freq_mid_high = RangedProperty<float>::create("frequency mid high", 1200, 0.f, 22050.f),
        m_freq_high = RangedProperty<float>::create("frequency high", 22050, 0.f, 22050.f);
        
        void init_audio();
        void update_audio();
        float get_volume_for_subspectrum(float from_freq, float to_freq);
        float get_volume_for_freq_band(FrequencyBand bnd);
        
        ////////////////////////////////////////////////////////////////////////////////////////
        
        void build_skeleton(gl::BonePtr currentBone, vector<glm::vec3> &points,
                            vector<string> &bone_names);
        
        /*!
         * load asset into the pool of prototype meshes
         */
        bool load_asset(const std::string &the_path, uint32_t the_lvl, bool add_to_scene = false);
        
        Property_<vector<string>>::Ptr
        m_asset_paths = Property_<vector<string>>::create("asset paths");
        
    public:
        
        ModelViewer(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
        void setup();
        void update(float timeDelta);
        void draw();
        void resize(int w ,int h);
        void key_press(const KeyEvent &e);
        void key_release(const KeyEvent &e);
        void mouse_press(const MouseEvent &e);
        void mouse_release(const MouseEvent &e);
        void mouse_move(const MouseEvent &e);
        void mouse_drag(const MouseEvent &e);
        void mouse_wheel(const MouseEvent &e);
        void got_message(const std::vector<uint8_t> &the_message);
        void file_drop(const MouseEvent &e, const std::vector<std::string> &files);
        void teardown();
        void update_property(const Property::ConstPtr &theProperty);
        
        ////////////////////////////////////////////////////////////////////////////////////////
        
        void midi_callback(double time_stamp, const std::vector<uint8_t> &the_msg);
    };
}// namespace kinski
