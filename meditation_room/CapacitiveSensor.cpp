//
//  CapacitveSensor.cpp
//  kinskiGL
//
//  Created by Fabian on 26/01/16.
//
//

#include "CapacitiveSensor.hpp"

#define NUM_SENSOR_PADS 12
#define SERIAL_START_CODE 0x7E
#define SERIAL_END_CODE 0xE7

#define TIMEOUT_RECONNECT 5.f

namespace kinski
{
    CapacitiveSensor::CapacitiveSensor(const std::string &dev_name):
    m_device_name(dev_name)
    {
        if(!connect(dev_name))
        {
            LOG_ERROR << "unable to connect capacitve touch sensor";
        }
        
        m_sensor_read_buf.resize(2048);
    }
    
    void CapacitiveSensor::update(float time_delta)
    {
        // init with unchanged status
        uint16_t current_touches = m_touch_status;
        
        if(m_sensor_device.isInitialized())
        {
            size_t num_bytes = sizeof(m_touch_status);
            
            size_t bytes_to_read = std::min(m_sensor_device.available(), m_sensor_read_buf.size());
            
            if(!bytes_to_read)
            {
                m_last_reading += time_delta;
                
                if(m_last_reading > TIMEOUT_RECONNECT)
                {
                    LOG_WARNING << "no response from sensor: trying reconnect";
                    m_last_reading = 0.f;
                    connect();
                    return;
                }
            }
            else{ m_last_reading = 0.f; }
            
            uint8_t *buf_ptr = &m_sensor_read_buf[0];
            m_sensor_device.readBytes(&m_sensor_read_buf[0], bytes_to_read);
            bool reading_complete = false;
            
            for(uint32_t i = 0; i < bytes_to_read; i++)
            {
                uint8_t byte = *buf_ptr++;
                
                switch(byte)
                {
                    case SERIAL_END_CODE:
                        if(m_sensor_accumulator.size() >= num_bytes)
                        {
                            memcpy(&current_touches, &m_sensor_accumulator[0], num_bytes);
                            m_sensor_accumulator.clear();
                            reading_complete = true;
                        }
                        else{ m_sensor_accumulator.push_back(byte); }
                        break;
                        
                    case SERIAL_START_CODE:
                        if(m_sensor_accumulator.empty()){ break; }
                        
                    default:
                        m_sensor_accumulator.push_back(byte);
                        break;
                }
            }
        }
        
        for (int i = 0; i < NUM_SENSOR_PADS; i++)
        {
            uint16_t mask = 1 << i;
            
            // pad is currently being touched
            if(mask & current_touches && !(mask & m_touch_status))
            {
                if(m_touch_callback){ m_touch_callback(i); }
            }
            else if(mask & m_touch_status && !(mask & current_touches))
            {
                if(m_release_callback){ m_release_callback(i); }
            }
        }
        m_touch_status = current_touches;
    }
    
    bool CapacitiveSensor::is_touched(int the_index)
    {
        if(the_index < 0 || the_index >= NUM_SENSOR_PADS){ return m_touch_status; }
        uint16_t mask = 1 << the_index;
        return m_touch_status & mask;
    }
    
    bool CapacitiveSensor::connect(const std::string &dev_name)
    {
        if(m_device_name.empty()){ m_sensor_device.setup(0, 57600); }
        else{ m_sensor_device.setup(m_device_name, 57600); }
        
        // finally flush the newly initialized device
        if(m_sensor_device.isInitialized())
        {
            m_sensor_device.flush();
            m_last_reading = 0.f;
            return true;
        }
        return false;
    }
}