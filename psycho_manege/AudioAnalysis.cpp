//
//  AudioAnalysis.cpp
//  kinskiGL
//
//  Created by Fabian on 28/08/15.
//
//

#include "PsychoManage.h"
//#include "fmod/Fmod_Sound.h"

using namespace std;
using namespace kinski;

float PsychoManage::get_volume_for_subspectrum(float from_freq, float to_freq)
{
    //TODO no hardcoded frequency here
    float sampling_rate = 44100.f;
    
    from_freq = std::max(0.f, from_freq);
    to_freq = std::min(sampling_rate / 2.f, to_freq);
    
    float fraction_from = from_freq / (sampling_rate / 2.f);
    float fraction_to = to_freq / (sampling_rate / 2.f);
    int from_bin = (int)(m_sound_values.size() * fraction_from);
    int to_bin = (int)(m_sound_values.size() * fraction_to);
    
    if(to_bin - from_bin <= 0){ return 0.f; }
    
    // sum values
    float sum = 0;
    for(int i = from_bin; i < to_bin; i++){ sum += m_sound_values[i].back(); }
    
    // return arithmetic average
    return sum / (to_bin - from_bin);
}

float PsychoManage::get_volume_for_freq_band(FrequencyBand bnd)
{
    auto iter = m_frequency_map.find(bnd);
    if(iter == m_frequency_map.end()) return 0.f;
    
    auto pair = iter->second;
    return get_volume_for_subspectrum(pair.first, pair.second);
}

void PsychoManage::init_audio()
{
    auto rec_devices = audio::get_recording_devices();
    
    // print audio input devices
    for(audio::device rec_dev : rec_devices){ LOG_INFO << rec_dev.name; }
    
    float inc_speed = .9f, dec_speed = .75f;
    
    // audio recording for fft
    m_sound_recording = std::make_shared<audio::Fmod_Sound>();
    m_sound_recording->set_loop(true);
    m_sound_recording->set_volume(0);
    m_sound_recording->record(.05);
    m_sound_recording->play();
    
    // smoothness filter mechanism
    int num_freq_bands = 256;
    m_sound_spectrum.resize(num_freq_bands);
    m_sound_values.resize(num_freq_bands);
    
//    for(auto &measure : m_sound_values)
//    {
//        auto filter = std::make_shared<FalloffFilter<float>>();
//        filter->set_increase_speed(inc_speed);
//        filter->set_decrease_speed(dec_speed);
//        measure.set_filter(filter);
//    }
}

void PsychoManage::update_audio()
{
    // audio analysis
    if(m_sound_recording)
    {
        audio::update();
        m_sound_recording->get_spectrum(m_sound_spectrum, m_sound_spectrum.size());
        
        for(int i = 0; i < m_sound_spectrum.size(); i++)
        {
            m_sound_values[i].push_back(m_sound_spectrum[i]);
        }
        
        std::vector<FrequencyBand> freq_bands = {FREQ_LOW, FREQ_MID_LOW, FREQ_MID_HIGH, FREQ_HIGH};
        m_last_volumes.resize(4);
        m_last_volume = 0;
        for(size_t i = 0; i < freq_bands.size(); i++)
        {
            m_last_volumes[i] = get_volume_for_freq_band(freq_bands[i]);
            m_last_volume += m_last_volumes[i];
        }
        m_last_volume /= m_last_volumes.size();
    }
}
