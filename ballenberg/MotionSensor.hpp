//
//  MotionSensor.hpp
//  kinskiGL
//
//  Created by Fabian on 22/03/16.
//
//

#pragma once

#include "core/core.hpp"

namespace kinski
{
    class MotionSensor
    {
    public:
        
        typedef std::function<void()> MotionCallback;
        
        MotionSensor();
        
        bool connect(const std::string &dev_name = "");
        void update(float time_delta);
        bool motion_detected() const;
        
        float timeout_reconnect() const;
        void set_timeout_reconnect(float val);
        
        void set_motion_callback(MotionCallback cb);
        bool is_initialized() const;
        
    private:
        struct MotionSensorImpl;
        struct Impl;
        std::shared_ptr<Impl> m_impl;
    };
}// namespace