//
//  MediaPlayer.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "MediaPlayer.hpp"

using namespace std;
using namespace kinski;
using namespace glm;

namespace
{
    uint16_t g_remote_port = 33333;
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::setup()
{
    ViewerApp::setup();
    fonts()[1].load(fonts()[0].path(), 44);
//    Logger::get()->set_use_log_file(true);

    m_movie_library->set_tweakable(false);
    m_ip_adresses->set_tweakable(false);
    register_property(m_movie_library);
    register_property(m_ip_adresses);

    register_property(m_movie_directory);
    register_property(m_movie_index);
    register_property(m_movie_path);
    register_property(m_movie_playlist);
    register_property(m_loop);
    register_property(m_auto_play);
    register_property(m_movie_volume);
    register_property(m_movie_speed);
    register_property(m_movie_delay);
    register_property(m_use_warping);
    observe_properties();
    add_tweakbar_for_component(shared_from_this());

    // check for command line input
    if(args().size() > 1 && fs::exists(args()[1])){ *m_movie_path = args()[1]; }

    // setup our components to receive rpc calls
    setup_rpc_interface();

    // scan for movie files
    search_movies();

    remote_control().set_components({ shared_from_this(), m_warp_component });
    load_settings();

    play_next_item();
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::update(float timeDelta)
{
    if(m_reload_movie)
    {
        set_clear_color(gl::Color(clear_color().rgb(), 0.f));
        m_movie->load(*m_movie_path, *m_auto_play, *m_loop,
                      media::MediaController::RenderTarget::TEXTURE,
                      media::MediaController::AudioTarget::AUDIO_JACK);
        m_movie->set_rate(*m_movie_speed);
        m_movie->set_volume(*m_movie_volume);
        m_movie->set_media_ended_callback([this](media::MediaControllerPtr mc)
        {
            play_next_item();
        });
        m_reload_movie = false;
    }

    m_needs_redraw = m_movie->copy_frame_to_texture(textures()[TEXTURE_INPUT]);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::draw()
{
    // background text
    gl::draw_text_2D(name(), fonts()[1], gl::COLOR_WHITE, vec2(35, 55));

    if(*m_use_warping){ m_warp_component->render_output(0, textures()[TEXTURE_INPUT]); }
    else{ gl::draw_texture(textures()[TEXTURE_INPUT], gl::window_dimension()); }

    if(!m_movie_start_timers.empty() && !m_movie_start_timers[0].has_expired())
    {
        gl::draw_text_2D(to_string(m_movie_start_timers[0].expires_from_now(), 1), fonts()[1],
                         gl::COLOR_WHITE, gl::window_dimension() / 2.f - vec2(50));
    }

    if(display_tweakbar())
    {
        gl::draw_text_2D(secs_to_time_str(m_movie->current_time()) + " / " +
                         secs_to_time_str(m_movie->duration()) + " - " +
                         fs::get_filename_part(m_movie->path()),
                         fonts()[1], gl::COLOR_WHITE, gl::vec2(10));
        draw_textures(textures());
    }
    
    m_needs_redraw = false;
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);

    switch (e.getCode())
    {
        case Key::_P:
            m_movie->is_playing() ? m_movie->pause() : m_movie->play();
            break;

        case Key::_LEFT:
            m_movie->seek_to_time(m_movie->current_time() - 5);
            break;

        case Key::_RIGHT:
            m_movie->seek_to_time(m_movie->current_time() + 5);
            break;
        case Key::_UP:
            m_movie->set_volume(m_movie->volume() + .1f);
            break;

        case Key::_DOWN:
            m_movie->set_volume(m_movie->volume() - .1f);
            break;

        default:
            break;
    }
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void MediaPlayer::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void MediaPlayer::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void MediaPlayer::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    start_playback(files.back());
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::teardown()
{
    stop_playback();
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);

    if(theProperty == m_movie_path)
    {
        m_reload_movie = true;
    }
    else if(theProperty == m_movie_index)
    {
        if(m_initiated && *m_movie_index >= 0 && *m_movie_index < (int)m_movie_library->value().size())
        {
            start_playback(m_movie_library->value()[*m_movie_index]);
        }
        else{ stop_playback(); }
    }
    else if(theProperty == m_movie_directory)
    {
        search_movies();
        fs::add_search_path(*m_movie_directory);
    }
    else if(theProperty == m_loop)
    {
        m_movie->set_loop(*m_loop);
    }
    else if(theProperty == m_movie_speed)
    {
        m_movie->set_rate(*m_movie_speed);
    }
    else if(theProperty == m_use_warping)
    {
        remove_tweakbar_for_component(m_warp_component);
        if(*m_use_warping){ add_tweakbar_for_component(m_warp_component); }
    }
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::play_next_item()
{
    // parse loop string
    auto splits = kinski::split(*m_movie_playlist, ',');
    std::vector<std::pair<int,float>> indices;
    
    for(const auto &s : splits)
    {
        auto values = split(s, ':');
        std::pair<int,float> v = std::make_pair(string_to<uint32_t>(values[0]), 0.0f);
        
        if(values.size() > 1){ v.second = string_to<float>(values[1]); }
        indices.push_back(v);
    }
    int next_index = indices.empty() ? *m_movie_index : 0;
    float next_delay = indices.empty() ? *m_movie_delay : 0;
    
    for(uint32_t i = 0; i < indices.size(); i++)
    {
        if(indices[i].first == m_movie_index->value())
        {
            next_index = indices[(i + (m_initiated ? 1 : 0)) % indices.size()].first;
            next_delay = indices[(i + (m_initiated ? 1 : 0)) % indices.size()].second;
        }
    }
    m_initiated = true;
    LOG_DEBUG << "next index: " << next_index;
    *m_movie_delay = next_delay;
    *m_movie_index = next_index;
}

/////////////////////////////////////////////////////////////////

std::string MediaPlayer::secs_to_time_str(float the_secs) const
{
    char buf[32];
    sprintf(buf, "%d:%02d:%.1f", (int)the_secs / 3600, ((int)the_secs / 60) % 60, fmodf(the_secs, 60));
    return buf;
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::start_playback(const std::string &the_path)
{
    stop_playback();

    // cancel timers
    for(auto & t : m_movie_start_timers){ t.cancel(); }
    m_movie_start_timers.resize(m_ip_adresses->value().size());

    std::vector<std::string> ip_adresses;

    if(*m_movie_delay > 0.f)
    {
        ip_adresses.assign(m_ip_adresses->value().begin(), m_ip_adresses->value().end());
    }
    else
    {
        ip_adresses.assign(m_ip_adresses->value().rbegin(), m_ip_adresses->value().rend());
    }

    float abs_delay = fabs(*m_movie_delay);

    for(uint8_t i = 0; i < m_movie_start_timers.size(); ++i)
    {
        auto ip = ip_adresses[i];

        // load movies
        net::async_send_tcp(background_queue().io_service(), "load " + fs::get_filename_part(the_path),
                            ip, g_remote_port);


        m_movie_start_timers[i] = Timer(background_queue().io_service(), [this, ip]()
        {
            net::async_send_tcp(background_queue().io_service(), "play", ip, g_remote_port);
        });

        m_movie_start_timers[i].expires_from_now(i * abs_delay + m_delay_static);
    }
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::stop_playback()
{
    for(const auto &ip : m_ip_adresses->value())
    {
        // unload movies
        net::async_send_tcp(background_queue().io_service(), "unload", ip, g_remote_port);
    }
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::search_movies()
{
    auto movs = fs::get_directory_entries(*m_movie_directory, fs::FileType::MOVIE, false);
    m_movie_library->value().assign(movs.begin(), movs.end());
}

/////////////////////////////////////////////////////////////////

bool MediaPlayer::needs_redraw() const
{
    return (m_movie && (!m_movie->is_playing() || !m_movie->has_video())) || !*m_use_warping
    || m_needs_redraw;
};

/////////////////////////////////////////////////////////////////

void MediaPlayer::setup_rpc_interface()
{
    remote_control().add_command("play");
    register_function("play", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty())
        {
            std::string p; for(const auto &arg : rpc_args){ p += arg + " "; }
            p = p.substr(0, p.size() - 1);
            *m_movie_path = p;
        }
        else{ m_movie->play(); }
    });
    remote_control().add_command("pause");
    register_function("pause", [this](const std::vector<std::string> &rpc_args)
    {
        m_movie->pause();
    });
    remote_control().add_command("load");
    register_function("load", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty())
        {
            std::string p; for(const auto &arg : rpc_args){ p += arg + " "; }
            p = p.substr(0, p.size() - 1);
            *m_movie_path = p;
        }
    });
    remote_control().add_command("unload");
    register_function("unload", [this](const std::vector<std::string> &rpc_args)
    {
        m_movie->unload();
        textures()[TEXTURE_INPUT].reset();
    });
    remote_control().add_command("set_volume");
    register_function("set_volume", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_movie->set_volume(kinski::string_to<float>(rpc_args.front())); }
    });

    remote_control().add_command("volume", [this](net::tcp_connection_ptr con,
                                                  const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_movie->set_volume(kinski::string_to<float>(rpc_args.front())); }
        con->write(to_string(m_movie->volume()));
    });

    remote_control().add_command("set_rate");
    register_function("set_rate", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_movie->set_rate(kinski::string_to<float>(rpc_args.front())); }
    });

    remote_control().add_command("rate", [this](net::tcp_connection_ptr con,
                                                const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_movie->set_rate(kinski::string_to<float>(rpc_args.front())); }
        con->write(to_string(m_movie->rate()));
    });

    remote_control().add_command("seek_to_time");
    register_function("seek_to_time", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty())
        {
            float secs = 0.f;

            auto splits = split(rpc_args.front(), ':');

            switch (splits.size())
            {
                case 3:
                    secs = kinski::string_to<float>(splits[2]) +
                           60.f * kinski::string_to<float>(splits[1]) +
                           3600.f * kinski::string_to<float>(splits[0]) ;
                    break;

                case 2:
                    secs = kinski::string_to<float>(splits[1]) +
                    60.f * kinski::string_to<float>(splits[0]);
                    break;

                case 1:
                    secs = kinski::string_to<float>(splits[0]);
                    break;

                default:
                    break;
            }
            m_movie->seek_to_time(secs);
        }
    });

    remote_control().add_command("current_time", [this](net::tcp_connection_ptr con,
                                                        const std::vector<std::string> &rpc_args)
    {
        con->write(to_string(m_movie->current_time(), 1));
    });

    remote_control().add_command("duration", [this](net::tcp_connection_ptr con,
                                                    const std::vector<std::string> &rpc_args)
    {
        con->write(to_string(m_movie->duration(), 1));
    });

    remote_control().add_command("set_loop");
    register_function("set_loop", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_movie->set_loop(kinski::string_to<bool>(rpc_args.front())); }
    });

    remote_control().add_command("loop", [this](net::tcp_connection_ptr con,
                                                const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_movie->set_loop(kinski::string_to<bool>(rpc_args.front())); }
        con->write(to_string(m_movie->loop()));
    });

    remote_control().add_command("is_playing", [this](net::tcp_connection_ptr con,
                                                const std::vector<std::string> &rpc_args)
    {
        con->write(to_string(m_movie->is_playing()));
    });
}
