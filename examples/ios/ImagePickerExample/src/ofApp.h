#pragma once

#include "ofxiOS.h"
#include "ofxiOSImagePicker.h"

class ofApp : public ofxiOSApp{
	
    public:
        void setup();
        void update();
        void draw();
        void exit();
	
        void touchDown(ofTouchEventArgs & touch);
        void touchMoved(ofTouchEventArgs & touch);
        void touchUp(ofTouchEventArgs & touch);
        void touchDoubleTap(ofTouchEventArgs & touch);
        void touchCancelled(ofTouchEventArgs & touch);
	
        void lostFocus();
        void gotFocus();
        void gotMemoryWarning();
        void deviceOrientationChanged(int newOrientation);
	
        ofxiOSImagePicker camera;
        ofImage	photo;
        glm::vec3 imgPos;
        glm::vec3 prePoint;
	
        ofImage camImg;
        ofImage libImg;
    
        ofRectangle camRect;
        ofRectangle libRect;
};

