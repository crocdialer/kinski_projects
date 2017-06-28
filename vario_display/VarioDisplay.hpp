// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  VarioDisplay.hpp
//
//  Created by Fabian on 29/01/14.

#pragma once

#include "app/ViewerApp.hpp"

namespace kinski
{
    class VarioDisplay : public ViewerApp
    {
    private:
        gl::MeshPtr m_proto_lines, m_proto_triangles, m_cursor_mesh;
        std::vector<gl::MeshPtr> m_digits_lines, m_digits_triangles;
        int m_current_index = 0;
        
        gl::MeshPtr create_cursor();
        gl::MeshPtr create_proto();
        gl::MeshPtr create_proto_triangles(float line_width);
        bool set_display(gl::MeshPtr the_vario_mesh, int the_value);
        bool set_display_triangles(gl::MeshPtr the_vario_mesh, int the_value);
        
        using VarioMap = std::map<int, std::list<int>>;
        void setup_vario_map();
        VarioMap m_vario_map;
        
    public:
        VarioDisplay(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
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
        void update_property(const Property::ConstPtr &theProperty) override;
    };
}// namespace kinski

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::VarioDisplay>(argc, argv);
    LOG_INFO<<"local ip: " << kinski::net::local_ip();
    return theApp->run();
}
