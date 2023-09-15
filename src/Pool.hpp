#include "../include/API.hpp"
#include <Sapphire/utils/cocos.hpp>
#include <Sapphire/utils/ranges.hpp>
#include <Sapphire/modify/CCNode.hpp>
#include <Sapphire/modify/CCTouchDispatcher.hpp>
#include <Sapphire/cocos/robtop/glfw/glfw3.h>
#include <json/stl_serialize.hpp>
#include "Platform.hpp"

using namespace prelude;
using namespace mouse;

using MouseListener = EventListener<MouseEventFilter>;

class MouseEventListenerPool : public DefaultEventListenerPool {
protected:
	MouseListener* m_capturing = nullptr;
	std::atomic_bool m_sorting = false;

public:
	bool add(EventListenerProtocol* listener) override {
		if (typeinfo_cast<MouseListener*>(listener)) {
			// log::debug("adding {}", &static_cast<MouseListener*>(listener)->getFilter());
			return DefaultEventListenerPool::add(listener);
		}
		return false;
	}

	void remove(EventListenerProtocol* listener) override  {
		// log::debug("removing {} => {}", &static_cast<MouseListener*>(listener)->getFilter(), listener);
		this->release(static_cast<MouseListener*>(listener));
		DefaultEventListenerPool::remove(listener);
	}

	void sortListeners() {
		// do not allow recursive sorting to happen in any way
		if (m_sorting) {
			return;
		}
		m_sorting = true;
		// log::debug("sortListeners");
		m_locked += 1;
		// log::debug("sorting");
		// sort all mouse listeners to put the nodes closer on the screen at the front
		std::sort(
			m_listeners.begin(),
			m_listeners.end(),
			[](EventListenerProtocol* a, EventListenerProtocol* b) {
				// listeners may be null if they are removed mid-handle iteration
				if (!a || !b) return a > b;
				auto& af = static_cast<MouseListener*>(a)->getFilter();
				auto& bf = static_cast<MouseListener*>(b)->getFilter();
				// if these listeners point to the same target, compare by which 
				// listener was added first
				if (af.getTarget() == bf.getTarget()) {
					return af.getFilterIndex() > bf.getFilterIndex();
				}
				// if one of the listeners is global, that comes first
				if (!af.getTarget()) {
					return true;
				}
				if (!bf.getTarget()) {
					return false;
				}
				// otherwise compare node tree indices, top nodes top bottom nodes
				auto ap = af.getTargetPriority(); 
				auto bp = bf.getTargetPriority();
				for (size_t i = 0; i < ap.size(); i++) {
					if (i < bp.size()) {
						if (ap[i] != bp[i]) {
							return ap[i] > bp[i];
						}
					}
				}
				return ap.size() > bp.size();
			}
		);
		// log::debug("sorting done: {}", m_listeners.size());
		// for (auto a : m_listeners) {
			// if (!a) continue;
			// auto af = static_cast<MouseListener*>(a);
			// log::debug("{}", &af->getFilter());
			// log::debug("{}: {}",
			// 	af->getFilter().getTargetPriority(),
			// 	af->getFilter().getTarget()
			// );
		// }
		m_locked -= 1;
		m_sorting = false;
	}

    std::vector<MouseListener*> getSortedListeners() {
        this->sortListeners();
        std::vector<MouseListener*> res;
        for (auto& l : m_listeners) {
            if (l) {
                res.push_back(static_cast<MouseListener*>(l));
            }
        }
        return res;
    }

    void clear() {
        m_capturing = nullptr;
        for (auto& l : m_listeners) {
            l = nullptr;
        }
    }

	MouseListener* getCapturing() const {
		return m_capturing;
	}

	cocos2d::CCNode* getCapturingNode() const {
		if (m_capturing) {
			if (auto target = m_capturing->getFilter().getTarget()) {
				return target;
			}
		}
		return nullptr;
	}

	void capture(MouseListener* listener) {
		if (!m_capturing) {
			m_capturing = listener;
		}
	}

	void release(MouseListener* listener) {
		if (m_capturing == listener) {
			m_capturing = nullptr;
		}
	}

	static MouseEventListenerPool* get() {
		static auto inst = new MouseEventListenerPool();
		return inst;
	}
};
