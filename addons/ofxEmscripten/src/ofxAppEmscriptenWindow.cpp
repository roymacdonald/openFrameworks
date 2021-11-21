/*
 * ofAppEmscriptenWindow.cpp
 *
 *  Created on: May 8, 2014
 *      Author: arturo
 */

#include "ofxAppEmscriptenWindow.h"
#include "ofLog.h"
#include "ofEvents.h"
#include "ofGLProgrammableRenderer.h"

using namespace std;

ofxAppEmscriptenWindow * ofxAppEmscriptenWindow::instance = NULL;

// from http://cantuna.googlecode.com/svn-history/r16/trunk/src/screen.cpp
#define CASE_STR(x,y) case x: str = y; break

static const char* eglErrorString(EGLint err) {
    string str;
    switch (err) {
        CASE_STR(EGL_SUCCESS, "no error");
        CASE_STR(EGL_NOT_INITIALIZED, "EGL not, or could not be, initialized");
        CASE_STR(EGL_BAD_ACCESS, "access violation");
        CASE_STR(EGL_BAD_ALLOC, "could not allocate resources");
        CASE_STR(EGL_BAD_ATTRIBUTE, "invalid attribute");
        CASE_STR(EGL_BAD_CONTEXT, "invalid context specified");
        CASE_STR(EGL_BAD_CONFIG, "invald frame buffer configuration specified");
        CASE_STR(EGL_BAD_CURRENT_SURFACE, "current window, pbuffer or pixmap surface is no longer valid");
        CASE_STR(EGL_BAD_DISPLAY, "invalid display specified");
        CASE_STR(EGL_BAD_SURFACE, "invalid surface specified");
        CASE_STR(EGL_BAD_MATCH, "bad argument match");
        CASE_STR(EGL_BAD_PARAMETER, "invalid paramater");
        CASE_STR(EGL_BAD_NATIVE_PIXMAP, "invalid NativePixmap");
        CASE_STR(EGL_BAD_NATIVE_WINDOW, "invalid NativeWindow");
        CASE_STR(EGL_CONTEXT_LOST, "APM event caused context loss");
        default: str = "unknown error " + ofToString(err); break;
    }
    return str.c_str();
}

ofxAppEmscriptenWindow::ofxAppEmscriptenWindow(){
	instance = this;
}

ofxAppEmscriptenWindow::~ofxAppEmscriptenWindow() {
	if(context != 0){
		emscripten_webgl_destroy_context(context);	
	}
}

void ofxAppEmscriptenWindow::setup(const ofGLESWindowSettings & settings){

	EmscriptenWebGLContextAttributes attrs;
	emscripten_webgl_init_context_attributes(&attrs);


/// when setting  explicitSwapControl to 0 it is emscripten that is in charge of swapping on each render call.

	attrs.explicitSwapControl = 0;
    attrs.depth = 1;
    attrs.stencil = 1;
    attrs.antialias = 1;
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;
	attrs.alpha = 1;

	context = emscripten_webgl_create_context("#canvas", &attrs);
	assert(context);
	  
	makeCurrent();

	setWindowShape(settings.getWidth(),settings.getHeight());

	_renderer = make_shared<ofGLProgrammableRenderer>(this);
	((ofGLProgrammableRenderer*)_renderer.get())->setup(2,0);

	//for some reason the key events were not working on Macos when using #canvas
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,this,1,&keydown_cb);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,this,1,&keyup_cb);
    
    emscripten_set_mousedown_callback("#canvas",this,1,&mousedown_cb);
    emscripten_set_mouseup_callback("#canvas",this,1,&mouseup_cb);
    emscripten_set_mousemove_callback("#canvas",this,1,&mousemoved_cb);

    emscripten_set_touchstart_callback("#canvas",this,1,&touch_cb);
    emscripten_set_touchend_callback("#canvas",this,1,&touch_cb);
    emscripten_set_touchmove_callback("#canvas",this,1,&touch_cb);
    emscripten_set_touchcancel_callback("#canvas",this,1,&touch_cb);

}

void ofxAppEmscriptenWindow::loop(){

	instance->events().notifySetup();


	// Emulate loop via callbacks
	emscripten_set_main_loop( display_cb, -1, 1 );
}

void ofxAppEmscriptenWindow::update(){
	events().notifyUpdate();
}

void ofxAppEmscriptenWindow::draw(){
	// set viewport, clear the screen
	renderer()->startRender();
	if( bEnableSetupScreen ) renderer()->setupScreen();

	events().notifyDraw();

	renderer()->finishRender();
}

void ofxAppEmscriptenWindow::display_cb(){
	if(instance){
		instance->update();
		instance->draw();
	}
}

int ofxAppEmscriptenWindow::keydown_cb(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData){
	int key = keyEvent->key[0];
	if(key==0){
		key = keyEvent->which + 32;
	}
	instance->events().notifyKeyPressed(key);
	return 0;
}

int ofxAppEmscriptenWindow::keyup_cb(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData){
	int key = keyEvent->key[0];
	if(key==0){
		key = keyEvent->which + 32;
	}
	instance->events().notifyKeyReleased(key);
	return 0;
}

int ofxAppEmscriptenWindow::mousedown_cb(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData){
	instance->events().notifyMousePressed(ofGetMouseX(),ofGetMouseY(),mouseEvent->button);
	return 0;
}

int ofxAppEmscriptenWindow::mouseup_cb(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData){
	instance->events().notifyMouseReleased(ofGetMouseX(),ofGetMouseY(),mouseEvent->button);
	return 0;

}

int ofxAppEmscriptenWindow::mousemoved_cb(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData){
	if(ofGetMousePressed()){
		instance->events().notifyMouseDragged(mouseEvent->targetX,mouseEvent->targetY,0);
	}else{
		instance->events().notifyMouseMoved(mouseEvent->targetX,mouseEvent->targetY);
	}
	return 0;

}

int ofxAppEmscriptenWindow::touch_cb(int eventType, const EmscriptenTouchEvent* e, void* userData) {

        ofTouchEventArgs::Type touchArgsType;
        switch (eventType) {
                    case EMSCRIPTEN_EVENT_TOUCHSTART:
                        touchArgsType = ofTouchEventArgs::down;
                        break;
                    case EMSCRIPTEN_EVENT_TOUCHEND:
                        touchArgsType = ofTouchEventArgs::up;
                        break;
                    case EMSCRIPTEN_EVENT_TOUCHMOVE:
                        touchArgsType = ofTouchEventArgs::move;
                        break;
                    case EMSCRIPTEN_EVENT_TOUCHCANCEL:
                        touchArgsType = ofTouchEventArgs::cancel;
                        break;
                    default:
                        return 1;
            }
        int numTouches = e->numTouches;
        for (int i = 0; i < numTouches; i++) {
                ofTouchEventArgs touchArgs;
                touchArgs.type = touchArgsType;
                touchArgs.id = i;
                touchArgs.x =  e->touches[i].targetX;
                touchArgs.y =  e->touches[i].targetY;
                instance->events().notifyTouchEvent(touchArgs);
           }
    return 0;
}

void ofxAppEmscriptenWindow::hideCursor(){
	emscripten_hide_mouse();
}

void ofxAppEmscriptenWindow::setWindowShape(int w, int h){
    emscripten_set_canvas_size(w,h);
}

glm::vec2 ofxAppEmscriptenWindow::getWindowPosition(){
	return glm::vec2(0,0);
}

glm::vec2 ofxAppEmscriptenWindow::getWindowSize(){
	int width;
	int height;

	emscripten_get_canvas_element_size("#canvas", &width, &height);
	return glm::vec2(width,height);
}

glm::vec2 ofxAppEmscriptenWindow::getScreenSize(){
	return getWindowSize();
}

ofOrientation ofxAppEmscriptenWindow::getOrientation(){
	return OF_ORIENTATION_DEFAULT;
}

bool ofxAppEmscriptenWindow::doesHWOrientation(){
	return false;
}

//this is used by ofGetWidth and now determines the window width based on orientation
int	ofxAppEmscriptenWindow::getWidth(){
	return getWindowSize().x;
}

int	ofxAppEmscriptenWindow::getHeight(){
	return getWindowSize().y;
}

ofWindowMode ofxAppEmscriptenWindow::getWindowMode(){
	return OF_WINDOW;
}

void ofxAppEmscriptenWindow::setFullscreen(bool fullscreen){
	if(fullscreen){
		emscripten_request_fullscreen(0,1);
	}else{
		emscripten_exit_fullscreen();
	}
}

void ofxAppEmscriptenWindow::toggleFullscreen(){
	EmscriptenFullscreenChangeEvent fullscreenStatus;
	emscripten_get_fullscreen_status(&fullscreenStatus);
	if(fullscreenStatus.isFullscreen){
		setFullscreen(false);
	}else if(fullscreenStatus.fullscreenEnabled){
		setFullscreen(true);
	}
}

void ofxAppEmscriptenWindow::enableSetupScreen(){
	bEnableSetupScreen = true;
}


void ofxAppEmscriptenWindow::disableSetupScreen(){
	bEnableSetupScreen = false;
}

EGLContext ofxAppEmscriptenWindow::getEGLContext(){
	return &context;
}

ofCoreEvents & ofxAppEmscriptenWindow::events(){
	return _events;
}

shared_ptr<ofBaseRenderer> & ofxAppEmscriptenWindow::renderer(){
	return _renderer;
}

void ofxAppEmscriptenWindow::makeCurrent(){
	if(context != 0){
		emscripten_webgl_make_context_current(context);	
	}
}

void ofxAppEmscriptenWindow::startRender(){
	renderer()->startRender();
}

void ofxAppEmscriptenWindow::finishRender(){
	renderer()->finishRender();
}
