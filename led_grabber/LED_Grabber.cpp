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

inline uint32_t color_to_uint(const gl::Color &the_color)
{
    return  static_cast<uint32_t>(0xFF * the_color.r) << (dst_offset_r * 8) |
            static_cast<uint32_t>(0xFF * the_color.g) << (dst_offset_g * 8) |
            static_cast<uint32_t>(0xFF * the_color.b) << (dst_offset_b * 8) |
            static_cast<uint32_t>(0xFF * the_color.a) << (dst_offset_w * 8);
}
    
gl::vec2 find_dot(const cv::Mat &the_frame, float the_thresh, const cv::Mat &the_mask = cv::Mat())
{
    cv::UMat gray, thresh;
    the_frame.copyTo(gray, the_mask);
    cv::cvtColor(gray, gray, cv::COLOR_RGB2GRAY);
    cv::blur(gray, gray, cv::Size(5, 5));
    cv::threshold(gray, thresh, the_thresh, 255, cv::THRESH_TOZERO);
    auto kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)).getUMat(cv::ACCESS_READ);
    cv::morphologyEx(thresh, thresh, cv::MORPH_OPEN, kernel);
    cv::Moments mom = cv::moments(thresh, false);
    return gl::vec2(mom.m10 / mom.m00, mom.m01 / mom.m00);
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
    std::mutex m_connection_mutex;
    std::set<ConnectionPtr> m_connections;
    std::string m_device_name;
    
    gl::vec4 m_brightness;
    gl::ivec2 m_resolution, m_unit_resolution;
    ImagePtr m_buffer_img;
    bool m_dirty_lut = true;
    
    std::vector<gl::vec2> m_calibration_points;
    gl::Warp m_warp;
    
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
    set_unit_resolution(58, 7);
    set_brightness(gl::vec4(0.4f, 0.4f, 0.4f, 0.2f));
}
    
LED_Grabber::~LED_Grabber()
{
    
}
    
LED_GrabberPtr LED_Grabber::create(ConnectionPtr the_con)
{
    LED_GrabberPtr ret(new LED_Grabber());
    if(the_con){ ret->connect(the_con); }
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

bool LED_Grabber::grab_from_image_calib(const ImagePtr &the_image)
{
    auto cast_img = std::dynamic_pointer_cast<Image_<uint8_t >>(the_image);

    if(is_initialized() && cast_img)
    {
        // create/update lookup tables, if necessary
        if(m_impl->m_dirty_lut){ m_impl->create_lut(); }
        
        std::vector<uint32_t> led_data(m_impl->m_calibration_points.size(), 0);
        
        // get channel offsets
        uint8_t src_offset_r, src_offset_g, src_offset_b;
        the_image->offsets(&src_offset_r, &src_offset_g, &src_offset_b);
        
        // for all calibration points, use matrix to project them.
        // then sample input image at location
        // gamma correction etc.
        auto m = m_impl->m_warp.inv_transform();
        const auto points = m_impl->m_calibration_points;

        int w = the_image->width();
        int h = the_image->height();

        for(size_t i = 0; i < points.size(); ++i)
        {
            const auto &cp = points[i];
            gl::vec4 loc_norm = m * gl::vec4(cp.x, 1 - cp.y, 0, 1);
            loc_norm /= loc_norm.w;
            int loc_x = std::round((w - 1) * loc_norm.x);
            int loc_y = std::round((h - 1) * (1 - loc_norm.y));
            
            if(loc_x > 0 && loc_x < w &&
               loc_y > 0 && loc_y < h)
            {
                // sample from image
                uint8_t *ch = cast_img->at(loc_x, loc_y);
                uint8_t *out_ptr = (uint8_t*)(led_data.data() + i);
                
                out_ptr[dst_offset_r] = m_impl->m_gamma_r[ch[src_offset_r]];
                out_ptr[dst_offset_g] = m_impl->m_gamma_g[ch[src_offset_g]];
                out_ptr[dst_offset_b] = m_impl->m_gamma_b[ch[src_offset_b]];
                
                // Y′ = 0.299 R′ + 0.587 G′ + 0.114 B′
                out_ptr[dst_offset_w] = m_impl->m_gamma_w[(uint8_t)(ch[src_offset_r] * 0.299f +
                                                                    ch[src_offset_g] * 0.587f +
                                                                    ch[src_offset_b] * 0.114f)];
            }
        }
        send_data((uint8_t*)led_data.data(), led_data.size() * sizeof(uint32_t));
        return true;
    }
    return false;
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

gl::ivec2 LED_Grabber::unit_resolution() const
{
    return m_impl->m_unit_resolution;
}
    
void LED_Grabber::set_unit_resolution(uint32_t the_width, uint32_t the_height)
{
    m_impl->m_unit_resolution = gl::ivec2(the_width, the_height);
}
    
void LED_Grabber::send_data(const std::vector<uint8_t> &the_data) const
{
    send_data(the_data.data(), the_data.size());
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

void LED_Grabber::show_segment(size_t the_segment, int the_mark_width) const
{
    std::vector<uint32_t> led_data(m_impl->m_resolution.x * m_impl->m_resolution.y, 0);
    
    // first, last index of segment
    size_t first = the_segment * m_impl->m_resolution.x, last = first + m_impl->m_resolution.x - 1;
    gl::Color color_first = gl::COLOR_GREEN, color_last = gl::COLOR_RED;
    color_first.a = color_last.a = 0;
    
    int width = std::min(the_mark_width, m_impl->m_resolution.x - 1);
    
    for(int w = 0; w < width; w++)
    {
        led_data[first + w] = color_to_uint(color_first);
        led_data[last - w] = color_to_uint(color_last);
    }
    
    // send m_data
    send_data((uint8_t*)led_data.data(), led_data.size() * sizeof(uint32_t));
}
    
std::vector<gl::vec2> LED_Grabber::run_calibration(int the_cam_index,
                                                   int the_thresh,
                                                   const gl::Color &the_calib_color)
{
    std::vector<uint32_t> calib_data(m_impl->m_resolution.x * m_impl->m_resolution.y, 0);
    std::vector<gl::vec2> points(calib_data.size());

    cv::Mat frame, mask;
    cv::VideoCapture cap;
    cap.open(the_cam_index);
    size_t num_calib_errors = 0;

    for(size_t j = 0; j < calib_data.size(); ++j)
    {
        calib_data[j] = color_to_uint(the_calib_color);

        // send m_data
        send_data((uint8_t*)calib_data.data(), calib_data.size() * sizeof(uint32_t));

        calib_data[j] = 0;

        // wait for LEDs to update
        std::this_thread::sleep_for(std::chrono::milliseconds(25));

        // grab frame and find location of dot
        if(cap.read(frame) && !frame.empty())
        {
            // mask input with quadcorners
            if(mask.cols != frame.cols || mask.rows != frame.rows)
            {
                mask = cv::Mat(frame.rows, frame.cols, CV_8UC1);

                auto corners = m_impl->m_warp.corners();

                for(auto &c : corners)
                { c = gl::vec2(c.x, 1 - c.y) * gl::vec2(frame.cols - 1, frame.rows - 1); }

                std::vector<cv::Point> points = {
                    {(int)corners[0].x, (int)corners[0].y},
                    {(int)corners[1].x, (int)corners[1].y},
                    {(int)corners[3].x, (int)corners[3].y},
                    {(int)corners[2].x, (int)corners[2].y}
                };
                cv::fillConvexPoly(mask, points, 255);
            }

            auto p = find_dot(frame, the_thresh, mask);
            if(!std::isnan(p.x) && !std::isnan(p.y))
            {
                LOG_TRACE << "got frame: " << j << " --> " << glm::to_string(p);
                points[j] = p / gl::vec2(frame.cols, frame.rows);
            }else
            {
                LOG_DEBUG << "invalid calibration point: " << num_calib_errors++;
                points[j] = gl::vec2(-1);
            }
        }
    }
    cap.release();
    return points;
}

const std::vector<gl::vec2>& LED_Grabber::calibration_points() const
{
    return m_impl->m_calibration_points;
}
    
void LED_Grabber::set_calibration_points(const std::vector<gl::vec2> &the_points)
{
    m_impl->m_calibration_points = the_points;
}
    
    void LED_Grabber::set_warp(const gl::Warp &the_warp)
{
    m_impl->m_warp = the_warp;
}

}// namespace
