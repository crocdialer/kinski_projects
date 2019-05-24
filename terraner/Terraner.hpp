//  Terraner.hpp
//
//  created by crocdialer on 21/05/18.

#pragma once

#include "app/ViewerApp.hpp"
#include "gl/Noise.hpp"

using namespace crocore;

namespace kinski
{
    class Terraner : public ViewerApp
    {
    private:

        enum TextureEnum{TEXTURE_HEIGHT = 0};
        gl::MeshPtr m_terrain;
        gl::Noise m_noise;

        Property_<gl::vec2>::Ptr
        m_noise_scale = Property_<gl::vec2>::create("noise scale", gl::vec2(0.05f));

        Property_<float>::Ptr
        m_displace_factor = RangedProperty<float>::create("displace factor", 10.f, -25.f, 25.f);

    public:
        Terraner(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
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
        void update_property(const PropertyConstPtr &the_property) override;
    };
}// namespace kinski

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::Terraner>(argc, argv);
    LOG_INFO<<"local ip: " << crocore::net::local_ip();
    return theApp->run();
}
