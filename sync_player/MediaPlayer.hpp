//
//  MediaPlayer.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//
#pragma once

#include "app/ViewerApp.hpp"
#include "app/WarpComponent.hpp"
#include "gl/Texture.hpp"

#include "media/media.h"

namespace kinski
{
    class MediaPlayer : public ViewerApp
    {
    private:
        
        enum TextureEnum{TEXTURE_INPUT = 0, TEXTURE_OUTPUT = 1};

        WarpComponent::Ptr m_warp;
        net::udp_server m_udp_server;
        
        media::MediaControllerPtr m_movie = media::MovieController::create();
        media::CameraControllerPtr m_camera_control = media::CameraController::create();
        bool m_reload_movie = false;
        std::vector<Timer> m_movie_start_timers;
        Timer m_timer_movie_search;
        std::unordered_map<std::string, float> m_ip_adresses_dynamic;
        
        // properties
        Property_<std::vector<std::string>>::Ptr
        m_ip_adresses_static = Property_<std::vector<std::string>>::create("ip adresses",
        {
            "localhost"
        }),
        m_movie_library = Property_<std::vector<std::string>>::create("movie library");
        Property_<int>::Ptr m_movie_index = Property_<int>::create("movie index", -1);
        
        bool m_initiated = false;
        
        Property_<string>::Ptr
        m_movie_directory = Property_<string>::create("movie directory", "/mnt/movies"),
        m_movie_path = Property_<string>::create("movie path", "");
        
        Property_<bool>::Ptr
        m_loop = Property_<bool>::create("loop", false),
        m_auto_play = Property_<bool>::create("autoplay", false),
        m_use_warping = Property_<bool>::create("use warping", false),
        m_force_audio_jack = Property_<bool>::create("force 3.5mm audio-jack", false),
        m_load_remote_movies = Property_<bool>::create("remote movie loading", false);
        
        Property_<float>::Ptr
        m_movie_delay = Property_<float>::create("movie delay", 0.f),
        m_movie_delay_static = Property_<float>::create("movie delay static", 1.f);
        
        Property_<float>::Ptr
        m_playback_speed = Property_<float>::create("playback speed", 1.f),
        m_volume = RangedProperty<float>::create("volume", 1.f, 0.f , 1.f);
        
        
        std::string secs_to_time_str(float the_secs) const;
        void setup_rpc_interface();
        
        std::vector<std::string> get_remote_adresses() const;
        
        void reload_movie();
        
    public:

        MediaPlayer(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
        void setup() override;
        void update(float timeDelta) override;
        void draw() override;
        void resize(int w ,int h) override;
        void keyPress(const KeyEvent &e) override;
        void keyRelease(const KeyEvent &e) override;
        void mousePress(const MouseEvent &e) override;
        void mouseRelease(const MouseEvent &e) override;
        void mouseMove(const MouseEvent &e) override;
        void mouseDrag(const MouseEvent &e) override;
        void mouseWheel(const MouseEvent &e) override;
        void touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void fileDrop(const MouseEvent &e, const std::vector<std::string> &files) override;
        void tearDown() override;
        void update_property(const Property::ConstPtr &theProperty) override;
        
        bool save_settings(const std::string &path = "") override;
        bool load_settings(const std::string &path = "") override;
        
        void start_playback(const std::string &the_path);
        void stop_playback();
        void search_movies();

    };
}// namespace kinski
