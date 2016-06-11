//
//  MediaPlayer.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "MediaPlayer.hpp"
#include <mutex>

using namespace std;
using namespace kinski;
using namespace glm;

namespace
{
    uint16_t g_remote_port = 33333;
    uint16_t g_discovery_listen_port = 55555;
    std::mutex g_ip_table_mutex;
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::setup()
{
    ViewerApp::setup();
    fonts()[1].load(fonts()[0].path(), 44);
//    Logger::get()->set_use_log_file(true);
    
    m_movie_library->setTweakable(false);
    m_ip_adresses_static->setTweakable(false);
    register_property(m_movie_library);
    register_property(m_ip_adresses_static);
    register_property(m_movie_directory);
    register_property(m_movie_index);
    register_property(m_movie_path);
    register_property(m_loop);
    register_property(m_volume);
    register_property(m_auto_play);
    register_property(m_playback_speed);
    register_property(m_movie_delay);
    register_property(m_movie_delay_static);
    register_property(m_load_remote_movies);
    register_property(m_use_warping);
    register_property(m_force_audio_jack);
    observe_properties();
    add_tweakbar_for_component(shared_from_this());

    // warp component
    m_warp = std::make_shared<WarpComponent>();
    m_warp->observe_properties();

    // check for command line input
    if(args().size() > 1 && fs::file_exists(args()[1])){ *m_movie_path = args()[1]; }
    
    // setup our components to receive rpc calls
    setup_rpc_interface();
    
    // scan for movie files
    search_movies();
    
    // schedule rescan every 10 secs
    m_timer_movie_search = Timer(background_queue().io_service(),
                                 std::bind(&MediaPlayer::search_movies, this));
    m_timer_movie_search.set_periodic(true);
    m_timer_movie_search.expires_from_now(10.f);
    
    // discovery udp-server to receive pings from existing nodes in the network
    m_udp_server = net::udp_server(background_queue().io_service());
    m_udp_server.start_listen(g_discovery_listen_port);
    m_udp_server.set_receive_function([this](const std::vector<uint8_t> &data,
                                             const std::string &remote_ip, uint16_t remote_port)
    {
        string str(data.begin(), data.end());
        LOG_TRACE_1 << str << " " << remote_ip << " (" << remote_port << ")";
        
        if((str == "media_player") /*== !kinski::is_in(remote_ip, m_ip_adresses_dynamic)*/)
        {
            std::unique_lock<std::mutex> lock(g_ip_table_mutex);
            m_ip_adresses_dynamic[remote_ip] = getApplicationTime();
        }
    });
    
    remote_control().set_components({ shared_from_this(), m_warp });
    load_settings();
    
    if(*m_movie_index >= 0 && *m_movie_index < (int)m_movie_library->value().size())
    {
        start_playback(m_movie_library->value()[*m_movie_index]);
    }
    m_initiated = true;
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::update(float timeDelta)
{
    if(m_reload_movie){ reload_movie(); }
    
    m_movie->copy_frame_to_texture(textures()[TEXTURE_INPUT]);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::draw()
{
    // background text
    if(!m_movie->is_playing())
    { gl::draw_text_2D(name(), fonts()[1], gl::COLOR_WHITE, vec2(75, 55)); }
    
    if(!m_movie_start_timers.empty() && !m_movie_start_timers[0].has_expired())
    {
        gl::draw_text_2D(as_string(m_movie_start_timers[0].expires_from_now(), 1), fonts()[1],
                         gl::COLOR_WHITE, gl::window_dimension() / 2.f - vec2(50));
    }
    else
    {
        if(*m_use_warping){ m_warp->render_output(textures()[TEXTURE_INPUT]); }
        else{ gl::draw_texture(textures()[TEXTURE_INPUT], gl::window_dimension()); }
    }
    
    if(displayTweakBar())
    {
        gl::draw_text_2D(secs_to_time_str(m_movie->current_time()) + " / " +
                         secs_to_time_str(m_movie->duration()) + " - " +
                         fs::get_filename_part(m_movie->path()),
                         fonts()[1], gl::COLOR_WHITE, gl::vec2(10));
        draw_textures(textures());
    }
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);

    switch (e.getCode())
    {
        case Key::_C:
            if(m_camera_control->is_capturing())
                m_camera_control->stop_capture();
            else
                m_camera_control->start_capture();
            break;

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

void MediaPlayer::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
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

void MediaPlayer::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    start_playback(files.back());
//    *m_movie_path = files.back();
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::tearDown()
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
    else if(theProperty == m_volume)
    {
        m_movie->set_volume(*m_volume);
    }
    else if(theProperty == m_playback_speed)
    {
        m_movie->set_rate(*m_playback_speed);
    }
    else if(theProperty == m_use_warping)
    {
        remove_tweakbar_for_component(m_warp);
        if(*m_use_warping){ add_tweakbar_for_component(m_warp); }
    }
}

/////////////////////////////////////////////////////////////////

bool MediaPlayer::save_settings(const std::string &path)
{
    bool ret = ViewerApp::save_settings(path);
    try
    {
        Serializer::saveComponentState(m_warp,
                                       fs::join_paths(path ,"warp_config.json"),
                                       PropertyIO_GL());
    }
    catch(Exception &e){ LOG_ERROR << e.what(); return false; }
    return ret;
}

/////////////////////////////////////////////////////////////////

bool MediaPlayer::load_settings(const std::string &path)
{
    bool ret = ViewerApp::load_settings(path);
    try
    {
        Serializer::loadComponentState(m_warp,
                                       fs::join_paths(path , "warp_config.json"),
                                       PropertyIO_GL());
    }
    catch(Exception &e){ LOG_ERROR << e.what(); return false; }
    return ret;
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
    
    fs::add_search_path(fs::get_directory_part(the_path));
    
    auto ip_adresses = get_remote_adresses();
    
    // cancel timers
    for(auto &t : m_movie_start_timers){ t.cancel(); }
    m_movie_start_timers.resize(ip_adresses.size());
    
    float abs_delay = fabs(*m_movie_delay);
    
    for(uint8_t i = 0; i < ip_adresses.size(); ++i)
    {
        auto ip = ip_adresses[i];
        
        string start_cmd = "play";
        
        // load movies
        if(*m_load_remote_movies || m_movie_path->value().empty())
        {
            net::async_send_tcp(background_queue().io_service(),
                                "load " + fs::get_filename_part(the_path),
                                ip, g_remote_port);
        }
        else{ start_cmd = "restart"; }
        
        m_movie_start_timers[i] = Timer(background_queue().io_service(), [this, ip, start_cmd]()
        {
            net::async_send_tcp(background_queue().io_service(), start_cmd, ip, g_remote_port);
        });
        
        m_movie_start_timers[i].expires_from_now(i * abs_delay + *m_movie_delay_static);
    }
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::stop_playback()
{
    if(*m_load_remote_movies)
    {
        for(const auto &ip : get_remote_adresses())
        {
            // unload movies
            net::async_send_tcp(background_queue().io_service(), "unload", ip, g_remote_port);
        }
    }
}

/////////////////////////////////////////////////////////////////

void MediaPlayer::search_movies()
{
    auto movs = get_directory_entries(*m_movie_directory, fs::FileType::MOVIE, false);
    m_movie_library->value().assign(movs.begin(), movs.end());
}

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
    remote_control().add_command("restart", [this](net::tcp_connection_ptr con,
                                                   const std::vector<std::string> &rpc_args)
    {
        m_movie->restart();
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
        if(!rpc_args.empty()){ m_movie->set_volume(kinski::string_as<float>(rpc_args.front())); }
    });
    
    remote_control().add_command("volume", [this](net::tcp_connection_ptr con,
                                                  const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_movie->set_volume(kinski::string_as<float>(rpc_args.front())); }
        con->send(as_string(m_movie->volume()));
    });
    
    remote_control().add_command("set_rate");
    register_function("set_rate", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_movie->set_rate(kinski::string_as<float>(rpc_args.front())); }
    });
    
    remote_control().add_command("rate", [this](net::tcp_connection_ptr con,
                                                const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_movie->set_rate(kinski::string_as<float>(rpc_args.front())); }
        con->send(as_string(m_movie->rate()));
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
                    secs = kinski::string_as<float>(splits[2]) +
                           60.f * kinski::string_as<float>(splits[1]) +
                           3600.f * kinski::string_as<float>(splits[0]) ;
                    break;
                    
                case 2:
                    secs = kinski::string_as<float>(splits[1]) +
                    60.f * kinski::string_as<float>(splits[0]);
                    break;
                    
                case 1:
                    secs = kinski::string_as<float>(splits[0]);
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
        con->send(as_string(m_movie->current_time(), 1));
    });
    
    remote_control().add_command("duration", [this](net::tcp_connection_ptr con,
                                                    const std::vector<std::string> &rpc_args)
    {
        con->send(as_string(m_movie->duration(), 1));
    });
    
    remote_control().add_command("set_loop");
    register_function("set_loop", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_movie->set_loop(kinski::string_as<bool>(rpc_args.front())); }
    });
    
    remote_control().add_command("loop", [this](net::tcp_connection_ptr con,
                                                const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ m_movie->set_loop(kinski::string_as<bool>(rpc_args.front())); }
        con->send(as_string(m_movie->loop()));
    });
    
    remote_control().add_command("is_playing", [this](net::tcp_connection_ptr con,
                                                const std::vector<std::string> &rpc_args)
    {
        con->send(as_string(m_movie->is_playing()));
    });
}

std::vector<std::string> MediaPlayer::get_remote_adresses() const
{
    std::vector<std::string> ret;
    
    if(*m_movie_delay > 0.f)
    {
        ret.assign(m_ip_adresses_static->value().begin(), m_ip_adresses_static->value().end());
    }
    else
    {
        ret.assign(m_ip_adresses_static->value().rbegin(), m_ip_adresses_static->value().rend());
    }
    std::unique_lock<std::mutex> lock(g_ip_table_mutex);
    for(const auto& pair : m_ip_adresses_dynamic){ ret.push_back(pair.first); }
    return ret;
}

void MediaPlayer::reload_movie()
{
    m_reload_movie = false;
    
    auto render_target = *m_use_warping ? media::MediaController::RenderTarget::TEXTURE :
    media::MediaController::RenderTarget::SCREEN;
    
    //        if(render_target == media::MediaController::RenderTarget::SCREEN)
    //        { set_clear_color(gl::Color(clear_color().rgb(), 0.f)); }
    
    auto audio_target = *m_force_audio_jack ? media::MediaController::AudioTarget::AUDIO_JACK :
    media::MediaController::AudioTarget::AUTO;
    
    m_movie->load(*m_movie_path, *m_auto_play, *m_loop, render_target, audio_target);
    m_movie->set_rate(*m_playback_speed);
    m_movie->set_volume(*m_volume);
    
    m_movie->set_media_ended_callback([this](media::MediaControllerPtr the_movie)
                                      {
                                          if(*m_loop){ start_playback(*m_movie_path); }
                                      });
    
    if(!*m_load_remote_movies){ start_playback(*m_movie_path); }
}
