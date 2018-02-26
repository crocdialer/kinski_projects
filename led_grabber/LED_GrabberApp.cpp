//
//  LED_GrabberApp.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "LED_GrabberApp.hpp"
#include "sensors/sensors.h"
#include <mutex>

using namespace std;
using namespace kinski;
using namespace glm;


namespace
{
    //! enforce mutual exlusion on state
    std::mutex g_ip_table_mutex;

    //! interval to send sync cmd (secs)
    const double g_sync_interval = 0.05;

    //! keep_alive timeout after which a remote node is considered dead (secs)
    const double g_dead_thresh = 10.0;

    //! interval for keep_alive broadcasts (secs)
    const double g_broadcast_interval = 2.0;

    //! minimum difference to remote media-clock for fine-tuning (secs)
    double g_sync_thresh = 0.02;
    
    //! minimum difference to remote media-clock for scrubbing (secs)
    const double g_scrub_thresh = 1.0;
    
    //! force reset of playback speed (secs)
    const double g_sync_duration = 1.0;
    
    //!
    uint16_t g_led_tcp_port = 44444;
    
    double g_led_refresh_interval = 0.04;
}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::setup()
{
    ViewerApp::setup();
    Logger::get()->set_use_log_file(true);

    fonts()[1].load(fonts()[0].path(), 28);
    register_property(m_media_path);
    register_property(m_scale_to_fit);
    register_property(m_loop);
    register_property(m_auto_play);
    register_property(m_volume);
    register_property(m_brightness);
    register_property(m_playback_speed);
    register_property(m_force_audio_jack);
    register_property(m_is_master);
    register_property(m_use_discovery_broadcast);
    register_property(m_broadcast_port);
    register_property(m_cam_index);
    register_property(m_show_cam_overlay);
    register_property(m_calibration_thresh);
    register_property(m_led_calib_color);
    register_property(m_led_channels);
    m_calibration_points->set_tweakable(false);
    register_property(m_calibration_points);
    register_property(m_led_res);
    register_property(m_downsample_res);
    observe_properties();
    add_tweakbar_for_component(shared_from_this());

    remote_control().set_components({ shared_from_this(), m_warp_component });
//    set_default_config_path("~/");

    // setup our components to receive rpc calls
    setup_rpc_interface();

    load_settings();
    
    // check for command line input
    if(args().size() > 1)
    {
        fs::path p = args()[1];
        
        if(fs::exists(p))
        {
            if(fs::is_directory(p))
            {
                create_playlist(p);
                
                m_scan_media_timer = Timer(background_queue().io_service(),
                                           [this, p](){ create_playlist(p); });
                m_scan_media_timer.set_periodic();
                m_scan_media_timer.expires_from_now(5.f);
                
                fs::add_search_path(p, 3);
            }
            else{ *m_media_path = p; }
        }
    }
    m_ip_adress = net::local_ip();
    m_check_ip_timer = Timer(background_queue().io_service(), [this]()
    {
        auto fetched_ip = net::local_ip();
        main_queue().submit([this, fetched_ip](){ m_ip_adress = fetched_ip; });
    });
    m_check_ip_timer.set_periodic();
    m_check_ip_timer.expires_from_now(5.f);
    
    // grabber setup
    m_led_grabber->set_resolution(m_led_res->value().x, m_led_res->value().y);
    m_led_update_timer = Timer(main_queue().io_service());
    
    // connect serial
    auto query_cb = [this](const std::string &the_id, ConnectionPtr the_device)
    {
        if(the_id == "LEDS" && m_led_grabber->connect(the_device))
        {
            LOG_DEBUG << "grabber connected: " << the_device->description();
        }
    };
    sensors::scan_for_serials(background_queue().io_service(), query_cb);
}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::update(float timeDelta)
{
    if(m_reload_media){ reload_media(); }
    
    if(m_runmode == MODE_DEFAULT)
    {
        if(m_camera){ m_camera->copy_frame_to_texture(textures()[TEXTURE_CAM_INPUT]); }
        if(m_media)
        {
            bool has_new_image = m_media->copy_frame_to_texture(textures()[TEXTURE_INPUT], true);
            
            if(has_new_image && m_led_update_timer.has_expired())
            {
                auto tex_size = textures()[TEXTURE_INPUT].size();
                
                if(tex_size.x > m_fbo_downsample.size().x || tex_size.y > m_fbo_downsample.size().y)
                {
                    gl::render_to_texture(m_fbo_downsample, [this]()
                    {
                        gl::draw_texture(textures()[TEXTURE_INPUT], gl::window_dimension());
                    });
                    m_image_input = gl::create_image_from_framebuffer(m_fbo_downsample);
                }
                else{ m_image_input = gl::create_image_from_texture(textures()[TEXTURE_INPUT]); }
                
                m_led_grabber->set_warp(m_warp_component->quad_warp(0));
                m_led_grabber->grab_from_image_calib(m_image_input);
                m_led_update_timer.expires_from_now(g_led_refresh_interval);
            }
            m_needs_redraw = has_new_image || m_needs_redraw;
        }
        else{ m_needs_redraw = true; }
    }
    else if(m_runmode == MODE_MANUAL_CALIBRATION)
    {
        m_needs_redraw = true;
        
        if(m_led_update_timer.has_expired())
        {
            m_led_grabber->set_warp(m_warp_component->quad_warp(0));
            m_led_grabber->show_segment(m_current_calib_segment);
            m_led_update_timer.expires_from_now(g_led_refresh_interval);
        }
    }
}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::draw()
{
    gl::clear();
    
    // draw quad warp
    if(m_warp_component->enabled(0))
    {
        m_warp_component->render_output(0, textures()[TEXTURE_INPUT]);
    }
    
    // draw camera input, if any
    if((m_camera->is_capturing() || *m_show_cam_overlay) && textures()[TEXTURE_CAM_INPUT])
    {
        auto mat = gl::Material::create();
        mat->set_blending();
        mat->set_depth_test(false);
        mat->set_depth_write(false);
        mat->set_diffuse(gl::Color(1, 1, 1, 0.8f));
        mat->set_textures({textures()[TEXTURE_CAM_INPUT]});
        gl::draw_quad(gl::window_dimension(), mat);
    }
    
    if(m_runmode == MODE_MANUAL_CALIBRATION)
    {
        gl::draw_text_2D("segment: " + to_string(m_current_calib_segment), fonts()[1],
                         gl::COLOR_WHITE, vec2(50));
    }
    
    // draw calibration points
    {
        auto points_tmp = m_calibration_points->value();
        for(auto &p : points_tmp){ p = p * gl::window_dimension(); }
        
        gl::draw_points_2D(points_tmp, gl::COLOR_WHITE, 3.f);
        gl::reset_state();
    }
    
    if(!*m_is_master && m_is_syncing && Logger::get()->severity() >= Severity::DEBUG)
    {
        gl::draw_text_2D(to_string(m_is_syncing) + " ms", fonts()[1], gl::COLOR_WHITE, vec2(50));
    }
    if(display_tweakbar())
    {
        // media title
        gl::draw_text_2D(m_media->is_loaded() ? fs::get_filename_part(m_media->path()) : *m_media_path,
                         fonts()[1], m_media->is_loaded() ? gl::COLOR_WHITE : gl::COLOR_RED, gl::vec2(10));
        
        // time + playlist position
        auto str = secs_to_time_str(m_media->current_time()) + " / " +
            secs_to_time_str(m_media->duration());
        str += m_playlist.empty() ? "" : format(" (%d / %d)", m_current_playlist_index + 1,
                                                m_playlist.size());
        
        gl::draw_text_2D(str, fonts()[1], gl::COLOR_WHITE, gl::vec2(10, 40));
        
        // ip-adress
        gl::draw_text_2D(m_ip_adress, fonts()[1],
                         m_ip_adress == net::UNKNOWN_IP ? gl::COLOR_RED : gl::COLOR_WHITE,
                         gl::vec2(10, 70));
        draw_textures(textures());
    }
    m_needs_redraw = false;
}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);
    
    if(!e.isAltDown())
    {
        switch (e.getCode())
        {
            case Key::_P:
                m_media->is_playing() ? m_media->pause() : m_media->play();
                if(*m_is_master){ send_network_cmd(m_media->is_playing() ? "play" : "pause"); }
                break;
            
            case Key::_C:
                m_camera->is_capturing() ? m_camera->stop_capture() : m_camera->start_capture();
                break;
                
            case Key::_LEFT:
                if(m_runmode == MODE_DEFAULT)
                {
                    m_media->seek_to_time(m_media->current_time() - (e.isShiftDown() ? 30 : 5));
                    m_needs_redraw = true;
                }
                else if(m_runmode == MODE_MANUAL_CALIBRATION)
                {
                    m_current_calib_segment = m_current_calib_segment == 0 ?
                        (size_t)(m_led_res->value().y) - 1 : m_current_calib_segment - 1;
                    m_last_calib_click = gl::vec2(-1);
                }
                break;
                
            case Key::_RIGHT:
                if(m_runmode == MODE_DEFAULT)
                {
                    m_media->seek_to_time(m_media->current_time() + (e.isShiftDown() ? 30 : 5));
                    m_needs_redraw = true;
                }
                else if(m_runmode == MODE_MANUAL_CALIBRATION)
                {
                    m_current_calib_segment = (m_current_calib_segment + 1) % (size_t)(m_led_res->value().y);
                    m_last_calib_click = gl::vec2(-1);
                }
                break;
            case Key::_UP:
                *m_volume += .1f;
                break;
                
            case Key::_DOWN:
                *m_volume -= .1f;
                break;
            
            case Key::_PAGE_UP:
                if(!m_playlist.empty())
                {
                    m_current_playlist_index = (m_current_playlist_index + 1) % m_playlist.size();
                    *m_media_path = m_playlist[m_current_playlist_index];
                }
                break;
                
            case Key::_PAGE_DOWN:
                if(!m_playlist.empty())
                {
                    int next_index = m_current_playlist_index - 1;
                    next_index += next_index < 0 ? m_playlist.size() : 0;
                    m_current_playlist_index = next_index;
                    *m_media_path = m_playlist[m_current_playlist_index];
                }
                break;
                
            case Key::_L:
                {
                    m_camera->stop_capture();
                    m_led_grabber->set_warp(m_warp_component->quad_warp(0));
                    
                    background_queue().submit([this]()
                    {
                        auto points = m_led_grabber->run_calibration(*m_cam_index,
                                                                     *m_calibration_thresh,
                                                                     *m_led_calib_color);
                        main_queue().submit([this, points]()
                        {
                            m_calibration_points->set_value(std::move(points));
                        });
                    });
                }
                break;
                
            case Key::_M:
                if(m_runmode == MODE_DEFAULT)
                {
                    set_runmode(MODE_MANUAL_CALIBRATION);
                }
                else{ set_runmode(MODE_DEFAULT); }
                break;
            default:
                break;
        }
    }
}

/////////////////////////////////////////////////////////////////

bool LED_GrabberApp::needs_redraw() const
{
    return (m_media && (!m_media->is_playing() || !m_media->has_video())) || !*m_use_warping
        || m_needs_redraw || m_is_syncing;
};

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::mouse_press(const MouseEvent &e)
{
    if(m_runmode == MODE_MANUAL_CALIBRATION)
    {
        process_calib_click(e.getPos());
    }
    else{ ViewerApp::mouse_press(e); }
}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);
}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{

}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    *m_media_path = files.back();
}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::teardown()
{
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);

    if(theProperty == m_media_path)
    {
        m_reload_media = true;
    }
#ifdef KINSKI_ARM
    else if(theProperty == m_use_warping)
    {
        m_reload_media = true;
    }
#endif
    else if(theProperty == m_loop)
    {
        m_media->set_loop(*m_loop);
    }
    else if(theProperty == m_volume)
    {
        m_media->set_volume(*m_volume);
    }
    else if(theProperty == m_playback_speed)
    {
        m_media->set_rate(*m_playback_speed);
        if(*m_is_master){ send_network_cmd("set_rate " + to_string(m_playback_speed->value(), 2)); }
    }
    else if(theProperty == m_use_discovery_broadcast || theProperty == m_broadcast_port)
    {
        if(*m_use_discovery_broadcast && !*m_is_master)
        {
            // setup a periodic udp-broadcast to enable discovery of this node
            m_broadcast_timer = Timer(main_queue().io_service(), [this]()
            {
                LOG_TRACE_2 << "sending discovery_broadcast on udp-port: " << m_broadcast_port->value();
                net::async_send_udp_broadcast(background_queue().io_service(), name(),
                                              *m_broadcast_port);
            });
            m_broadcast_timer.set_periodic();
            m_broadcast_timer.expires_from_now(g_broadcast_interval);
        }else{ m_broadcast_timer.cancel(); }
    }
    else if(theProperty == m_is_master)
    {
        if(*m_is_master)
        {
            *m_use_discovery_broadcast = false;
            m_ip_timestamps.clear();

            // discovery udp-server to receive pings from existing nodes in the network
            m_udp_server = net::udp_server(background_queue().io_service());
            m_udp_server.start_listen(*m_broadcast_port);
            m_udp_server.set_receive_function([this](const std::vector<uint8_t> &data,
                                                     const std::string &remote_ip, uint16_t remote_port)
            {
                string str(data.begin(), data.end());
                LOG_TRACE_1 << str << " " << remote_ip << " (" << remote_port << ")";

                if(str == name())
                {
                    std::unique_lock<std::mutex> lock(g_ip_table_mutex);
                    m_ip_timestamps[remote_ip] = get_application_time();
                    
                    ping_delay(remote_ip);
                }
                else if(str == LED_Grabber::id())
                {
                    bool is_new_connection = true;

                    for(const auto &c : m_led_grabber->connections())
                    {
                        auto tcp_con = std::dynamic_pointer_cast<net::tcp_connection>(c);
                        if(tcp_con && tcp_con->remote_ip() == remote_ip){ is_new_connection = false; break;}
                    }

                    if(is_new_connection)
                    {
                        auto con = net::tcp_connection::create(background_queue().io_service(),
                                                               remote_ip, g_led_tcp_port);
                        con->set_connect_cb([this](ConnectionPtr c)
                        {
                            if(m_led_grabber->connect(c))
                            {
                                LOG_DEBUG << "grabber connected: " << c->description();
                            }
                        });
                    }
                }
            });
            begin_network_sync();
        }
        else
        {
            m_udp_server.stop_listen();
            m_sync_timer.cancel();
            m_use_discovery_broadcast->notify_observers();
            
            m_sync_off_timer = Timer(background_queue().io_service(), [this]()
            {
                m_media->set_rate(*m_playback_speed);
                m_is_syncing = 0;
            });
        }
    }
    else if(theProperty == m_cam_index)
    {
        m_camera->stop_capture();
        m_camera = media::CameraController::create(*m_cam_index);
    }
    else if(theProperty == m_led_channels)
    {
        m_led_grabber->set_brightness(*m_led_channels);
    }
    else if(theProperty == m_calibration_points)
    {
        m_led_grabber->set_calibration_points(m_calibration_points->value());
    }
    else if(theProperty == m_led_res)
    {
        m_led_grabber->set_resolution(m_led_res->value().x, m_led_res->value().y);
    }
    else if(theProperty == m_downsample_res)
    {
        m_fbo_downsample = gl::Fbo(m_downsample_res->value());
    }
}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::reload_media()
{
    App::Task t(this);

    textures()[TEXTURE_INPUT].reset();
    m_sync_off_timer.cancel();

    std::string abs_path;
    try{ abs_path = fs::search_file(*m_media_path); }
    catch (fs::FileNotFoundException &e){ LOG_DEBUG << e.what(); m_reload_media = false; return; }

    auto media_type = fs::get_file_type(abs_path);

    LOG_DEBUG << "loading: " << m_media_path->value();

    if(fs::is_uri(*m_media_path) ||
       media_type == fs::FileType::AUDIO ||
       media_type == fs::FileType::MOVIE)
    {
        auto render_target = *m_use_warping ? media::MediaController::RenderTarget::TEXTURE :
        media::MediaController::RenderTarget::SCREEN;

        auto audio_target = *m_force_audio_jack ? media::MediaController::AudioTarget::BOTH :
        media::MediaController::AudioTarget::AUTO;

        if(render_target == media::MediaController::RenderTarget::SCREEN)
        { set_clear_color(gl::Color(clear_color().rgb(), 0.f)); }

        m_media->set_media_ended_callback([this](media::MediaControllerPtr mc)
        {
            LOG_DEBUG << "media ended";
            
            if(*m_is_master && *m_loop)
            {
                send_network_cmd("restart");
                begin_network_sync();
            }
            else if(!m_playlist.empty())
            {
                main_queue().submit([this]()
                {
                    m_current_playlist_index = (m_current_playlist_index + 1) % m_playlist.size();
                    *m_media_path = m_playlist[m_current_playlist_index];
                });
            }
        });
        
        m_media->set_on_load_callback([this](media::MediaControllerPtr mc)
        {
            if(m_media->has_video() && m_media->fps() > 0)
            {
                g_sync_thresh = 1.0 / m_media->fps() / 2.0;
                LOG_DEBUG << "media fps: " << to_string(m_media->fps(), 2);
            }
            m_media->set_rate(*m_playback_speed);
            m_media->set_volume(*m_volume);
        });
        m_media->load(abs_path, *m_auto_play, *m_loop, render_target, audio_target);
    }
    else if(media_type == fs::FileType::IMAGE)
    {
        m_media->unload();
        textures()[TEXTURE_INPUT] = gl::create_texture_from_file(abs_path);
    }
    else if(media_type == fs::FileType::FONT)
    {
        m_media->unload();
        gl::Font font;
        font.load(abs_path, 44);
        textures()[TEXTURE_INPUT] = font.create_texture("The quick brown fox \njumps over the lazy dog ... \n0123456789");
    }
    m_reload_media = false;

    // network sync
    if(*m_is_master)
    {
        send_network_cmd("load " + fs::get_filename_part(*m_media_path));
        begin_network_sync();
    }
}

/////////////////////////////////////////////////////////////////

std::string LED_GrabberApp::secs_to_time_str(float the_secs) const
{
    return format("%d:%02d:%04.1f", (int) the_secs / 3600, ((int) the_secs / 60) % 60,
                  fmodf(the_secs, 60));
}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::sync_media_to_timestamp(double the_timestamp)
{
    auto diff = the_timestamp - m_media->current_time();
    
    if(m_media->is_playing())
    {
        m_is_syncing = diff * 1000.0;
        
        // adapt to playback rate
        auto scrub_thresh = g_scrub_thresh / *m_playback_speed;
        auto sync_thresh = g_sync_thresh / *m_playback_speed;
        
        if((abs(diff) > scrub_thresh))
        {
            m_media->seek_to_time(the_timestamp);
        }
        else if(abs(diff) > sync_thresh)
        {
            auto rate = *m_playback_speed * (1.0 + sgn(diff) * 0.05 + 0.75 * diff / scrub_thresh);
            m_media->set_rate(rate);
            m_sync_off_timer.expires_from_now(g_sync_duration);
        }
        else
        {
            m_media->set_rate(*m_playback_speed);
            m_is_syncing = 0;
        }
    }
}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::begin_network_sync()
{
    m_sync_timer = Timer(background_queue().io_service(), [this]()
    {
        if(m_media && m_media->is_playing()){ send_sync_cmd(); }
    });
    m_sync_timer.set_periodic();
    m_sync_timer.expires_from_now(g_sync_interval);
}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::send_sync_cmd()
{
    remove_dead_ip_adresses();
    
    std::unique_lock<std::mutex> lock(g_ip_table_mutex);
    bool use_udp = true;
    
    for(auto &pair : m_ip_roundtrip)
    {
        double sync_delay = median(pair.second) * (use_udp ? 0.5 : 1.5);
        string cmd = "seek_to_time " + to_string(m_media->current_time() + sync_delay, 3);
        
        if(use_udp)
        {
            net::async_send_udp(background_queue().io_service(), cmd, pair.first,
                                remote_control().udp_port());
        }
        else
        {
            net::async_send_tcp(background_queue().io_service(), cmd, pair.first,
                                remote_control().tcp_port());
        }
    }
}

void LED_GrabberApp::remove_dead_ip_adresses()
{
    std::unique_lock<std::mutex> lock(g_ip_table_mutex);
    
    auto now = get_application_time();
    std::list<std::unordered_map<std::string, float>::iterator> dead_iterators;
    
    auto it = m_ip_timestamps.begin();
    
    for(; it != m_ip_timestamps.end(); ++it)
    {
        if(now - it->second >= g_dead_thresh){ dead_iterators.push_back(it); }
    }
    
    // remove dead iterators
    for(auto &dead_it : dead_iterators)
    {
        m_ip_roundtrip.erase(dead_it->first);
        m_ip_timestamps.erase(dead_it);
    }
}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::send_network_cmd(const std::string &the_cmd)
{
    remove_dead_ip_adresses();
    
    std::unique_lock<std::mutex> lock(g_ip_table_mutex);

    auto it = m_ip_timestamps.begin();

    for(; it != m_ip_timestamps.end(); ++it)
    {
        net::async_send_tcp(background_queue().io_service(), the_cmd, it->first,
                            remote_control().tcp_port());
    }
}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::ping_delay(const std::string &the_ip)
{
    Stopwatch timer;
    timer.start();
    
    auto con = net::tcp_connection::create(background_queue().io_service(), the_ip,
                                           remote_control().tcp_port());
    auto receive_func = [this, timer, con](net::tcp_connection_ptr ptr,
                                           const std::vector<uint8_t> &data)
    {
        std::unique_lock<std::mutex> lock(g_ip_table_mutex);
        
        // we measured 2 roundtrips
        auto delay = timer.time_elapsed() * 0.5;
        
        auto it = m_ip_roundtrip.find(ptr->remote_ip());
        if(it == m_ip_roundtrip.end())
        {
            m_ip_roundtrip[ptr->remote_ip()] = CircularBuffer<double>(5);
            m_ip_roundtrip[ptr->remote_ip()].push_back(delay);
        }
        else{ it->second.push_back(delay); }
        
        LOG_TRACE << ptr->remote_ip() << " (roundtrip, last 10s): "
            << (int)(1000.0 * mean(m_ip_roundtrip[ptr->remote_ip()])) << " ms";
        
//        ptr->close();
        con->set_tcp_receive_cb();
    };
    con->set_connect_cb([this](ConnectionPtr the_con){ the_con->write("echo ping"); });
    con->set_tcp_receive_cb(receive_func);
}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::create_playlist(const std::string &the_base_dir)
{
    std::map<fs::FileType, std::vector<fs::path>> files;
    files[fs::FileType::MOVIE] = {};
    files[fs::FileType::AUDIO] = {};
    
    for(const auto &p : fs::get_directory_entries(the_base_dir, "", 3))
    {
        files[fs::get_file_type(p)].push_back(p);
    }
    auto file_list = concat_containers<fs::path>(files[fs::FileType::MOVIE], files[fs::FileType::AUDIO]);
    
    if(file_list.size() != m_playlist.size())
    {
        main_queue().submit([this, file_list]()
        {
            m_current_playlist_index = 0;
            m_playlist = file_list;
            *m_media_path = m_playlist[0];
        });
    }
}

/////////////////////////////////////////////////////////////////

void LED_GrabberApp::setup_rpc_interface()
{
    remote_control().add_command("play");
    register_function("play", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty())
        {
            std::string p; for(const auto &arg : rpc_args){ p += arg + " "; }
            p = p.substr(0, p.size() - 1);
            *m_media_path = p;
        }
        else
        {
            m_media->play();
            m_sync_off_timer.cancel();
            if(*m_is_master){ send_network_cmd("play"); }
        }
    });
    remote_control().add_command("pause");
    register_function("pause", [this](const std::vector<std::string> &rpc_args)
    {
        m_media->pause();
        m_sync_off_timer.cancel();
        if(*m_is_master){ send_network_cmd("pause"); }
    });
    remote_control().add_command("restart", [this](net::tcp_connection_ptr con,
                                                   const std::vector<std::string> &rpc_args)
    {
        m_media->restart();
        m_sync_off_timer.cancel();
    });
    remote_control().add_command("load");
    register_function("load", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty())
        {
            std::string p; for(const auto &arg : rpc_args){ p += arg + " "; }
            p = p.substr(0, p.size() - 1);
            *m_media_path = p;
        }
    });
    remote_control().add_command("unload");
    register_function("unload", [this](const std::vector<std::string> &rpc_args)
    {
        m_media->unload();
        textures()[TEXTURE_INPUT].reset();
    });
    remote_control().add_command("set_volume");
    register_function("set_volume", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ *m_volume = kinski::string_to<float>(rpc_args.front()); }
    });

    remote_control().add_command("volume", [this](net::tcp_connection_ptr con,
                                                  const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ *m_volume = kinski::string_to<float>(rpc_args.front()); }
        else{ con->write(to_string(m_media->volume())); }
    });

    remote_control().add_command("brightness", [this](net::tcp_connection_ptr con,
                                                      const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ *m_brightness = kinski::string_to<float>(rpc_args.front()); }
        else{ con->write(to_string(m_brightness->value())); }
    });

    remote_control().add_command("set_brightness", [this](net::tcp_connection_ptr con,
                                                          const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ *m_brightness = kinski::string_to<float>(rpc_args.front()); }
    });

    remote_control().add_command("set_rate");
    register_function("set_rate", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ *m_playback_speed = kinski::string_to<float>(rpc_args.front()); }
    });

    remote_control().add_command("rate", [this](net::tcp_connection_ptr con,
                                                const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ *m_playback_speed = kinski::string_to<float>(rpc_args.front()); }
        con->write(to_string(m_media->rate()));
    });

    remote_control().add_command("seek_to_time");
    register_function("seek_to_time", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty())
        {
            float secs = 0.f;
            auto split_list = split(rpc_args.front(), ':');
            std::vector<std::string> splits(split_list.begin(), split_list.end());

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
            sync_media_to_timestamp(secs);
        }
    });

    remote_control().add_command("current_time", [this](net::tcp_connection_ptr con,
                                                        const std::vector<std::string> &rpc_args)
    {
        con->write(to_string(m_media->current_time(), 1));
    });

    remote_control().add_command("duration", [this](net::tcp_connection_ptr con,
                                                    const std::vector<std::string> &rpc_args)
    {
        con->write(to_string(m_media->duration(), 1));
    });

    remote_control().add_command("set_loop");
    register_function("set_loop", [this](const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ *m_loop = kinski::string_to<bool>(rpc_args.front()); }
    });

    remote_control().add_command("loop", [this](net::tcp_connection_ptr con,
                                                const std::vector<std::string> &rpc_args)
    {
        if(!rpc_args.empty()){ *m_loop = kinski::string_to<bool>(rpc_args.front()); }
        con->write(to_string(m_media->loop()));
    });

    remote_control().add_command("is_playing", [this](net::tcp_connection_ptr con,
                                                      const std::vector<std::string> &rpc_args)
    {
        con->write(to_string(m_media->is_playing()));
    });
}

void LED_GrabberApp::set_runmode(RunMode the_mode)
{
    m_runmode = the_mode;
    
    if(the_mode == MODE_MANUAL_CALIBRATION)
    {
        m_last_calib_click = gl::vec2(-1);
        size_t num_leds = m_led_res->value().x * m_led_res->value().y;
        
        if(m_calibration_points->value().size() != num_leds)
        {
            m_calibration_points->value().resize(num_leds, gl::vec2(-1));
        }
    }
}

void LED_GrabberApp::process_calib_click(const gl::vec2 &the_click_pos)
{
    if(m_last_calib_click != gl::vec2(-1))
    {
        // generate points via lerp
        size_t num_points = m_led_res->value().x;
        auto &calib_points = m_calibration_points->value();
        auto diff = (the_click_pos / gl::window_dimension()) - m_last_calib_click;
        
        for(size_t i = 0; i < num_points; ++i)
        {
            float frac = i / (float) num_points;
            calib_points[m_current_calib_segment * m_led_res->value().x + i] =
                m_last_calib_click + frac * diff;
        }
        m_last_calib_click = gl::vec2(-1);
        m_calibration_points->notify_observers();
        m_current_calib_segment = (m_current_calib_segment + 1) % (int) m_led_res->value().y;
    }
    else
    {
        m_last_calib_click = the_click_pos / gl::window_dimension();
    }
}
