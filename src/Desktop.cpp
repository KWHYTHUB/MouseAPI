#include <Sapphire/DefaultInclude.hpp>

#ifdef SAPPHIRE_IS_DESKTOP
#include "Platform.hpp"
#include "../include/API.hpp"
#include <Sapphire/modify/CCMouseDispatcher.hpp>

using namespace sapphire::prelude;
using namespace mouse;

struct $modify(CCMouseDispatcher) {
	bool dispatchScrollMSG(float y, float x) {
		auto ev = MouseScrollEvent(
			Mouse::get()->getCapturingNode(), y, x,
			getMousePos()
		);
		postMouseEventThroughTouches(ev, CCTOUCHOTHER);
		if (ev.isSwallowed()) {
			return true;
		}
		return CCMouseDispatcher::dispatchScrollMSG(y, x);
	}
};

#endif
