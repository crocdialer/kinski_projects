//
//  SensorDebug.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#pragma once

#include <crocore/Animation.hpp>
#include "app/ViewerApp.hpp"
#include "crocore/Serial.hpp"
#include "crocore/CircularBuffer.hpp"

// module includes
#include "dmx/dmx.h"

namespace kinski
{
    class SensorDebug : public ViewerApp
    {
    private:
        
        enum FontEnum{FONT_CONSOLE = 0, FONT_MEDIUM = 1, FONT_LARGE = 2};
        
        enum AnimationEnum{ANIMATION_SCORE = 0};

        std::vector<crocore::Animation> m_animations{10};

        //////////////////////// serial IO //////////////////////////////////////
        
        ConnectionPtr
        m_serial_sensor = Serial::create(background_queue().io_service()),
        m_serial_nixie = Serial::create(background_queue().io_service());
        
        float m_last_sensor_reading = 0.f, m_sensor_timeout = 5.f;
        dmx::Controller m_dmx{background_queue().io_service()};
        
        //////////////////////// sensor input ///////////////////////////////////

        std::vector<CircularBuffer<float>> m_measurements;
        std::vector<uint8_t> m_serial_accumulator, m_serial_read_buf;
        
        float m_sensor_last_max;
        
        uint32_t m_sensor_refresh_count = 0;
        
        Timer m_timer_sensor_refresh, m_timer_game_ready, m_timer_impact_measure;
        
        //////////////////////////////////////////////////////////////////////////
        
        Property_<std::string>::Ptr
        m_device_name_sensor = Property_<std::string>::create("sensor device", ""),
        m_device_name_nixie = Property_<std::string>::create("nixie device", ""),
        m_device_name_dmx = Property_<std::string>::create("dmx device", "");
        
        Property_<int>::Ptr
        m_sensor_refresh_rate = Property_<int>::create("sensor refresh rate", 0),
        m_sensor_hist_size = RangedProperty<int>::create("sensor history size", 150, 1, 500);
        
        RangedProperty<float>::Ptr
        m_timeout_game_ready = RangedProperty<float>::create("timeout game_ready", 1.f, 0.f, 60.f),
        m_duration_impact_measure = RangedProperty<float>::create("duration impact measure", .4f, 0.f, 5.f),
        m_duration_count_score = RangedProperty<float>::create("duration count score", 3.f, 0.f, 60.f),
        m_duration_display_score = RangedProperty<float>::create("duration display score", 4.f, 0.f, 60.f),
        m_force_multiplier = RangedProperty<float>::create("force multiplier", 1.f, 0.f, 20.f);
        
        Property_<gl::vec2>::Ptr
        m_range_min_max = Property_<gl::vec2>::create("sensor range min/max", gl::vec2(0.05f, 1.f));
        
        ///////////////////////////// gameplay //////////////////////////////////
        
        enum GameState{IDLE = 0, IMPACT = 1, SCORE = 2} m_current_gamestate = IDLE;
        
        std::map<GameState, std::string> m_gamestate_names =
        {
            {IDLE, "IDLE"},
            {IMPACT, "IMPACT"},
            {SCORE, "SCORE"}
        };
        
        Property_<int>::Ptr
        m_score_min = Property_<int>::create("score min", 0),
        m_score_max = Property_<int>::create("score max", 999);
        
        float m_current_value;
        float m_display_value;
        
        gl::MeshPtr m_line_mesh;
        
        bool change_gamestate(GameState the_state);
        
        /////////////////////////////////////////////////////////////////////////
        
        void update_sensor_values(float time_delta);

        void update_score_display();
        
        void update_dmx();
        
        void process_impact(float val);
        
        // final score
        inline int final_score(float v) const
        {
            return (int)roundf(map_value<float>(v,
                                                m_range_min_max->value().x,
                                                m_range_min_max->value().y,
                                                *m_score_min,
                                                *m_score_max));
        }
        
    public:
        SensorDebug(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
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
        void update_property(const PropertyConstPtr &theProperty) override;
    };
}// namespace kinski

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::SensorDebug>(argc, argv);
    LOG_INFO<<"local ip: " << crocore::net::local_ip();
    return theApp->run();
}
