
/**
 * libemulation-hal
 * OpenGL canvas
 * (C) 2010-2011 by Marc S. Ressl (mressl@umich.edu)
 * Released under the GPL
 *
 * Implements an OpenGL canvas.
 */

#ifndef _OPENGLCANVAS_H
#define _OPENGLCANVAS_H

#include <pthread.h>

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#include "OEComponent.h"
#include "CanvasInterface.h"

#define OPENGLCANVAS_PERSISTANCE_FRAME_NUM 6

typedef enum
{
	OPENGLCANVAS_CAPTURE_NONE,
	OPENGLCANVAS_CAPTURE_KEYBOARD_AND_DISCONNECT_MOUSE_CURSOR,
	OPENGLCANVAS_CAPTURE_KEYBOARD_AND_HIDE_MOUSE_CURSOR,
} OpenGLCanvasCapture;

typedef enum
{
	OPENGLCANVAS_TEXTURE_SHADOWMASK_TRIAD,
	OPENGLCANVAS_TEXTURE_SHADOWMASK_INLINE,
	OPENGLCANVAS_TEXTURE_SHADOWMASK_APERTURE,
	OPENGLCANVAS_TEXTURE_SHADOWMASK_LCD,
	OPENGLCANVAS_TEXTURE_SHADOWMASK_BAYER,
	OPENGLCANVAS_TEXTURE_BEZEL_POWER,
	OPENGLCANVAS_TEXTURE_BEZEL_PAUSE,
	OPENGLCANVAS_TEXTURE_BEZEL_CAPTURE,
	OPENGLCANVAS_TEXTURE_FRAME_RAW,
	OPENGLCANVAS_TEXTURE_FRAME_RENDERED,
	OPENGLCANVAS_TEXTURE_FRAME_RENDERED_END = (OPENGLCANVAS_TEXTURE_FRAME_RENDERED +
												OPENGLCANVAS_PERSISTANCE_FRAME_NUM),
	OPENGLCANVAS_TEXTURE_END,
} OpenGLCanvasTextureIndex;

typedef enum
{
	OPENGLCANVAS_SHADER_RGB,
	OPENGLCANVAS_SHADER_NTSC,
	OPENGLCANVAS_SHADER_PAL,
	OPENGLCANVAS_SHADER_DISPLAY,
	OPENGLCANVAS_SHADER_END,
} OpenGLCanvasProgram;

typedef void (*CanvasSetCapture)(void *userData, OpenGLCanvasCapture capture);
typedef void (*CanvasSetKeyboardFlags)(void *userData, OEUInt32 flags);



class OpenGLCanvas : public OEComponent
{
public:
	OpenGLCanvas(string resourcePath);
	~OpenGLCanvas();
	
	void open(CanvasSetCapture setCapture,
			  CanvasSetKeyboardFlags setKeyboardFlags,
			  void *userData);
	void close();
	
	void setEnableShader(bool value);
	CanvasMode getMode();
	OESize getResolution();
	OEImage getFrame();
	bool update(float width, float height, float origin, bool isVSync);
	
	void becomeKeyWindow();
	void resignKeyWindow();
	void setKey(int usageId, bool value);
	void sendUnicodeKeyEvent(int unicode);
	
	void setMouseButton(int index, bool value);
	void enterMouse();
	void exitMouse();
	void setMousePosition(float x, float y);
	void moveMouse(float rx, float ry);
	void sendMouseWheelEvent(int index, float value);
	
	void setJoystickButton(int deviceIndex, int index, bool value);
	void setJoystickPosition(int deviceIndex, int index, float value);
	void sendJoystickHatEvent(int deviceIndex, int index, float value);
	void moveJoystickBall(int deviceIndex, int index, float value);
	
	void copy(string& value);
	void paste(string value);
	
	bool postMessage(OEComponent *sender, int message, void *data);
	void notify(OEComponent *sender, int notification, void *data);
	bool addObserver(OEComponent *observer, int notification);
	bool removeObserver(OEComponent *observer, int notification);
	
private:
	string resourcePath;
	
	CanvasSetCapture setCapture;
	CanvasSetKeyboardFlags setKeyboardFlags;
	void *userData;
	
	bool isOpen;
	bool isShaderEnabled;
	
	pthread_mutex_t mutex;
	
	CanvasMode mode;
	CanvasCaptureMode captureMode;
	
	OESize viewportSize;
	GLuint texture[OPENGLCANVAS_TEXTURE_END];
	OESize textureSize[OPENGLCANVAS_TEXTURE_END];
	
	bool isDisplayConfigurationUpdated;
	CanvasDisplayConfiguration displayConfiguration;
	bool isFrameUpdated;
	OEImage frame;
	bool isShaderActive;
	GLuint shader[OPENGLCANVAS_SHADER_END];
	int renderIndex;
	GLuint renderShader;
	int persistance[OPENGLCANVAS_PERSISTANCE_FRAME_NUM];
	
	OEImage paper;
	bool isPaperUpdated;
	
	CanvasBezel bezel;
	bool isBezelDrawRequired;
	bool isBezelCapture;
	double bezelCaptureTime;
	
	OpenGLCanvasCapture capture;
	
	bool keyDown[CANVAS_KEYBOARD_KEY_NUM];
	int keyDownCount;
	bool ctrlAltWasPressed;
	bool mouseEntered;
	bool mouseButtonDown[CANVAS_MOUSE_BUTTON_NUM];
	bool joystickButtonDown[CANVAS_JOYSTICK_NUM][CANVAS_JOYSTICK_BUTTON_NUM];
	
	void lock();
	void unlock();
	
	bool initOpenGL();
	void freeOpenGL();
	GLuint getGLFormat(OEImageFormat format);
	void loadTextures();
	void loadTexture(string path, bool isMipmap, int textureIndex);
	void updateTextureSize(int textureIndex, OESize size);
	void loadShaders();
	void deleteShaders();
	GLuint loadShader(const char *source);
	void deleteShader(GLuint glShader);
	void updateViewport();
	
	bool uploadFrame();
	void updateDisplayConfiguration();
	void renderFrame();
	bool isDisplayDrawRequired();
	void drawDisplayCanvas();
	void updatePersistance();
	
	void drawPaperCanvas(float origin);
	
	double getCurrentTime();
	void drawBezel();
	
	void postHIDNotification(int notification, int usageId, float value);
	void updateCapture(OpenGLCanvasCapture capture);
	void resetKeysAndButtons();
	
	bool setMode(CanvasMode *mode);
	bool setCaptureMode(CanvasCaptureMode *captureMode);
	bool setBezel(CanvasBezel *bezel);
	bool setDisplayConfiguration(CanvasDisplayConfiguration *configuration);
	bool postFrame(OEImage *frame);
	bool setPaperConfiguration(CanvasPaperConfiguration *configuration);
	bool printImage(OEImage *image);
};

#endif
