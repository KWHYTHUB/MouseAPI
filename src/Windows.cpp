#include <Sapphire/DefaultInclude.hpp>

#ifdef SAPPHIRE_IS_WINDOWS
#include <Sapphire/cocos/robtop/glfw/glfw3.h>
#include "../include/API.hpp"
#include "Platform.hpp"

using namespace sapphire::prelude;
using namespace mouse;

struct _GLFWwindow
{
    struct _GLFWwindow* next;									// 0

    // Window settings and state
    GLboolean           iconified;								// 4
    GLboolean           resizable;
    GLboolean           decorated;
    GLboolean           visible;
    GLboolean           closed;									// 8
    void*               userPointer;							// c
    GLFWvidmode         videoMode;								// 10
    GLFWmonitor*        monitor;								// 28

    // Window input state
    GLboolean           stickyKeys;								// 2c
    GLboolean           stickyMouseButtons;
    double              cursorPosX, cursorPosY;					// 30
    int                 cursorMode;								// 40
    char                mouseButton[GLFW_MOUSE_BUTTON_LAST + 1];// 44
    char                key[GLFW_KEY_LAST + 1];					// 4c

    // OpenGL extensions and context attributes
    int                 clientAPI;								// 1ac
    int                 glMajor, glMinor, glRevision;			// 1b0
    GLboolean           glForward, glDebug;						// 1b4
    int                 glProfile;								// 1b8
    int                 glRobustness;							// 1bc
    PFNGLGETSTRINGIPROC GetStringi;								// 1c0

	PAD(0x20);

    struct {
        GLFWwindowposfun        pos;		// 1ec
        GLFWwindowsizefun       size;		// 1f0
        GLFWwindowclosefun      close;		// 1f4
        GLFWwindowrefreshfun    refresh;	// 1f8
        GLFWwindowfocusfun      focus;		// 1fc
        GLFWwindowiconifyfun    iconify;	// 200
        GLFWframebuffersizefun  fbsize;		// 204
        GLFWmousebuttonfun      mouseButton;// 208
        GLFWcursorposfun        cursorPos;	// 20c
        GLFWcursorenterfun      cursorEnter;// 210
        GLFWscrollfun           scroll;		// 214
        GLFWkeyfun              key;		// 218
        GLFWcharfun             character;	// 21c
    } callbacks;

	// there's other stuff i'm too lazy to include
};

static CCPoint convertMouseCoords(double x, double y) {
    auto* director = CCDirector::get();
    auto* gl = director->getOpenGLView();
    auto winSize = director->getWinSize();
    auto frameSize = gl->getFrameSize();
    auto mouse = CCPoint { static_cast<float>(x), static_cast<float>(y) } / frameSize;
    return ccp(mouse.x, 1.f - mouse.y) * winSize;
}

#include <Sapphire/modify/CCEGLView.hpp>

static GLFWcursorposfun originalCursorPosFun = nullptr;

void __cdecl glfwPosCallback(GLFWwindow* window, double x, double y) {
	originalCursorPosFun(window, x, y);
    auto event = MouseMoveEvent(Mouse::get()->getCapturingNode(), convertMouseCoords(x, y));
    postMouseEventThroughTouches(
        event,
        (Mouse::get()->isHeld(MouseButton::Left) ?
            CCTOUCHMOVED :
            CCTOUCHOTHER)
    );
}

void setCursorPosCallback(GLFWwindow* window) {
	if (!originalCursorPosFun) {
		originalCursorPosFun = reinterpret_cast<_GLFWwindow*>(window)->callbacks.cursorPos;
		reinterpret_cast<_GLFWwindow*>(window)->callbacks.cursorPos
		= reinterpret_cast<GLFWcursorposfun>(&glfwPosCallback);
	}
}

$on_mod(Loaded) {
	if (CCEGLView::get()->getWindow()) {
		setCursorPosCallback(CCEGLView::get()->getWindow());
	}
};

struct $modify(CCEGLViewModify, CCEGLView) {
	void setupWindow(CCRect size) {
		CCEGLView::setupWindow(size);
		setCursorPosCallback(m_pMainWindow);
	}

	void onGLFWMouseCallBack(GLFWwindow* window, int button, int action, int mods) {
        if (action) {
            Mouse::get()->m_heldButtons.insert(static_cast<MouseButton>(button));
        }
        else {
            Mouse::get()->m_heldButtons.erase(static_cast<MouseButton>(button));
        }
        auto event = MouseClickEvent(
            Mouse::get()->getCapturingNode(),
            static_cast<MouseButton>(button), action,
            getMousePos()
        );
        postMouseEventThroughTouches(
            event,
            (static_cast<MouseButton>(button) == MouseButton::Left ?
                (action ? CCTOUCHBEGAN : CCTOUCHENDED) :
                CCTOUCHOTHER)
        );
	}
};

#endif
