//
//  CapacitveSensor.hpp
//  kinskiGL
//
//  Created by Fabian on 26/01/16.
//
//
#pragma once
#include "core/core.h"
#include "core/Serial.h"

namespace kinski
{
    class CapacitiveSensor
    {
    public:
        
        typedef std::function<void(int)> Callback;
        
        CapacitiveSensor(const std::string &dev_name = "");
        
        void update(float time_delta);
        
        //! return touch state for provided index,
        //  or "any" touch, if index is out of bounds
        bool is_touched(int the_index = -1);
        

    private:
        bool connect(const std::string &dev_name = "");
        
        Serial m_sensor_device;
        std::string m_device_name;
        std::vector<uint8_t> m_sensor_read_buf, m_sensor_accumulator;
        uint16_t m_touch_status = 0;
        float m_last_reading;
        
        Callback m_touch_callback, m_release_callback;
    };
}