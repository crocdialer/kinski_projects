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

        media::MediaControllerPtr m_movie = media::MediaController::create();

        bool m_reload_movie = false, m_needs_redraw = true;
        std::vector<Timer> m_movie_start_timers;

        // properties
        Property_<std::vector<std::string>>::Ptr
        m_ip_adresses = Property_<std::vector<std::string>>::create("ip adresses",
        {
            "localhost",// vellocet.local
//            "synthemesc.local",
//            "drencrome.local"
        }),
        m_movie_library = Property_<std::vector<std::string>>::create("movie library");
        Property_<int>::Ptr m_movie_index = Property_<int>::create("movie index", -1);

        bool m_initiated = false;
        float m_delay_static = 3.f;

        Property_<string>::Ptr
        m_movie_directory = Property_<string>::create("movie directory", "/mnt/movies"),
        m_movie_path = Property_<string>::create("movie path", ""),
        m_movie_playlist = Property_<string>::create("movie playlist", "");

        Property_<bool>::Ptr
        m_loop = Property_<bool>::create("loop", false),
        m_auto_play = Property_<bool>::create("autoplay", false);

        Property_<float>::Ptr
        m_movie_speed = Property_<float>::create("movie speed", 1.f),
        m_movie_delay = Property_<float>::create("movie delay", 0.f),
        m_movie_volume = Property_<float>::create("movie volume", 0.f);

        Property_<bool>::Ptr m_use_warping = Property_<bool>::create("use warping", false);

        std::string secs_to_time_str(float the_secs) const;
        void setup_rpc_interface();
        void play_next_item();

    public:

        MediaPlayer(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
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
        void update_property(const Property::ConstPtr &theProperty) override;

        void start_playback(const std::string &the_path);
        void stop_playback();
        void search_movies();
        
        bool needs_redraw() const override;

    };
}// namespace kinski

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::MediaPlayer>(argc, argv);
    LOG_INFO << "local ip: " << kinski::net::local_ip();
    return theApp->run();
}
