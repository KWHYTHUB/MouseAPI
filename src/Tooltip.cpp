#include "../include/Tooltip.hpp"

using namespace sapphire::prelude;
using namespace mouse;

bool Tooltip::init(std::string const& text) {
    if (!CCNode::init())
        return false;
    
    auto label = CCLabelBMFont::create(text.c_str(), "goldFont.fnt");

    auto bg = CCScale9Sprite::create("square02b_small-uhd.png", { 0, 0, 40, 40 });
    bg->setColor({ 0, 0, 0 });
    bg->setOpacity(155);
    bg->setScale(.4f);
    bg->setContentSize(label->getScaledContentSize() + CCSize { 12.f, 12.f });
    bg->setPosition(bg->getScaledContentSize() / 2);
    this->addChild(bg);

    label->setPosition(bg->getContentSize() / 2);
    bg->addChild(label);

    this->setContentSize(bg->getScaledContentSize());
    this->setAnchorPoint({ 0.f, 1.f });
    this->setID("tooltip"_spr);

    return true;
}

Tooltip* Tooltip::create(std::string const& text) {
    auto ret = new Tooltip;
    if (ret && ret->init(text)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

void Tooltip::move(CCPoint const& pos) {
    if (m_pParent) {
        this->setPosition(m_pParent->convertToNodeSpace(pos));
    }
    else {
        this->setPosition(pos);
    }
}

void Tooltip::show(CCPoint const& pos) {
    this->setVisible(true);
    if (!m_pParent) {
        CCScene::get()->addChild(this);
    }
    this->move(pos);
}

void Tooltip::show(CCNode* node) {
    if (node->getParent()) {
        this->show(node->getParent()->convertToWorldSpace(node->getPosition()));
    }
    else {
        this->show(node->getPosition());
    }
}

void Tooltip::hide() {
    this->removeFromParent();
}

$execute {
    new EventListener<AttributeSetFilter>(
        +[](AttributeSetEvent* event) {
            auto node = event->node;
            if (node->getEventListener("tooltip"_spr)) {
                return;
            }
            node->template addEventListener<MouseEventFilter>(
                "tooltip"_spr,
                [=](MouseEvent* event) {
                    auto tip = static_cast<Tooltip*>(CCScene::get()->getChildByID("tooltip"_spr));
                    if (MouseAttributes::from(node)->isHovered()) {
                        if (tip) {
                            tip->move(event->getPosition() + ccp(5.f, 0.));
                        }
                        else {
                            if (auto value = node->template getAttribute<std::string>("tooltip"_spr)) {
                                Tooltip::create(value.value())->show(event->getPosition() + ccp(5.f, 0.));
                            }
                        }
                    }
                    else if (tip) {
                        tip->hide();
                    }
                    return MouseResult::Eat;
                }
            );
        },
        AttributeSetFilter("tooltip"_spr)
    );
}
