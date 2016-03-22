//
//  MotionSensor.cpp
//  kinskiGL
//
//  Created by Fabian on 22/03/16.
//
//

#include <cstring>
#include <thread>
#include "core/Serial.hpp"
#include "MotionSensor.hpp"

#define SERIAL_END_CODE '\n'
#define STD_TIMEOUT_RECONNECT 5.f

namespace kinski{
    
    struct MotionSensor::Impl
    {
        Serial m_sensor_device;
        std::string m_device_name;
        std::vector<uint8_t> m_sensor_read_buf, m_sensor_accumulator;
        bool m_motion_detected = false;
        float m_last_reading = 0.f;
        float m_timeout_reconnect = STD_TIMEOUT_RECONNECT;
        std::thread m_reconnect_thread;
        
        MotionCallback m_motion_callback;
    };
    
    MotionSensor::MotionSensor():
    m_impl(new Impl)
    {
        
    }
    
    bool MotionSensor::connect(const std::string &dev_name)
    {
        if(dev_name.empty()){}
        else{ m_impl->m_sensor_device.setup(dev_name, 57600); }
        
        // finally flush the newly initialized device
        if(m_impl->m_sensor_device.isInitialized())
        {
            m_impl->m_device_name = dev_name;
            m_impl->m_sensor_device.flush();
            m_impl->m_last_reading = 0.f;
            return true;
        }
        return false;
    }
    
    void MotionSensor::update(float time_delta)
    {
        size_t bytes_to_read = 0;
        m_impl->m_last_reading += time_delta;
        bool reading_complete = false, motion_detected = false;
        
        if(m_impl->m_sensor_device.isInitialized())
        {
            bytes_to_read = std::min(m_impl->m_sensor_device.available(),
                                     m_impl->m_sensor_read_buf.size());
            
            if(bytes_to_read){ m_impl->m_last_reading = 0.f; }
            
            uint8_t *buf_ptr = &m_impl->m_sensor_read_buf[0];
            m_impl->m_sensor_device.readBytes(&m_impl->m_sensor_read_buf[0], bytes_to_read);
            
            for(uint32_t i = 0; i < bytes_to_read; i++)
            {
                uint8_t byte = *buf_ptr++;
                
                switch(byte)
                {
                    case SERIAL_END_CODE:
                        motion_detected = string_as<bool>(string(m_impl->m_sensor_accumulator.begin(),
                                                                 m_impl->m_sensor_accumulator.end()));
                        m_impl->m_sensor_accumulator.clear();
                        reading_complete = true;
                        break;
                        
                    default:
                        m_impl->m_sensor_accumulator.push_back(byte);
                        break;
                }
            }
        }
        
        m_impl->m_motion_detected = motion_detected;
        
        if((m_impl->m_timeout_reconnect > 0.f) && m_impl->m_last_reading > m_impl->m_timeout_reconnect)
        {
            LOG_WARNING << "no response from sensor: trying reconnect ...";
            m_impl->m_last_reading = 0.f;
            try { if(m_impl->m_reconnect_thread.joinable()) m_impl->m_reconnect_thread.join(); }
            catch (std::exception &e) { LOG_WARNING << e.what(); }
            m_impl->m_reconnect_thread = std::thread([this](){ connect(m_impl->m_device_name); });
        }
    }
    
    void MotionSensor::set_motion_callback(MotionCallback cb)
    {
        m_impl->m_motion_callback = cb;
    }
    
    float MotionSensor::timeout_reconnect() const
    {
        return m_impl->m_timeout_reconnect;
    }
    
    void MotionSensor::set_timeout_reconnect(float val)
    {
        m_impl->m_timeout_reconnect = std::max(val, 0.f);
    }
    
    bool MotionSensor::is_initialized() const
    {
        return m_impl->m_sensor_device.isInitialized();
    }
}