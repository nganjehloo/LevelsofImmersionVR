/************************************************************************************

Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
Copyright   :   Copyright Brad Davis. All Rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

************************************************************************************/


#include <iostream>
#include <memory>
#include <exception>
#include <algorithm>
#include <time.h>
#include <Windows.h>

#define __STDC_FORMAT_MACROS 1

#define FAIL(X) throw std::runtime_error(X)

#include "OVRUtil.h"

using namespace std;

///////////////////////////////////////////////////////////////////////////////
//
// GLEW gives cross platform access to OpenGL 3.x+ functionality.  
//

#include <GL/glew.h>

bool checkFramebufferStatus(GLenum target = GL_FRAMEBUFFER) {
	GLuint status = glCheckFramebufferStatus(target);
	switch (status) {
	case GL_FRAMEBUFFER_COMPLETE:
		return true;
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		std::cerr << "framebuffer incomplete attachment" << std::endl;
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		std::cerr << "framebuffer missing attachment" << std::endl;
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		std::cerr << "framebuffer incomplete draw buffer" << std::endl;
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		std::cerr << "framebuffer incomplete read buffer" << std::endl;
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		std::cerr << "framebuffer incomplete multisample" << std::endl;
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
		std::cerr << "framebuffer incomplete layer targets" << std::endl;
		break;

	case GL_FRAMEBUFFER_UNSUPPORTED:
		std::cerr << "framebuffer unsupported internal format or image" << std::endl;
		break;

	default:
		std::cerr << "other framebuffer error" << std::endl;
		break;
	}

	return false;
}

bool checkGlError() {
	GLenum error = glGetError();
	if (!error) {
		return false;
	}
	else {
		switch (error) {
		case GL_INVALID_ENUM:
			std::cerr << ": An unacceptable value is specified for an enumerated argument.The offending command is ignored and has no other side effect than to set the error flag.";
			break;
		case GL_INVALID_VALUE:
			std::cerr << ": A numeric argument is out of range.The offending command is ignored and has no other side effect than to set the error flag";
			break;
		case GL_INVALID_OPERATION:
			std::cerr << ": The specified operation is not allowed in the current state.The offending command is ignored and has no other side effect than to set the error flag..";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			std::cerr << ": The framebuffer object is not complete.The offending command is ignored and has no other side effect than to set the error flag.";
			break;
		case GL_OUT_OF_MEMORY:
			std::cerr << ": There is not enough memory left to execute the command.The state of the GL is undefined, except for the state of the error flags, after this error is recorded.";
			break;
		case GL_STACK_UNDERFLOW:
			std::cerr << ": An attempt has been made to perform an operation that would cause an internal stack to underflow.";
			break;
		case GL_STACK_OVERFLOW:
			std::cerr << ": An attempt has been made to perform an operation that would cause an internal stack to overflow.";
			break;
		}
		return true;
	}
}

void glDebugCallbackHandler(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *msg, GLvoid* data) {
	OutputDebugStringA(msg);
	std::cout << "debug call: " << msg << std::endl;
}

//////////////////////////////////////////////////////////////////////
//
// GLFW provides cross platform window creation
//

#include <GLFW/glfw3.h>

namespace glfw {
	inline GLFWwindow * createWindow(const uvec2 & size, const ivec2 & position = ivec2(INT_MIN)) {
		GLFWwindow * window = glfwCreateWindow(size.x, size.y, "glfw", nullptr, nullptr);
		if (!window) {
			FAIL("Unable to create rendering window");
		}
		if ((position.x > INT_MIN) && (position.y > INT_MIN)) {
			glfwSetWindowPos(window, position.x, position.y);
		}
		return window;
	}
}

// A class to encapsulate using GLFW to handle input and render a scene
class GlfwApp {

protected:
	uvec2 windowSize;
	ivec2 windowPosition;
	GLFWwindow * window{ nullptr };
	unsigned int frame{ 0 };

public:
	GlfwApp() {
		// Initialize the GLFW system for creating and positioning windows
		if (!glfwInit()) {
			FAIL("Failed to initialize GLFW");
		}
		glfwSetErrorCallback(ErrorCallback);
	}

	virtual ~GlfwApp() {
		if (nullptr != window) {
			glfwDestroyWindow(window);
		}
		glfwTerminate();
	}

	virtual int run() {
		preCreate();

		window = createRenderingTarget(windowSize, windowPosition);

		if (!window) {
			std::cout << "Unable to create OpenGL window" << std::endl;
			return -1;
		}

		postCreate();

		initGl();

		while (!glfwWindowShouldClose(window)) {
			++frame;
			glfwPollEvents();
			update();
			draw();
			finishFrame();
		}

		shutdownGl();

		return 0;
	}


protected:
	virtual GLFWwindow * createRenderingTarget(uvec2 & size, ivec2 & pos) = 0;

	virtual void draw() = 0;

	void preCreate() {
		glfwWindowHint(GLFW_DEPTH_BITS, 16);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
	}


	void postCreate() {
		glfwSetWindowUserPointer(window, this);
		glfwSetKeyCallback(window, KeyCallback);
		glfwSetMouseButtonCallback(window, MouseButtonCallback);
		glfwMakeContextCurrent(window);

		// Initialize the OpenGL bindings
		// For some reason we have to set this experminetal flag to properly
		// init GLEW if we use a core context.
		glewExperimental = GL_TRUE;
		if (0 != glewInit()) {
			FAIL("Failed to initialize GLEW");
		}
		glGetError();

		if (GLEW_KHR_debug) {
			GLint v;
			glGetIntegerv(GL_CONTEXT_FLAGS, &v);
			if (v & GL_CONTEXT_FLAG_DEBUG_BIT) {
				//glDebugMessageCallback(glDebugCallbackHandler, this);
			}
		}
	}

	virtual void initGl() {
	}

	virtual void shutdownGl() {
	}

	virtual void finishFrame() {
		glfwSwapBuffers(window);
	}

	virtual void destroyWindow() {
		glfwSetKeyCallback(window, nullptr);
		glfwSetMouseButtonCallback(window, nullptr);
		glfwDestroyWindow(window);
	}

	virtual void onKey(int key, int scancode, int action, int mods) {
		if (GLFW_PRESS != action) {
			return;
		}

		switch (key) {
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, 1);
			return;
		}
	}

	virtual void update() {}

	virtual void onMouseButton(int button, int action, int mods) {}

protected:
	virtual void viewport(const ivec2 & pos, const uvec2 & size) {
		glViewport(pos.x, pos.y, size.x, size.y);
	}

private:

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		GlfwApp * instance = (GlfwApp *)glfwGetWindowUserPointer(window);
		instance->onKey(key, scancode, action, mods);
	}

	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
		GlfwApp * instance = (GlfwApp *)glfwGetWindowUserPointer(window);
		instance->onMouseButton(button, action, mods);
	}

	static void ErrorCallback(int error, const char* description) {
		FAIL(description);
	}
};

class RiftManagerApp {
protected:
	ovrSession _session;
	ovrHmdDesc _hmdDesc;
	ovrGraphicsLuid _luid;

public:
	RiftManagerApp() {
		if (!OVR_SUCCESS(ovr_Create(&_session, &_luid))) {
			FAIL("Unable to create HMD session");
		}

		_hmdDesc = ovr_GetHmdDesc(_session);
	}

	~RiftManagerApp() {
		ovr_Destroy(_session);
		_session = nullptr;
	}
};

// Global used by draw calls to know which eye is currently being rendered to
ovrEyeType currentEye = ovrEye_Left;

class RiftApp : public GlfwApp, public RiftManagerApp {
public:

private:
	GLuint _fbo{ 0 };
	GLuint _depthBuffer{ 0 };
	ovrTextureSwapChain _eyeTexture;

	GLuint _mirrorFbo{ 0 };
	ovrMirrorTexture _mirrorTexture;

	ovrEyeRenderDesc _eyeRenderDescs[2];

	mat4 _eyeProjections[2];

	ovrLayerEyeFov _sceneLayer;
	ovrViewScaleDesc _viewScaleDesc;

	uvec2 _renderTargetSize;
	uvec2 _mirrorSize;

public:

	RiftApp() {
		using namespace ovr;
		_viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;

		memset(&_sceneLayer, 0, sizeof(ovrLayerEyeFov));
		_sceneLayer.Header.Type = ovrLayerType_EyeFov;
		_sceneLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;

		ovr::for_each_eye([&](ovrEyeType eye) {
			ovrEyeRenderDesc& erd = _eyeRenderDescs[eye] = ovr_GetRenderDesc(_session, eye, _hmdDesc.DefaultEyeFov[eye]);
			ovrMatrix4f ovrPerspectiveProjection =
				ovrMatrix4f_Projection(erd.Fov, 0.01f, 1000.0f, ovrProjection_ClipRangeOpenGL);
			_eyeProjections[eye] = ovr::toGlm(ovrPerspectiveProjection);
			_viewScaleDesc.HmdToEyeOffset[eye] = erd.HmdToEyeOffset; // cse190: adjust the eye separation here - need to use 3D vector from central point on Rift for each eye

			ovrFovPort & fov = _sceneLayer.Fov[eye] = _eyeRenderDescs[eye].Fov;
			auto eyeSize = ovr_GetFovTextureSize(_session, eye, fov, 1.0f);
			_sceneLayer.Viewport[eye].Size = eyeSize;
			_sceneLayer.Viewport[eye].Pos = { (int)_renderTargetSize.x, 0 };

			_renderTargetSize.y = std::max(_renderTargetSize.y, (uint32_t)eyeSize.h);
			_renderTargetSize.x += eyeSize.w;
		});
		// Make the on screen window 1/4 the resolution of the render target
		_mirrorSize = _renderTargetSize;
		_mirrorSize /= 4;
	}

protected:
	enum RenderMode { STEREO, MONO, LEFT, RIGHT };
	enum TrackingMode { BOTH, ORIENT, POS, NONE };
	enum SceneMode { CUBE, SKYBOX, SIMULTANEOUS, CUSTOM };
	RenderMode currentRenderMode = STEREO;
	TrackingMode currentTrackingMode = BOTH;
	SceneMode currentSceneMode = SIMULTANEOUS;
	ovrPosef setEyePoses[2];
	clock_t t;
	clock_t APressTime = 0;
	bool AReleased = true;
	clock_t BPressTime = 0;
	bool BReleased = true;
	clock_t XPressTime = 0;
	bool XReleased = true;
	bool RTReleased = true;
	bool LTReleased = true;
	bool controllerViewpoint = false;
	const float INIT_SCALE = 1.0f;
	float scaleamt = INIT_SCALE;
	float lastScaleAmt = 0.0f;
	vec3 transamt = vec3(0.0f);
	const vec3 INIT_TRANS = vec3(0.0f);

	GLFWwindow * createRenderingTarget(uvec2 & outSize, ivec2 & outPosition) override {
		return glfw::createWindow(_mirrorSize);
	}

	void initGl() override {
		APressTime = clock();
		BPressTime = clock();
		XPressTime = clock();

		GlfwApp::initGl();

		// Disable the v-sync for buffer swap
		glfwSwapInterval(0);

		ovrTextureSwapChainDesc desc = {};
		desc.Type = ovrTexture_2D;
		desc.ArraySize = 1;
		desc.Width = _renderTargetSize.x;
		desc.Height = _renderTargetSize.y;
		desc.MipLevels = 1;
		desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.SampleCount = 1;
		desc.StaticImage = ovrFalse;
		ovrResult result = ovr_CreateTextureSwapChainGL(_session, &desc, &_eyeTexture);
		_sceneLayer.ColorTexture[0] = _eyeTexture;
		if (!OVR_SUCCESS(result)) {
			FAIL("Failed to create swap textures");
		}

		int length = 0;
		result = ovr_GetTextureSwapChainLength(_session, _eyeTexture, &length);
		if (!OVR_SUCCESS(result) || !length) {
			FAIL("Unable to count swap chain textures");
		}
		for (int i = 0; i < length; ++i) {
			GLuint chainTexId;
			ovr_GetTextureSwapChainBufferGL(_session, _eyeTexture, i, &chainTexId);
			glBindTexture(GL_TEXTURE_2D, chainTexId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		glBindTexture(GL_TEXTURE_2D, 0);

		// Set up the framebuffer object
		glGenFramebuffers(1, &_fbo);
		glGenRenderbuffers(1, &_depthBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
		glBindRenderbuffer(GL_RENDERBUFFER, _depthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, _renderTargetSize.x, _renderTargetSize.y);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		ovrMirrorTextureDesc mirrorDesc;
		memset(&mirrorDesc, 0, sizeof(mirrorDesc));
		mirrorDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
		mirrorDesc.Width = _mirrorSize.x;
		mirrorDesc.Height = _mirrorSize.y;
		if (!OVR_SUCCESS(ovr_CreateMirrorTextureGL(_session, &mirrorDesc, &_mirrorTexture))) {
			FAIL("Could not create mirror texture");
		}
		glGenFramebuffers(1, &_mirrorFbo);
	}

	void onKey(int key, int scancode, int action, int mods) override {
		if (GLFW_PRESS == action) switch (key) {
		case GLFW_KEY_R:
			ovr_RecenterTrackingOrigin(_session);
			return;
		}

		GlfwApp::onKey(key, scancode, action, mods);
	}

	void draw() final override {
		ovrPosef eyePoses[2];
		float handPosOffset = 0.065f / 2.0f;
		
		// Render from headset point of view
		if (!controllerViewpoint)
		{
			ovr_GetEyePoses(_session, frame, true, _viewScaleDesc.HmdToEyeOffset, eyePoses, &_sceneLayer.SensorSampleTime);
		}
		// Render from right controller point of view
		else
		{
			// Get the position of the right hand for hand viewpoint
			double displayMidpointSeconds = ovr_GetPredictedDisplayTime(_session, frame);
			ovrTrackingState trackState = ovr_GetTrackingState(_session, displayMidpointSeconds, ovrTrue);
			ovrPosef handPose = trackState.HandPoses[ovrEye_Right].ThePose;

			// set up the left "eye" (camera)
			vec3 leftPos = ovr::toGlm(handPose.Position) - glm::vec3(handPosOffset, 0.0f, 0.0f);
			eyePoses[ovrEye_Left].Orientation = handPose.Orientation;
			eyePoses[ovrEye_Left].Position = ovr::fromGlm(leftPos);

			// set up the right "eye" (camera)
			vec3 rightPos = ovr::toGlm(handPose.Position) + glm::vec3(handPosOffset, 0.0f, 0.0f);
			eyePoses[ovrEye_Right].Orientation = handPose.Orientation;
			eyePoses[ovrEye_Right].Position = ovr::fromGlm(leftPos);
		}

		int curIndex;
		ovr_GetTextureSwapChainCurrentIndex(_session, _eyeTexture, &curIndex);
		GLuint curTexId;
		ovr_GetTextureSwapChainBufferGL(_session, _eyeTexture, curIndex, &curTexId);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		ovr::for_each_eye([&](ovrEyeType eye) {
			currentEye = eye;
			const auto& vp = _sceneLayer.Viewport[eye];
			glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);
			_sceneLayer.RenderPose[eye] = eyePoses[eye];
			//renderScene(_eyeProjections[eye], ovr::toGlm(eyePoses[eye]));  // cse190: this is for normal stereo rendering
//			renderScene(_eyeProjections[eye], ovr::toGlm(eyePoses[ovrEye_Left]));  // cse190: use eyePoses[ovrEye_Left] to render one eye's view to both eyes = monoscopic view
//			if (eye==ovrEye_Left) renderScene(_eyeProjections[eye], ovr::toGlm(eyePoses[eye]));  // cse190: this is how to render to only one eye


			switch (currentTrackingMode)
			{
			case RiftApp::BOTH:
				setEyePoses[eye].Orientation = eyePoses[eye].Orientation;
				setEyePoses[eye].Position = eyePoses[eye].Position;
				break;
			case RiftApp::ORIENT:
				setEyePoses[eye].Orientation = eyePoses[eye].Orientation;
				break;
			case RiftApp::POS:
				setEyePoses[eye].Position = eyePoses[eye].Position;
				break;
			case RiftApp::NONE:
				break;
			default:
				setEyePoses[eye].Orientation = eyePoses[eye].Orientation;
				setEyePoses[eye].Position = eyePoses[eye].Position;
				break;
			}
			
			if (currentRenderMode == STEREO)
				renderScene(_eyeProjections[eye], ovr::toGlm(setEyePoses[eye]));  // cse190: this is for normal stereo rendering
			else if (currentRenderMode == MONO)
				renderScene(_eyeProjections[eye], ovr::toGlm(setEyePoses[ovrEye_Left]));  // cse190: use eyePoses[ovrEye_Left] to render one eye's view to both eyes = monoscopic view
			else if (currentRenderMode == LEFT && eye == ovrEye_Left)
				renderScene(_eyeProjections[eye], ovr::toGlm(setEyePoses[eye]));  // cse190: this is how to render to only one eye
			else if (currentRenderMode == RIGHT && eye == ovrEye_Right)
				renderScene(_eyeProjections[eye], ovr::toGlm(setEyePoses[eye]));

		});
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		ovr_CommitTextureSwapChain(_session, _eyeTexture);
		ovrLayerHeader* headerList = &_sceneLayer.Header;
		ovr_SubmitFrame(_session, frame, &_viewScaleDesc, &headerList, 1);

		GLuint mirrorTextureId;
		ovr_GetMirrorTextureBufferGL(_session, _mirrorTexture, &mirrorTextureId);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, _mirrorFbo);
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mirrorTextureId, 0);
		glBlitFramebuffer(0, 0, _mirrorSize.x, _mirrorSize.y, 0, _mirrorSize.y, _mirrorSize.x, 0, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}
	
	void update() final override 
	{
		ovrInputState inputState;
		float IODstep = 0.0f;
		float scalestep = 0.0f;

		if (OVR_SUCCESS(ovr_GetInputState(_session, ovrControllerType_Touch, &inputState)))
		{
			// move cube left/right/forward/backward with left thumbstick
			if (fabs(inputState.Thumbstick[ovrHand_Left].x) > 0.1f) {
				transamt.x += inputState.Thumbstick[ovrHand_Left].x * 0.01f;
			}
			if (fabs(inputState.Thumbstick[ovrHand_Left].y) > 0.1f) {
				transamt.z -= inputState.Thumbstick[ovrHand_Left].y * 0.01f;
			}

			// move cube up/down with right thumbstick y axis
			if (fabs(inputState.Thumbstick[ovrHand_Right].y) > 0.1f) {
				transamt.y += inputState.Thumbstick[ovrHand_Right].y * 0.01f;
			}
			// scale the cube with the right thumbstick x axis
			if (fabs(inputState.Thumbstick[ovrHand_Right].x) > 0.1f) {
				scalestep = (inputState.Thumbstick[ovrHand_Right].x* 0.01f);
				scaleamt = INIT_SCALE + scalestep;
			}

			//Changes to controller viewpoint
			if (inputState.HandTrigger[ovrHand_Right] >= 0.5f && !controllerViewpoint)
			{
				cerr << "Switching to controller viewpoint" << endl;
				controllerViewpoint = true;
			}

			// Changes viewpoint back to headset
			else if (inputState.HandTrigger[ovrHand_Right] < 0.5f && controllerViewpoint)
			{
				cerr << "Switching back to headset viewpoint" << endl;
				controllerViewpoint = false;
			}

			if (inputState.IndexTrigger[ovrHand_Right] > 0.5f)	cerr << "right index trigger pressed"  << endl;
			if (inputState.HandTrigger[ovrHand_Left] > 0.5f)    cerr << "left middle trigger pressed"  << endl;
			if (inputState.IndexTrigger[ovrHand_Left] > 0.5f)	cerr << "left index trigger pressed"   << endl;

			int state = 0;
			if ((state = inputState.Buttons) > 0) {
				////Changes Rendering Mode
				//if ((state == ovrButton_A) && AReleased) {
				//	APressTime = clock();
				//	AReleased = false;
				//	currentRenderMode = static_cast<RenderMode>((currentRenderMode + 1) % 4);
				//	cerr << "RenderMode:" << currentRenderMode << endl;
				//}

				////Changes Tracking Mode
				//if ((state == ovrButton_B) && BReleased) {
				//	BReleased = false;
				//	currentTrackingMode = static_cast<TrackingMode>((currentTrackingMode + 1) % 4);
				//	cout << "TrackingMode:" << currentTrackingMode << endl;
				//}

				////Changes Scene Mode
				//if ((state == ovrButton_X) && XReleased) {
				//	XReleased = false;
				//	switch (currentSceneMode)
				//	{
				//	case CUBE:
				//		currentSceneMode = SKYBOX;
				//		break;
				//	case SKYBOX:
				//		currentSceneMode = SIMULTANEOUS;
				//		break;
				//	case SIMULTANEOUS:
				//		currentSceneMode = CUSTOM;
				//		break;
				//	case CUSTOM:
				//		currentSceneMode = CUBE;
				//		break;
				//	default:
				//		break;
				//	}
				//	cout << "New SceneMode:" << currentSceneMode << endl;
				//}

				//if ((state == ovrButton_RThumb) && RTReleased) {
				//	RTReleased = false;
				//	_viewScaleDesc.HmdToEyeOffset[0].x = -0.0299428f;
				//	_viewScaleDesc.HmdToEyeOffset[1].x = 0.0299428f;
				//	cerr << "Reset IODR:" << _viewScaleDesc.HmdToEyeOffset[1].x << endl;
				//	cerr << "Reset IODL:" << _viewScaleDesc.HmdToEyeOffset[0].x << endl;
				//}

				if ((state == ovrButton_LThumb) && LTReleased) {
					RTReleased = false;
					scaleamt = INIT_SCALE;
					cerr << "Reset Scale:" << scaleamt << endl;
				}
			}

			if (state != ovrButton_A && state <= 0) {
				AReleased = true;
			}
			if (state != ovrButton_B && state <= 0) {
				BReleased = true;
			}
			if (state != ovrButton_X && state <= 0) {
				XReleased = true;
			}
			if (state != ovrButton_RThumb && state <= 0) {
				RTReleased = true;
			}
			if (state != ovrButton_LThumb && state <= 0) {
				LTReleased = true;
			}
		}

		updateScene();
	}

	virtual void renderScene(const glm::mat4 & projection, const glm::mat4 & headPose) = 0;
	virtual void updateScene() = 0;
};

//////////////////////////////////////////////////////////////////////
//
// The remainder of this code is specific to the scene we want to 
// render.  I use oglplus to render an array of cubes, but your 
// application would perform whatever rendering you want
//


//////////////////////////////////////////////////////////////////////
//
// OGLplus is a set of wrapper classes for giving OpenGL a more object
// oriented interface
//
#define OGLPLUS_USE_GLCOREARB_H 0
#define OGLPLUS_USE_GLEW 1
#define OGLPLUS_USE_BOOST_CONFIG 0
#define OGLPLUS_NO_SITE_CONFIG 1
#define OGLPLUS_LOW_PROFILE 1

#pragma warning( disable : 4068 4244 4267 4065)
#include <oglplus/config/basic.hpp>
#include <oglplus/config/gl.hpp>
#include <oglplus/all.hpp>
#include <oglplus/interop/glm.hpp>
#include <oglplus/bound/texture.hpp>
#include <oglplus/bound/framebuffer.hpp>
#include <oglplus/bound/renderbuffer.hpp>
#include <oglplus/bound/buffer.hpp>
#include <oglplus/shapes/cube.hpp>
#include <oglplus/shapes/wrapper.hpp>
#pragma warning( default : 4068 4244 4267 4065)



namespace Attribute {
	enum {
		Position = 0,
		TexCoord0 = 1,
		Normal = 2,
		Color = 3,
		TexCoord1 = 4,
		InstanceTransform = 5,
	};
}

static const char * VERTEX_SHADER = R"SHADER(
#version 410 core

uniform mat4 ProjectionMatrix = mat4(1);
uniform mat4 CameraMatrix = mat4(1);

layout(location = 0) in vec4 Position;
layout(location = 2) in vec3 Normal;
layout(location = 5) in mat4 InstanceTransform;

out vec3 vertNormal;

void main(void) {
   mat4 ViewXfm = CameraMatrix * InstanceTransform;
   //mat4 ViewXfm = CameraMatrix;
   vertNormal = Normal;
   gl_Position = ProjectionMatrix * ViewXfm * Position;
}
)SHADER";

static const char * FRAGMENT_SHADER = R"SHADER(
#version 410 core

in vec3 vertNormal;
out vec4 fragColor;

void main(void) {
    vec3 color = vertNormal;
    if (!all(equal(color, abs(color)))) {
        color = vec3(1.0) - abs(color);
    }
    fragColor = vec4(color, 1.0);
}
)SHADER";

// a class for encapsulating building and rendering an RGB cube
struct ColorCubeScene {

	// Program
	oglplus::shapes::ShapeWrapper cube;
	oglplus::Program prog;
	oglplus::VertexArray vao;
	GLuint instanceCount;
	oglplus::Buffer instances;

	// VBOs for the cube's vertices and normals

	const unsigned int GRID_SIZE{ 5 };

public:
	ColorCubeScene() : cube({ "Position", "Normal" }, oglplus::shapes::Cube()) {
		using namespace oglplus;
		try {
			// attach the shaders to the program
			prog.AttachShader(
				FragmentShader()
				.Source(GLSLSource(String(FRAGMENT_SHADER)))
				.Compile()
			);
			prog.AttachShader(
				VertexShader()
				.Source(GLSLSource(String(VERTEX_SHADER)))
				.Compile()
			);
			prog.Link();
		}
		catch (ProgramBuildError & err) {
			FAIL((const char*)err.what());
		}

		// link and use it
		prog.Use();

		vao = cube.VAOForProgram(prog);
		vao.Bind();
		// Create a cube of cubes
		{
			std::vector<mat4> instance_positions;
			for (unsigned int z = 0; z < GRID_SIZE; ++z) {
				for (unsigned int y = 0; y < GRID_SIZE; ++y) {
					for (unsigned int x = 0; x < GRID_SIZE; ++x) {
						int xpos = (x - (GRID_SIZE / 2)) * 2;
						int ypos = (y - (GRID_SIZE / 2)) * 2;
						int zpos = (z - (GRID_SIZE / 2)) * 2;
						vec3 relativePosition = vec3(xpos, ypos, zpos);
						if (relativePosition == vec3(0)) {
							continue;
						}
						instance_positions.push_back(glm::translate(glm::mat4(1.0f), relativePosition));
					}
				}
			}

			Context::Bound(Buffer::Target::Array, instances).Data(instance_positions);
			instanceCount = (GLuint)instance_positions.size();
			int stride = sizeof(mat4);
			for (int i = 0; i < 4; ++i) {
				VertexArrayAttrib instance_attr(prog, Attribute::InstanceTransform + i);
				size_t offset = sizeof(vec4) * i;
				instance_attr.Pointer(4, DataType::Float, false, stride, (void*)offset);
				instance_attr.Divisor(1);
				instance_attr.Enable();
			}
		}
	}

	void update() {

	}

	void render(const mat4 & projection, const mat4 & modelview) {
		using namespace oglplus;
		prog.Use();
		Uniform<mat4>(prog, "ProjectionMatrix").Set(projection);
		Uniform<mat4>(prog, "CameraMatrix").Set(modelview);  // cse190: this is default for normal rendering with head tracking
//		Uniform<mat4>(prog, "CameraMatrix").Set(identity);   // cse190: changing modelview to identity freezes scene and head motion
		vao.Bind();
		cube.Draw(instanceCount);
	}
};

#define SB_VERT_SHADER "skybox.vert"
#define SB_FRAG_SHADER "skybox.frag"
#define CUBE_VERT_SHADER "cube.vert"
#define CUBE_FRAG_SHADER "cube.frag"

#include "skybox.h"
#include "cube.h"

// An example application that renders a simple cube
class ExampleApp : public RiftApp {
	//std::shared_ptr<ColorCubeScene> cubeScene;
	Shader* skyboxShader = NULL;
	Shader* cubeShader = NULL;
	Skybox* skybox_left = NULL;
	Skybox* skybox_right = NULL;
	Skybox* custom_left = NULL;
	Skybox* custom_right = NULL;
	Cube* cube = NULL;

public:
	ExampleApp() {}

	~ExampleApp()
	{
		delete skyboxShader;
		delete cubeShader;
		delete skybox_left;
		delete skybox_right;
		delete custom_left;
		delete custom_right;
		delete cube;
	}

protected:
	void initGl() override {
		RiftApp::initGl();
		glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
		glEnable(GL_DEPTH_TEST);
		ovr_RecenterTrackingOrigin(_session);
		//cubeScene = std::shared_ptr<ColorCubeScene>(new ColorCubeScene());

		skyboxShader = new Shader(SB_VERT_SHADER, SB_FRAG_SHADER);
		cubeShader = new Shader(CUBE_VERT_SHADER, CUBE_FRAG_SHADER);
		skybox_left = new Skybox(ovrEye_Left, false);
		skybox_right = new Skybox(ovrEye_Right, false);
		custom_left = new Skybox(ovrEye_Left, true);
		custom_right = new Skybox(ovrEye_Right, true);
		cube = new Cube();
	}

	void shutdownGl() override {
		//cubeScene.reset();
	}

	void onKey(int key, int scancode, int action, int mods) override {
		RiftApp::onKey(key, scancode, action, mods);

		if (GLFW_PRESS == action)
		{
			switch (key)
			{
			case GLFW_KEY_T:
				switch (currentSceneMode)
				{
				case CUBE:
					currentSceneMode = SKYBOX;
					break;
				case SKYBOX:
					currentSceneMode = SIMULTANEOUS;
					break;
				case SIMULTANEOUS:
					currentSceneMode = CUBE;
					break;
				default:
					break;
				}
				cout << "New SceneMode:" << currentSceneMode << endl;
				break;

			default:
				break;
			}
		}
	}

	void updateScene() override 
	{
		if (transamt != INIT_TRANS)
		{
			cube->translate(transamt);
			transamt = INIT_TRANS;
		}

		if (scaleamt != lastScaleAmt)
		{
			cube->scale(scaleamt);
			lastScaleAmt = scaleamt;
		}
	}

	void renderScene(const glm::mat4 & projection, const glm::mat4 & headPose) override 
	{
		// Render the cube
		if (currentSceneMode == SIMULTANEOUS || currentSceneMode == CUSTOM || currentSceneMode == CUBE)
		{
			cubeShader->Use();
			GLuint cubeViewLoc = glGetUniformLocation(cubeShader->Program, "view");
			GLuint cubeProjLoc = glGetUniformLocation(cubeShader->Program, "projection");
			glUniformMatrix4fv(cubeViewLoc, 1, GL_FALSE, &glm::inverse(headPose)[0][0]);
			glUniformMatrix4fv(cubeProjLoc, 1, GL_FALSE, &projection[0][0]);

			cube->draw(*cubeShader);
			//cubeScene->render(projection, glm::inverse(headPose));
		}
		
		if (currentSceneMode == SIMULTANEOUS || currentSceneMode == SKYBOX)
		{
			// Render the skybox
			skyboxShader->Use();
			GLuint viewLoc = glGetUniformLocation(skyboxShader->Program, "view");
			GLuint projLoc = glGetUniformLocation(skyboxShader->Program, "projection");
			glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &glm::inverse(headPose)[0][0]);
			glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);

			if (currentRenderMode != MONO)
			{
				if (currentEye == ovrEye_Left)
					skybox_left->draw(*skyboxShader);
				else
					skybox_right->draw(*skyboxShader);
			}
			else
			{
				skybox_left->draw(*skyboxShader);
			}
		}

		if (currentSceneMode == CUSTOM)
		{
			// Render the skybox
			skyboxShader->Use();
			GLuint viewLoc = glGetUniformLocation(skyboxShader->Program, "view");
			GLuint projLoc = glGetUniformLocation(skyboxShader->Program, "projection");
			glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &glm::inverse(headPose)[0][0]);
			glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);

			if (currentRenderMode != MONO)
			{
				if (currentEye == ovrEye_Left)
					custom_left->draw(*skyboxShader);
				else
					custom_right->draw(*skyboxShader);
			}
			else
			{
				custom_left->draw(*skyboxShader);
			}
		}
	}
};
 
// Execute our example class
int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	int result = -1;

	try {
		AllocConsole();
		freopen("conin$", "r", stdin);
		freopen("conout$", "w", stdout);
		freopen("conout$", "w", stderr);
		if (!OVR_SUCCESS(ovr_Initialize(nullptr))) {
			FAIL("Failed to initialize the Oculus SDK");
		}
		result = ExampleApp().run();
	}
	catch (std::exception & error) {
		OutputDebugStringA(error.what());
		std::cerr << error.what() << std::endl;
	}
	ovr_Shutdown();
	return result;
}