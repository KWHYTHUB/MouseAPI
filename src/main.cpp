#include "../include/API.hpp"
#include <Sapphire/utils/cocos.hpp>
#include <Sapphire/utils/ranges.hpp>
#include <Sapphire/modify/CCNode.hpp>
#include <Sapphire/modify/CCTouchDispatcher.hpp>
#include <Sapphire/cocos/robtop/glfw/glfw3.h>
#include <json/stl_serialize.hpp>
#include "Platform.hpp"
#include "Pool.hpp"

json::Value json::Serialize<MouseButton>::to_json(MouseButton const& button) {
	return static_cast<int>(button);
}

MouseButton json::Serialize<MouseButton>::from_json(json::Value const& json) {
	return static_cast<MouseButton>(json.as_int());
}

using List = std::unordered_set<MouseButton>;

MouseAttributes* MouseAttributes::from(CCNode* node) {
	auto attrs = new MouseAttributes();
	attrs->m_node = node;
	attrs->autorelease();
	return attrs;
}

bool MouseAttributes::isHeld(MouseButton button) const {
	if (auto list = m_node->template getAttribute<List>("held"_spr)) {
		return list.value().contains(button);
	}
	return false;
}

bool MouseAttributes::isHovered() const {
	return m_node->template getAttribute<bool>("hovered"_spr).value_or(false);
}

void MouseAttributes::addHeld(MouseButton button) {
	auto list = m_node->template getAttribute<List>("held"_spr).value_or(List {});
	list.insert(button);
	m_node->setAttribute("held"_spr, list);
}

void MouseAttributes::removeHeld(MouseButton button) {
	auto list = m_node->template getAttribute<List>("held"_spr).value_or(List {});
	list.erase(button);
	m_node->setAttribute("held"_spr, list);
}

void MouseAttributes::clearHeld() {
	m_node->setAttribute("held"_spr, List {});
}

void MouseAttributes::setHovered(bool hovered) {
	m_node->setAttribute("hovered"_spr, hovered);
}

MouseEvent::MouseEvent(CCNode* target, CCPoint const& position)
  : m_target(target),
	m_position(position) {}

void MouseEvent::swallow() {
	m_swallow = true;
}

bool MouseEvent::isSwallowed() const {
	return m_swallow;
}

CCPoint MouseEvent::getPosition() const {
	return m_position;
}

CCNode* MouseEvent::getTarget() const {
	return m_target;
}

CCTouch* MouseEvent::createTouch() const {
	auto touch = new CCTouch();
	// Never call autorelease on CCTouch. If you do it, at some point in the 
	// future CCSequence::update will crash.
	touch->m_point = CCDirector::get()->convertToUI(m_position);
	touch->m_prevPoint = (touch->m_startPoint = touch->m_point);
	return touch;
}

CCEvent* MouseEvent::createEvent() const {
	auto event = new CCEvent();
	event->autorelease();
	return event;
}

void MouseEvent::updateTouch(CCTouch* touch) const {
	touch->m_prevPoint = touch->m_point;
	touch->m_point = CCDirector::get()->convertToUI(m_position);
}

EventListenerPool* MouseEvent::getPool() const {
	return MouseEventListenerPool::get();
}

MouseClickEvent::MouseClickEvent(
	CCNode* target, MouseButton button,
	bool down, CCPoint const& pos
) : MouseEvent(target, pos), m_button(button), m_down(down) {}

MouseClickEvent::MouseClickEvent(
	MouseButton button, bool down, CCPoint const& pos
) : MouseClickEvent(nullptr, button, down, pos) {}

void MouseClickEvent::dispatchDefault(CCNode* target, CCTouch* touch) const {
	if (touch && m_button == MouseButton::Left) {
		if (auto delegate = typeinfo_cast<CCTouchDelegate*>(target)) {
			if (m_down) {
				delegate->ccTouchBegan(touch, this->createEvent());
			}
			else {
				delegate->ccTouchEnded(touch, this->createEvent());
			}
		}
	}
}

MouseButton MouseClickEvent::getButton() const {
	return m_button;
}

bool MouseClickEvent::isDown() const {
	return m_down;
}

MouseMoveEvent::MouseMoveEvent(CCPoint const& pos)
  : MouseMoveEvent(nullptr, pos) {}

MouseMoveEvent::MouseMoveEvent(CCNode* target, CCPoint const& pos)
  : MouseEvent(target, pos) {}

void MouseMoveEvent::dispatchDefault(CCNode* target, CCTouch* touch) const {
	if (!touch) return;
	if (auto delegate = typeinfo_cast<CCTouchDelegate*>(target)) {
		if (MouseAttributes::from(target)->isHeld(MouseButton::Left)) {
			delegate->ccTouchMoved(touch, this->createEvent());
		}
	}
}

MouseScrollEvent::MouseScrollEvent(float deltaY, float deltaX, CCPoint const& pos)
  : MouseScrollEvent(nullptr, deltaY, deltaX, pos) {}

MouseScrollEvent::MouseScrollEvent(
	CCNode* target, float deltaY, float deltaX, CCPoint const& pos
) : MouseEvent(target, pos), m_deltaY(deltaY), m_deltaX(deltaX) {}

void MouseScrollEvent::dispatchDefault(CCNode* target, CCTouch*) const {
	if (auto delegate = typeinfo_cast<CCMouseDelegate*>(target)) {
		delegate->scrollWheel(m_deltaY, m_deltaY);
	}
}

float MouseScrollEvent::getDeltaY() const {
	return m_deltaY;
}

float MouseScrollEvent::getDeltaX() const {
	return m_deltaX;
}

MouseHoverEvent::MouseHoverEvent(CCNode* target, bool enter, CCPoint const& pos)
  : MouseEvent(target, pos), m_enter(enter)
{}

void MouseHoverEvent::dispatchDefault(CCNode*, CCTouch*) const {}

bool MouseHoverEvent::isEnter() const {
	return m_enter;
}

bool MouseHoverEvent::isLeave() const {
	return !m_enter;
}

ListenerResult MouseEventFilter::handle(MiniFunction<Callback> fn, MouseEvent* event) {
	if (m_target) {
		// Make sure to get a Ref to the target so if it gets released during 
		// the execution of this function for example due to ccTouchEnded then 
		// it doesn't get freed yet and cause MouseEventFilter to get destroyed
		Ref target { m_target };
		if (!target || !nodeIsVisible(target) || !target->hasAncestor(nullptr)) {
			return ListenerResult::Propagate;
		}
		// Events will only be dispatched to nodes in the scene that are visible
		bool inside =
			m_ignorePosition ||
			target->boundingBox().containsPoint(
				target->getParent()->convertToNodeSpace(event->getPosition())
			);
		
		bool capturing = !Mouse::get()->getCapturing() ||
			Mouse::get()->getCapturing() == this->getListener();
	 	
		// Events will only be dispatched to the target under the 
		// following conditions: 
		// 1. The target is capturing the mouse
		// 2. The event is meant for the target node specifically
		// 3. When the event is dispatched, if no other target has yet 
		// captured the mouse, another target can "eat" it, in other words 
		// registering themselves as a "weak" target that will still 
		// receive events
		// 4. Nothing is capturing the mouse (event is "edible")
		if (
			this->getListener() == Mouse::get()->getCapturing() ||
			!event->isSwallowed() && (
				// Is this the node the event was meant for?
				event->getTarget() == target ||
				// Is this node eating events?
				m_eaten.data() ||
				// Is this event inside the node and edible?
				(!event->getTarget() && inside)
			)
		) {
			auto attrs = MouseAttributes::from(target);
			// Post hover event
			if (capturing && !attrs->isHovered() && inside) {
				attrs->setHovered(true);
				MouseHoverEvent(target, true, event->getPosition()).post();
			}
			else if (capturing && attrs->isHovered() && !inside) {
				attrs->setHovered(false);
				MouseHoverEvent(target, false, event->getPosition()).post();
			}
			// Add click to held list (may be something the callback needs 
			// to know, so needs to be set before it's called)
			auto click = typeinfo_cast<MouseClickEvent*>(event);
			if (click) {
				if (click->isDown()) {
					attrs->addHeld(click->getButton());
				}
				else {
					attrs->removeHeld(click->getButton());
				}
			}
			auto s = fn(event);
			if (s == MouseResult::Leave) {
				// If the callback didn't want to capture the mouse, release 
				// the hold attribute immediately
				if (click) {
					attrs->removeHeld(click->getButton());
				}
			}
			else {
				// Eat if clicked
				if (click && click->isDown()) {
					m_eaten = event->createTouch();
				}
			}
			if (m_eaten) {
				event->updateTouch(m_eaten);
			}
			event->dispatchDefault(target, m_eaten);
			// Release eaten only after dispatching the touch event so the 
			// touch event can still access the touch
			if (s != MouseResult::Leave && click && !click->isDown()) {
				m_eaten = nullptr;
			}
			// If the callback wants to swallow, or has captured the mouse, 
			// capture the mouse and stop propagation here
			if (
				s == MouseResult::Swallow ||
				event->getTarget() == target ||
				this->getListener() == Mouse::get()->getCapturing()
			) {
				event->swallow();
				if (click) {
					if (click->isDown()) {
						Mouse::get()->capture(static_cast<MouseListener*>(this->getListener()));
					}
					else {
						Mouse::get()->release(static_cast<MouseListener*>(this->getListener()));
					}
				}
				// keep propagating so every remaining listener gets their 
				// hover event posted
				return ListenerResult::Propagate;
			}
			return ListenerResult::Propagate;
		}
		// If this target doesn't get the event, propagate onwards
		else {
			auto attrs = MouseAttributes::from(target);
			// Post hover leave event if necessary
			if (attrs->isHovered() && !inside) {
				attrs->setHovered(false);
				MouseHoverEvent(target, false, event->getPosition()).post();
			}
			attrs->clearHeld();
			m_eaten = nullptr;
			return ListenerResult::Propagate;
		}
	}
	// Otherwise handle global listeners, which will only be fired if no node 
	// is capturing the mouse
	else if (!event->getTarget()) {
		auto s = fn(event);
		if (s == MouseResult::Swallow) {
			event->swallow();
			return ListenerResult::Propagate;
		}
		return ListenerResult::Propagate;
	}
	// Otherwise keep going and find the capturing target
	else {
		return ListenerResult::Propagate;
	}
}

CCNode* MouseEventFilter::getTarget() const {
	return m_target;
}

std::vector<int> MouseEventFilter::getTargetPriority() const {
	if (!m_target) return {};
	auto node = Ref(m_target);
	std::vector<int> tree {};
	while (auto parent = node->getParent()) {
		tree.insert(tree.begin(), parent->getChildren()->indexOfObject(node));
		node = parent;
	}
	return tree;
}

EventListenerPool* MouseEventFilter::getPool() const {
	return MouseEventListenerPool::get();
}

size_t MouseEventFilter::getFilterIndex() const {
	return m_filterIndex;
}

MouseEventFilter::MouseEventFilter(CCNode* target, bool ignorePosition)
  : m_target(target),
  	m_ignorePosition(ignorePosition),
  	m_filterIndex(target ? target->getEventListenerCount() : 0)
{}

MouseEventFilter::~MouseEventFilter() {}

void Mouse::updateListeners() {
	if (s_updating) return;
	s_updating = true;
	// update only once per frame at most
	Loader::get()->queueInGDThread([]() {
		MouseEventListenerPool::get()->sortListeners();
		s_updating = false;
	});
}

Mouse* Mouse::get() {
	static auto inst = new Mouse;
	return inst;
}

bool Mouse::isHeld(MouseButton button) const {
	return m_heldButtons.contains(button);
}

EventListener<MouseEventFilter>* Mouse::getCapturing() const {
	return MouseEventListenerPool::get()->getCapturing();
}

CCNode* Mouse::getCapturingNode() const {
	return MouseEventListenerPool::get()->getCapturingNode();
}

void Mouse::capture(EventListener<MouseEventFilter>* listener) {
	return MouseEventListenerPool::get()->capture(listener);
}

void Mouse::release(EventListener<MouseEventFilter>* listener) {
	return MouseEventListenerPool::get()->release(listener);
}

struct MouseEventContainer : public CCEvent {
	MouseEvent& event;
	MouseEventContainer(MouseEvent& event) : event(event) {
		this->autorelease();
	}
};

void postMouseEventThroughTouches(MouseEvent& event, ccTouchType action) {
	auto set = CCSet::create();
	set->addObject(event.createTouch());
	CCTouchDispatcher::get()->touches(set, new MouseEventContainer(event), action);
	set->release();
}

struct $modify(CCTouchDispatcherModify, CCTouchDispatcher) {
	static void onModify(auto& self) {
		(void)self.setHookPriority("CCTouchDispatcher::touches", 1000);
	}

	void touches(CCSet* set, CCEvent* event, unsigned int type) {
		if (auto me = typeinfo_cast<MouseEventContainer*>(event)) {
			// Update event position in case some touches hook changed it
			me->event.m_position = static_cast<CCTouch*>(set->anyObject())->getLocation();
			me->event.post();
		}
		else {
			CCTouchDispatcher::touches(set, event, type);
		}
	}
};
