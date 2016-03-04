//
//  EmptySample.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#ifndef __gl__BluetoothApp__
#define __gl__BluetoothApp__

#include "app/ViewerApp.h"

// module headers
#include "bluetooth/bluetooth.h"

namespace kinski
{
    class BluetoothApp : public ViewerApp
    {
    private:
        
        gl::Font m_font;
        
        bluetooth::Central m_central;
        
    public:
        BluetoothApp(int argc = 0, char *argv[] = nullptr):ViewerApp(argc, argv){};
        void setup() override;
        void update(float timeDelta) override;
        void draw() override;
        void resize(int w ,int h) override;
        void keyPress(const KeyEvent &e) override;
        void keyRelease(const KeyEvent &e) override;
        void mousePress(const MouseEvent &e) override;
        void mouseRelease(const MouseEvent &e) override;
        void mouseMove(const MouseEvent &e) override;
        void mouseDrag(const MouseEvent &e) override;
        void mouseWheel(const MouseEvent &e) override;
        void touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches) override;
        void fileDrop(const MouseEvent &e, const std::vector<std::string> &files) override;
        void tearDown() override;
        void update_property(const Property::ConstPtr &theProperty) override;
    };
}// namespace kinski

#endif /* defined(__gl__BluetoothApp__) */
