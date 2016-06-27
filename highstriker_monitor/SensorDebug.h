//
//  SensorDebug.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#pragma once

#include "app/ViewerApp.hpp"
#include "core/Serial.hpp"
#include "core/Measurement.hpp"

// module includes
#include "dmx/dmx.h"

namespace kinski
{
    class SensorDebug : public ViewerApp
    {
    private:
        
        enum FontEnum{FONT_CONSOLE = 0, FONT_MEDIUM = 1, FONT_LARGE = 2};
        
        enum AnimationEnum{ANIMATION_SCORE = 0};
        
        //////////////////////// serial IO //////////////////////////////////////
        
        UART_Ptr
        m_serial_sensor = Serial::create(),
        m_serial_nixie = Serial::create();
        
        float m_last_sensor_reading = 0.f, m_sensor_timeout = 5.f;
        DMXController m_dmx;
        
        //////////////////////// sensor input ///////////////////////////////////
        
        std::vector<float> m_sensor_vals;
        std::vector<Measurement<float>> m_measurements;
        
        std::vector<uint8_t> m_serial_accumulator, m_serial_read_buf;
        
        float m_sensor_last_max;
        
        uint32_t m_sensor_refresh_count = 0;
        
        Timer m_timer_sensor_refresh, m_timer_game_ready, m_timer_impact_measure;
        
        //////////////////////////////////////////////////////////////////////////
        
        Property_<string>::Ptr
        m_device_name_sensor = Property_<string>::create("sensor device", ""),
        m_device_name_nixie = Property_<string>::create("nixie device", ""),
        m_device_name_dmx = Property_<string>::create("dmx device", "");
        
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
        m_score_min = Property_<int>::create("score min", 999),
        m_score_max = Property_<int>::create("score max", 0);
        
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