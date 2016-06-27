//
//  Ballenberg.h
//  gl
//
//  Created by Fabian on 16/03/16.
//
//
#pragma once

#include "app/ViewerApp.hpp"
#include "gl_post_process/WarpComponent.hpp"
#include "core/Timer.hpp"
#include "core/Serial.hpp"

#include "sensors/DistanceSensor.hpp"

// modules
#include "media/media.h"
#include "sensors/CapacitiveSensor.hpp"
#include "dmx/dmx.h"

namespace kinski
{
    class Ballenberg : public ViewerApp
    {
    private:
        
        enum class State{IDLE};
        std::map<State, std::string> m_state_string_map =
        {
            {State::IDLE, "Idle"}
        };
        
        State m_current_state = State::IDLE;
        Timer m_timer_idle, m_timer_lamp_noise, m_timer_movie_kitchen, m_timer_movie_living_room,
            m_timer_motor_spense;
        
        std::vector<Timer> m_motor_move_timers;
        
        Property_<float>::Ptr
        m_timeout_idle = Property_<float>::create("timeout idle", 30.f),
        m_timeout_motor_01 = Property_<float>::create("timeout motor (spense)", 30.f),
        m_duration_lamp_noise = Property_<float>::create("duration for lamp noise", 10.f);
        
        Property_<string>::Ptr
        m_asset_base_dir = Property_<string>::create("asset base directory", "~/ballenberg_assets"),
        m_cap_sense_dev_name = Property_<string>::create("touch sensor device"),
        m_dmx_dev_name = Property_<string>::create("dmx device"),
        m_motion_sense_dev_name_01 = Property_<string>::create("motion sensor (kitchen)"),
        m_motion_sense_dev_name_02 = Property_<string>::create("motion sensor (living room)"),
        m_motion_sense_dev_name_03 = Property_<string>::create("motion sensor (spense)"),
        m_motor_dev_name_01 = Property_<string>::create("motor device (spense)"),
        m_ip_kitchen = Property_<string>::create("ip-adress kitchen", "192.168.69.101"),
        m_ip_living_room = Property_<string>::create("ip-adress living room", "192.168.69.102");
        
        Property_<uint32_t>::Ptr
        m_cap_sense_thresh_touch = Property_<uint32_t>::create("capsense: touch threshold", 20),
        m_cap_sense_thresh_release = Property_<uint32_t>::create("capsense: release threshold", 16),
        m_cap_sense_charge_current = Property_<uint32_t>::create("capsense: charge current", 63);
        
        Property_<float>::Ptr
        m_cap_sense_proxi_multiplier = RangedProperty<float>::create("capsense: proximity multiplier",
                                                                     1.f , 0.f, 100.f);
        
        Property_<gl::Color>::Ptr
        m_spot_color = Property_<gl::Color>::create("spot color", gl::COLOR_BLACK);
        
        DistanceSensor m_motion_sense_01, m_motion_sense_02, m_motion_sense_03;
        
        DMXController m_dmx;
        
        //! servo-motor control, send integers [0 ... 180]
        UART_Ptr m_motor_uart;
        
        Property_<float>::Ptr
        m_volume = Property_<float>::create("volume", 1.f);
        
        CapacitiveSensor m_cap_sense;
        float m_proxi_val = 0.f;
        
        bool change_state(State the_the_state, bool force_change = false);
        
        //! display sensor and application state
        void draw_status_info();
        
        bool load_assets();
        
        void motor_move(int the_degree);
        
    public:
        
        Ballenberg(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
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
    };
}// namespace kinski