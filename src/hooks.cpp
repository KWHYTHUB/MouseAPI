#include <Sapphire/modify/CCLayer.hpp>
#include <Sapphire/modify/CCMenu.hpp>
#include <Sapphire/modify/CCTouchDispatcher.hpp>
#include <Sapphire/modify/AchievementNotifier.hpp>
#include <Sapphire/modify/CCTextInputNode.hpp>
#include <Sapphire/utils/cocos.hpp>
#include "../include/API.hpp"
#include "Pool.hpp"

using namespace sapphire::prelude;
using namespace mouse;

struct $modify(CCTouchDispatcher) {
    void addStandardDelegate(CCTouchDelegate* delegate, int prio) {
        CCTouchDispatcher::addStandardDelegate(delegate, prio);
        if (auto node = typeinfo_cast<CCNode*>(delegate)) {
            if (node->getEventListener("mouse"_spr)) return;
            node->template addEventListener<MouseEventFilter>(
                "mouse"_spr,
                [=](MouseEvent*) {
                    return MouseResult::Eat;
                }
            );
            Mouse::updateListeners();
        }
    }

    void addTargetedDelegate(CCTouchDelegate* delegate, int prio, bool swallows) {
        CCTouchDispatcher::addTargetedDelegate(delegate, prio, swallows);
        if (auto node = typeinfo_cast<CCNode*>(delegate)) {
            if (node->getEventListener("mouse"_spr)) return;
            if (swallows) {
                node->template addEventListener<MouseEventFilter>(
                    "mouse"_spr,
                    [=](MouseEvent*) {
                        return MouseResult::Swallow;
                    }
                );
            }
            else {
                node->template addEventListener<MouseEventFilter>(
                    "mouse"_spr,
                    [=](MouseEvent*) {
                        return MouseResult::Eat;
                    }
                );
            }
            Mouse::updateListeners();
        }
    }

    void removeDelegate(CCTouchDelegate* delegate) {
        if (auto node = typeinfo_cast<CCNode*>(delegate)) {
            node->removeEventListener("mouse"_spr);
            Mouse::updateListeners();
        }
        CCTouchDispatcher::removeDelegate(delegate);
    }
};

struct $modify(CCTextInputNode) {
    bool ccTouchBegan(CCTouch* touch, CCEvent* event) {
        if (m_delegate && !m_delegate->allowTextInput(this)) {
            return false;
        }
        this->onClickTrackNode(true);
        return true;
    }
};

struct $modify(CCMenu) {
    bool initWithArray(CCArray* items) {
        if (!CCMenu::initWithArray(items))
            return false;

        this->template addEventListener<MouseEventFilter>(
            "mouse"_spr,
            [=](MouseEvent* ev) {
                for (auto& child : CCArrayExt<CCNode>(m_pChildren)) {
                    if (child->boundingBox().containsPoint(this->convertToNodeSpace(ev->getPosition()))) {
                        return MouseResult::Swallow;
                    }
                }
                return MouseResult::Leave;
            },
            true
        );
        Mouse::updateListeners();
        
        return true;
    }
};

struct $modify(AchievementNotifier) {
    void willSwitchToScene(CCScene* scene) {
        AchievementNotifier::willSwitchToScene(scene);
        Mouse::updateListeners();
    }
};
