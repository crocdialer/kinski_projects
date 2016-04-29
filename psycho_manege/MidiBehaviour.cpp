//
//  MidiBehaviour.cpp
//  kinskiGL
//
//  Created by Croc Dialer on 27/08/15.
//
//

#include "ModelViewer.h"

using namespace std;
using namespace kinski;

#define NOTE_ON 144
#define NOTE_OFF 128
#define CONTROL_CHANGE 176

void ModelViewer::midi_callback(double time_stamp, const std::vector<uint8_t> &the_msg)
{
    if(the_msg.size() != 3){ return; }
//    LOG_DEBUG << "midi msg: " << the_msg.size() << " bytes";
    
    for (auto &c : the_msg) { LOG_TRACE << (uint32_t)c; }
    
    uint32_t event_type = the_msg[0], key = the_msg[1], value = the_msg[2];
    
    float min, max;
    
    if(event_type == CONTROL_CHANGE)
    {
        switch (key)
        {
                ////////////////////////////// Line Faders /////////////////////////////////////////////////
                
            case 1:
                m_rotation_lvl_1->getRange(min, max);
                *m_rotation_lvl_1 = map_value<float>(value, 0, 127, min, max);
                break;
            case 2:
                m_rotation_lvl_2->getRange(min, max);
                *m_rotation_lvl_2 = map_value<float>(value, 0, 127, min, max);
                break;
            case 3:
                m_rotation_lvl_3->getRange(min, max);
                *m_rotation_lvl_3 = map_value<float>(value, 0, 127, min, max);
                break;
            case 4:
                m_animation_speed->getRange(min, max);
                *m_animation_speed = map_value<float>(value, 0, 127, min, max);
                break;
            case 5:
                m_displace_factor->getRange(min, max);
                *m_displace_factor = map_value<float>(value, 0, 127, min, max);
                break;
            case 6:
                m_displace_res->getRange(min, max);
                *m_displace_res = map_value<float>(value, 0, 127, min, max);
                break;
            case 7:
                m_obj_scale->getRange(min, max);
                *m_obj_scale = map_value<float>(value, 0, 127, min, max);
                break;
            case 8:
                m_obj_audio_react_low->getRange(min, max);
                *m_obj_audio_react_low = map_value<float>(value, 0, 127, min, max);
                break;
            case 9:
                m_obj_audio_auto_rotate->getRange(min, max);
                *m_obj_audio_auto_rotate = map_value<float>(value, 0, 127, min, max);
                break;
            case 10:
                m_obj_audio_react_hi->getRange(min, max);
                *m_obj_audio_react_hi = map_value<float>(value, 0, 127, min, max);
                break;
            default:
                break;
        }
    }
    else if(event_type == NOTE_ON)
    {
        switch (key)
        {
            ////////////////////////////// Pads /////////////////////////////////////////////////
            
            // select animation
            case 36:
            case 37:
            case 38:
            case 39:
            case 40:
            case 41:
            case 42:
            case 43:
            {
                uint32_t idx = key - 36;
                if(auto m = selected_mesh()){ m->set_animation_index(idx); }
            }
                break;
            
            // wireframe
            case 44:
            {
                if(auto m = selected_mesh())
                {
                    for(auto &mat : m->materials())
                    {
                        mat->setWireframe(!mat->wireframe());
                    }
                }
            }
                break;
            
            // toggle selection
            case 48:
            case 49:
            {
                uint32_t inc = key - 48 ? -1 : 1;
                toggle_selection(inc);
            }
                break;
                
            default:
                break;
        }
    }
}