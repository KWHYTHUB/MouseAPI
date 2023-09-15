#include <Sapphire/modify/MenuLayer.hpp>
#include "../include/API.hpp"
#include "../include/ContextMenu.hpp"

using namespace sapphire::prelude;
using namespace mouse;

// #define MOUSEAPI_TEST

#ifdef MOUSEAPI_TEST

class HoveredNode : public CCNode {
protected:
    CCLabelBMFont* m_label;

    bool init() {
        if (!CCNode::init())
            return false;
        
        this->setContentSize({ 100.f, 50.f });

        m_label = CCLabelBMFont::create("Hi", "bigFont.fnt");
        m_label->setPosition(m_obContentSize / 2);
        this->addChild(m_label);

        this->template addEventListener<MouseEventFilter>([=](MouseEvent* event) {
            if (MouseAttributes::from(this)->isHeld(MouseButton::Right)) {
                m_label->setString("Right-clicked!");
            }
            else if (MouseAttributes::from(this)->isHovered()) {
                m_label->setString("Hovered!");
            }
            else {
                m_label->setString("Hi");
            }
            return MouseResult::Swallow;
        });

        return true;
    }

public:
    static HoveredNode* create() {
        auto ret = new HoveredNode();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

$execute {
    new EventListener<ContextMenuFilter>(+[](CCNode*) {
        FLAlertLayer::create("Hiii", "Yay it works", "OK")->show();
        return ListenerResult::Propagate;
    }, ContextMenuFilter("my-event-id"_spr));

    new EventListener<ContextMenuFilter>(+[](CCNode* target) {
        target->runAction(CCSequence::create(
            CCEaseElasticOut::create(CCScaleBy::create(.25f, 1.25f), 2.f),
            CCEaseElasticIn::create(CCScaleBy::create(.25f, 1 / 1.25f), 2.f),
            nullptr
        ));
        return ListenerResult::Propagate;
    }, ContextMenuFilter("my-bounce"_spr));

    new EventListener<ContextMenuFilter>(+[](CCNode*) {
        AppDelegate::get()->trySaveGame();
        exit(0);
        return ListenerResult::Propagate;
    }, ContextMenuFilter("quit-game"_spr));

    new EventListener<ContextMenuDragFilter>(+[](CCNode* target, float value) {
        target->setPositionY(value);
        return ListenerResult::Propagate;
    }, ContextMenuDragFilter("node-y"_spr));

    new EventListener<ContextMenuDragInitFilter>(+[](CCNode* target, float* value) {
        *value = target->getPositionY();
        return ListenerResult::Stop;
    }, ContextMenuDragInitFilter("node-y"_spr));

    new EventListener<ContextMenuDragFilter>(+[](CCNode* target, float value) {
        target->setPositionX(value);
        return ListenerResult::Propagate;
    }, ContextMenuDragFilter("node-x"_spr));

    new EventListener<ContextMenuDragInitFilter>(+[](CCNode* target, float* value) {
        *value = target->getPositionX();
        return ListenerResult::Stop;
    }, ContextMenuDragInitFilter("node-x"_spr));
};

struct $modify(MenuLayer) {
    bool init() {
        if (!MenuLayer::init())
            return false;
        
        this->getChildByID("main-title")
            ->setAttribute("sapphire.mouse-api/tooltip", "Omg tooltips");
        
        this->getChildByID("profile-menu")
            ->setAttribute("sapphire.mouse-api/context-menu",
                buildContextMenu()
                    .addItem("Hiii", "my-event-id"_spr)
                    .addItem("Ba", "my-event-id"_spr, .5f)
                    .addItem("Bo", "my-event-id"_spr, .5f)
                    .addSubMenu("Test", buildContextMenu()
                        .addItem("Yahoo!", "my-bounce"_spr)
                        .addItem("Fantastic", "__invalid")
                    )
            );

        this->getChildByID("main-menu")
            ->setAttribute("sapphire.mouse-api/context-menu",
                json::Array {
                    json::Object {
                        { "text", "Click to Work?" },
                        { "frame", "GJ_infoIcon_001.png" },
                        { "click", "my-event-id"_spr },
                    },
                    json::Object {
                        { "text", "Bounce Animation!" },
                        { "click", "my-bounce"_spr },
                    },
                    json::Object {
                        { "text", "X" },
                        { "precision", 1.0 },
                        { "drag", "node-x"_spr },
                        { "ratio", .5 },
                    },
                    json::Object {
                        { "text", "Y" },
                        { "precision", 1.0 },
                        { "drag", "node-y"_spr },
                        { "ratio", .5 },
                    },
                    json::Object {
                        { "text", "Sub menu" },
                        { "sub-menu", json::Array {
                            json::Object {
                                { "text", "Sub menu!!" },
                                { "click", "my-event-id"_spr },
                            }
                        } },
                    },
                    json::Object {
                        { "text", "Quit Game" },
                        { "click", "quit-game"_spr },
                    },
                }
            );
        
        auto node = HoveredNode::create();
        node->setPosition(60, 80);
        node->setZOrder(15);
        this->addChild(node);

        return true;
    }
};

#endif
