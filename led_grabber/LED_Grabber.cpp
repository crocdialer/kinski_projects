//
//  LED_Grabber.cpp
//  led_grabber
//
//  Created by Fabian on 14.11.17.
//

#include <array>
#include <mutex>
#include <thread>
#include "LED_Grabber.hpp"

#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"

#define DEVICE_ID "LEDS"

namespace kinski
{

namespace
{
    // WBRG byte-order in debug SK6812-RGBW strip
    constexpr uint8_t dst_offset_r = 1, dst_offset_g = 0, dst_offset_b = 2, dst_offset_w = 3;
}
    
gl::vec2 find_dot(const cv::Mat &the_frame, float the_thresh, const cv::Mat &the_mask = cv::Mat())
{
    cv::UMat gray, thresh;
    cv::cvtColor(the_frame, gray, cv::COLOR_RGB2GRAY);
    cv::blur(gray, gray, cv::Size(5, 5));
    
    cv::threshold(gray, thresh, the_thresh, 255, cv::THRESH_TOZERO);
    auto kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)).getUMat(cv::ACCESS_READ);
    cv::morphologyEx(thresh, thresh, cv::MORPH_OPEN, kernel);
    
    cv::Moments mom = cv::moments(thresh, false);
    return gl::vec2(mom.m10 / mom.m00, mom.m01 / mom.m00);
    
//    float weight_sum = 0.0;
//    gl::vec2 center(0);
//
//    for(size_t y = 0; y < gray.rows; ++y)
//    {
//        uint8_t* line_ptr = gray.ptr<uint8_t>(y);
//        uint8_t* thresh_ptr = thresh.ptr<uint8_t>(y);
//
//        for(size_t x = 0; x < gray.cols; ++x)
//        {
//            if(thresh_ptr[x])
//            {
//                float weight = line_ptr[x] / 255.f;
//                weight_sum += weight;
//                center += weight * gl::vec2(x, y);
//            }
//        }
//    }
//    return center / weight_sum;
}
    
std::array<uint8_t, 256> create_gamma_lut(float the_brighntess, float the_gamma,
                                          uint8_t the_max_out = 255)
{
    std::array<uint8_t, 256> ret;
    
    for(int i = 0; i < 256; ++i)
    {
        ret[i] = roundf(the_max_out * the_brighntess * pow((float)i / 255, the_gamma));
    }
    return ret;
}
    
struct LED_GrabberImpl
{
//    ConnectionPtr m_connection;
    std::mutex m_connection_mutex;
    std::set<ConnectionPtr> m_connections;
    std::string m_device_name;
    
    gl::vec4 m_brightness;
    gl::ivec2 m_resolution;
    ImagePtr m_buffer_img;
    bool m_dirty_lut = true;
    
    std::vector<gl::vec2> m_calibration_points;
    glm::mat4 m_warp_matrix;
    
    std::array<uint8_t, 256> m_gamma_r, m_gamma_g, m_gamma_b, m_gamma_w;
    
    void create_lut()
    {
        m_gamma_r = create_gamma_lut(m_brightness.r, 2.6f);
        m_gamma_g = create_gamma_lut(m_brightness.g, 2.2f);
        m_gamma_b = create_gamma_lut(m_brightness.b, 2.2f);
        m_gamma_w = create_gamma_lut(m_brightness.w, 3.2f);
        m_dirty_lut = false;
    }
};

LED_Grabber::LED_Grabber():
m_impl(std::make_unique<LED_GrabberImpl>())
{
    set_resolution(58, 6);
    set_brightness(gl::vec4(0.4f, 0.4f, 0.4f, 0.2f));
}
    
LED_Grabber::~LED_Grabber()
{
    
}
    
LED_GrabberPtr LED_Grabber::create(ConnectionPtr the_device)
{
    LED_GrabberPtr ret(new LED_Grabber());
    if(the_device){ ret->connect(the_device); }
    return ret;
}
    
const std::string LED_Grabber::id()
{
    return DEVICE_ID;
}
    
bool LED_Grabber::connect(ConnectionPtr the_device)
{
    if(the_device && the_device->is_open())
    {
        std::unique_lock<std::mutex> lock(m_impl->m_connection_mutex);
        the_device->drain();
        m_impl->m_connections.insert(the_device);
        return true;
    }
    return false;
}
    
const std::set<ConnectionPtr>& LED_Grabber::connections() const
{
    return m_impl->m_connections;
}
    
bool LED_Grabber::is_initialized() const
{
    return !m_impl->m_connections.empty();
}

bool LED_Grabber::grab_from_image(const ImagePtr &the_image)
{
    if(is_initialized())
    {
        size_t num_segs = m_impl->m_resolution.y * m_impl->m_connections.size();
        size_t num_leds_per_seg = m_impl->m_resolution.x;
        
        uint8_t src_offset_r, src_offset_g, src_offset_b;

        // get channel offsets
        the_image->offsets(&src_offset_r, &src_offset_g, &src_offset_b);

        auto resized_img = the_image->resize(3 * num_leds_per_seg, 3 * num_segs, 3);
        resized_img = resized_img->blur();
        resized_img = resized_img->resize(num_leds_per_seg, num_segs, 4);
        
        if(!resized_img->data){ return false; }
        
        uint8_t *ptr = resized_img->data;
        uint8_t *end_ptr = resized_img->data + resized_img->num_bytes();
        
        // create/update lookup tables, if necessary
        if(m_impl->m_dirty_lut){ m_impl->create_lut(); }
        
        // swizzle components, apply brightness and gamma
        for(; ptr < end_ptr; ptr += 4)
        {
            uint32_t c = *(reinterpret_cast<uint32_t*>(ptr));
            uint8_t *ch = reinterpret_cast<uint8_t*>(&c);

            ptr[dst_offset_r] = m_impl->m_gamma_r[ch[src_offset_r]];
            ptr[dst_offset_g] = m_impl->m_gamma_g[ch[src_offset_g]];
            ptr[dst_offset_b] = m_impl->m_gamma_b[ch[src_offset_b]];

            // Y′ = 0.299 R′ + 0.587 G′ + 0.114 B′
            ptr[dst_offset_w] = m_impl->m_gamma_w[(uint8_t)(ch[2] * 0.299f + ch[1] * 0.587f + ch[0] * 0.114f)];
        }
        
        // reverse every second line (using a zigzag wiring-layout)
        for(uint32_t i = 0; i < resized_img->height; ++i)
        {
            if(i % 2)
            {
                uint32_t* line_ptr = ((uint32_t*)resized_img->data) + i * resized_img->width;
                std::reverse(line_ptr, line_ptr + resized_img->width);
            }
        }

        send_data(resized_img->data, resized_img->num_bytes());

        m_impl->m_buffer_img = resized_img;
        return true;
    }
    return false;
}

bool LED_Grabber::grab_from_image_calib(const ImagePtr &the_image)
{
    if(is_initialized())
    {
        // create/update lookup tables, if necessary
        if(m_impl->m_dirty_lut){ m_impl->create_lut(); }
        
        std::vector<uint32_t> led_data(m_impl->m_calibration_points.size());
        
        // get channel offsets
        uint8_t src_offset_r, src_offset_g, src_offset_b;
        the_image->offsets(&src_offset_r, &src_offset_g, &src_offset_b);
        
        // for all calibration points, use matrix to project them.
        // then sample input image at location
        // gamma correction etc.
        auto m = m_impl->m_warp_matrix;
        const auto points = m_impl->m_calibration_points;
        
        for(size_t i = 0; i < points.size(); ++i)
        {
            const auto &cp = points[i];
            gl::vec4 loc_norm = m * gl::vec4(cp.x, 1 - cp.y, 0, 1);
            loc_norm /= loc_norm.w;
            int loc_x = std::round((the_image->width - 1) * loc_norm.x);
            int loc_y = std::round((the_image->height - 1) * (1 - loc_norm.y));
            
            if(loc_x > 0 && loc_x < the_image->width &&
               loc_y > 0 && loc_y < the_image->height)
            {
                // sample from image
                uint32_t c = *(reinterpret_cast<uint32_t*>(the_image->at(loc_x, loc_y)));
                uint8_t *ch = reinterpret_cast<uint8_t*>(&c);
                
                uint8_t *out_ptr = (uint8_t*)(led_data.data() + i);
                
                out_ptr[dst_offset_r] = m_impl->m_gamma_r[ch[src_offset_r]];
                out_ptr[dst_offset_g] = m_impl->m_gamma_g[ch[src_offset_g]];
                out_ptr[dst_offset_b] = m_impl->m_gamma_b[ch[src_offset_b]];
                
                // Y′ = 0.299 R′ + 0.587 G′ + 0.114 B′
                out_ptr[dst_offset_w] = m_impl->m_gamma_w[(uint8_t)(ch[2] * 0.299f + ch[1] * 0.587f + ch[0] * 0.114f)];
            }
        }
        
        send_data((uint8_t*)led_data.data(), led_data.size() * sizeof(uint32_t));
        return true;
    }
    return false;
}
    
gl::Texture LED_Grabber::output_texture()
{
    auto ret = gl::create_texture_from_image(m_impl->m_buffer_img);
    ret.set_mag_filter(GL_NEAREST);
//    ret.set_swizzle()
    return ret;
}
    
gl::vec4 LED_Grabber::brightness() const
{
    return m_impl->m_brightness;
}

void LED_Grabber::set_brightness(const gl::vec4 &the_brightness)
{
    m_impl->m_brightness = glm::clamp(the_brightness, glm::vec4(0.f), glm::vec4(1.f));
    m_impl->m_dirty_lut = true;
}
    
void LED_Grabber::set_brightness(float the_brightness)
{
    set_brightness(gl::vec4(the_brightness));
}

gl::ivec2 LED_Grabber::resolution() const
{
    return m_impl->m_resolution;
}


void LED_Grabber::set_resolution(uint32_t the_width, uint32_t the_height)
{
    m_impl->m_resolution = gl::ivec2(the_width, the_height);
}

void LED_Grabber::send_data(const std::vector<uint8_t> &the_data) const
{

}

void LED_Grabber::send_data(const uint8_t *the_data, size_t the_num_bytes) const
{
    size_t max_bytes = 4 * m_impl->m_resolution.x * m_impl->m_resolution.y;
    std::unique_lock<std::mutex> lock(m_impl->m_connection_mutex);
    size_t total_bytes_written = 0;

    std::list<ConnectionPtr> cons(m_impl->m_connections.begin(), m_impl->m_connections.end());
    cons.sort([](const ConnectionPtr &lhs, const ConnectionPtr &rhs)
    {
        return lhs->description() < rhs->description();
    });

    for(auto &c : cons)
    {
        size_t bytes_to_write = std::min(max_bytes, the_num_bytes);
        if(!bytes_to_write){ return; }
        the_num_bytes -= bytes_to_write;

        c->write("DATA:" + to_string(bytes_to_write) + "\n");

        while(bytes_to_write)
        {
            size_t num_bytes_tranferred = c->write_bytes(the_data + total_bytes_written,
                                                         bytes_to_write);
            bytes_to_write -= num_bytes_tranferred;
            total_bytes_written += num_bytes_tranferred;
        }
    }
}
    
std::vector<gl::vec2> LED_Grabber::run_calibration()
{
    std::vector<uint32_t> calib_data(58 * 7, 0);
    
    cv::Mat frame;
    cv::VideoCapture cap;
    cap.open(0);
    std::vector<gl::vec2> points(calib_data.size());
    
    for(size_t j = 0; j < calib_data.size(); ++j)
    {
        calib_data[j] = std::numeric_limits<uint32_t >::max();
        
        // send data
        send_data((uint8_t*)calib_data.data(), calib_data.size() * sizeof(uint32_t));
        
        calib_data[j] = 0;
        
        // wait for LEDs to update
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        
        // grab frame and find location of dot
        if(cap.read(frame) && !frame.empty())
        {
//            constexpr size_t num_samples = 3;
            
            //TODO: mask input with quad
            auto p = find_dot(frame, 245.f);
            LOG_DEBUG << "got frame: " << j << " --> " << glm::to_string(p);
            points[j] = p / gl::vec2(frame.cols, frame.rows);
        }
    }
    cap.release();
    return points;
}
    
void LED_Grabber::set_calibration_points(const std::vector<gl::vec2> &the_points)
{
    m_impl->m_calibration_points = the_points;
}
    
void LED_Grabber::set_warp_matrix(const glm::mat4 &the_matrix)
{
    m_impl->m_warp_matrix = the_matrix;
}

}// namespace
