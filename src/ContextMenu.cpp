#include "../include/ContextMenu.hpp"

using namespace sapphire::prelude;
using namespace mouse;

bool ContextMenuItem::init(ContextMenu* menu) {
    if (!CCNode::init())
        return false;

    m_parentMenu = menu;
    m_hoverBG = CCScale9Sprite::create(menu->getStyle().hoverSprite.c_str());
    m_hoverBG->setOpacity(0);
    m_hoverBG->setScale(.4f);
    m_hoverBG->setColor(to3B(m_parentMenu->getStyle().hoverColor));
    this->addChild(m_hoverBG);

    this->template addEventListener<MouseEventFilter>([this](MouseEvent* event) {
        if (auto click = typeinfo_cast<MouseClickEvent*>(event)) {
            m_lastDrag = event->getPosition();
            if (click->getButton() == MouseButton::Left && !click->isDown()) {
                this->select();
            }
        }
        m_dragged = typeinfo_cast<MouseMoveEvent*>(event);
        if (m_dragged && MouseAttributes::from(this)->isHeld(MouseButton::Left)) {
            this->drag(event->getPosition().y - m_lastDrag.y);
            m_lastDrag = event->getPosition();
        }
        if (auto scroll = typeinfo_cast<MouseScrollEvent*>(event)) {
            this->drag(-scroll->getDeltaY());
        }
        return MouseResult::Swallow;
    });

    return true;
}

float ContextMenuItem::getPreferredWidth() {
    auto const& style = m_parentMenu->getStyle();
    float width = style.padding * 2.f;
    width += style.height; // icon
    if (m_label) {
        limitNodeSize(m_label, {
            style.maxWidth,
            style.height - style.padding * 2
        }, 1.f, .1f);
        width += m_label->getScaledContentSize().width;
    }
    width += style.height;
    return width / m_ratio;
}

void ContextMenuItem::fitToWidth(float width) {
    auto const& style = m_parentMenu->getStyle();
    this->setContentSize({ width, style.height });
    m_hoverBG->setContentSize(
        (m_obContentSize - ccp(style.padding, style.padding)) /
            m_hoverBG->getScale()
    );
    m_hoverBG->setPosition(m_obContentSize / 2);
    if (m_label) {
        limitNodeSize(m_label, {
            width - style.height * 2,
            style.height - style.padding * 2
        }, 1.f, .1f);
    }
}

void ContextMenuItem::setIcon(CCNode* icon) {
    if (m_icon) {
        m_icon->removeFromParent();
    }
    m_icon = icon;
    if (m_icon) {
        auto const& style = m_parentMenu->getStyle();
        m_icon->setPosition(style.height / 2, style.height / 2);
        this->addChild(m_icon);
        limitNodeSize(m_icon, {
            style.height - style.padding * 2,
            style.height - style.padding * 2
        }, 1.f, .1f);
    }
}

void ContextMenuItem::setText(std::string const& text) {
    if (m_label) {
        m_label->setString(text.c_str());
    } else {
        auto const& style = m_parentMenu->getStyle();
        m_label = CCLabelBMFont::create(
            text.c_str(), style.fontName.c_str()
        );
        m_label->setAnchorPoint({ .0f, .5f });
        m_label->setColor(to3B(style.textColor));
        m_label->setOpacity(style.textColor.a);
        m_label->setPosition(style.height, style.height / 2);
        this->addChild(m_label);
    }
}

void ContextMenuItem::setRatio(float ratio) {
    m_ratio = ratio;
}

void ContextMenuItem::draw() {
    auto const& style = m_parentMenu->getStyle();
    ccGLBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (this->isHovered()) {
        m_hoverBG->setOpacity(style.hoverColor.a);
    }
    else {
        m_hoverBG->setOpacity(0);
    }
    CCNode::draw();
}

bool ContextMenuItem::isHovered() {
    return m_dragged || MouseAttributes::from(this)->isHovered();
}

void ContextMenuItem::hide() {}
void ContextMenuItem::select() {}
void ContextMenuItem::drag(float) {}

bool ActionMenuItem::init(ContextMenu* menu, std::string const& eventID) {
    if (!ContextMenuItem::init(menu))
        return false;

    m_eventID = eventID;

    return true;
}

ActionMenuItem* ActionMenuItem::create(ContextMenu* menu, std::string const& eventID) {
    auto ret = new ActionMenuItem();
    if (ret && ret->init(menu, eventID)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

void ActionMenuItem::select() {
    ContextMenuEvent(m_eventID, m_parentMenu->getTarget()).post();
    m_parentMenu->getTopMostMenu()->hide();
}

bool DragMenuItem::init(ContextMenu* menu, std::string const& eventID) {
    if (!ContextMenuItem::init(menu))
        return false;

    m_eventID = eventID;
    ContextMenuDragInitEvent(eventID, menu->getTarget(), &m_value).post();

    return true;
}

DragMenuItem* DragMenuItem::create(ContextMenu* menu, std::string const& eventID) {
    auto ret = new DragMenuItem();
    if (ret && ret->init(menu, eventID)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

void DragMenuItem::updateText() {
    if (!m_label) {
        this->setText("");
    }
    if (m_text.size()) {
        m_label->setString(fmt::format("{}: {}", m_text, m_value).c_str());
    }
    else {
        m_label->setString(fmt::format("{}", m_value).c_str());
    }
}

void DragMenuItem::setText(std::string const& text) {
    m_text = text;
    ContextMenuItem::setText(text);
    this->updateText();
}

void DragMenuItem::setValue(float value) {
    m_value = value;
    this->updateText();
}

void DragMenuItem::setRate(float rate) {
    m_rate = rate;
}

void DragMenuItem::setPrecision(float precision) {
    m_precision = precision;
}

void DragMenuItem::drag(float delta) {
    if (m_precision > .000001f) {
        m_value = roundf((m_value + delta * m_rate) / m_precision) * m_precision;
    }
    else {
        m_value += delta * m_rate;
    }
    this->updateText();
    ContextMenuDragEvent(m_eventID, m_parentMenu->getTarget(), m_value).post();
}

bool SubMenuItem::init(ContextMenu* menu, json::Value const& json) {
    if (!ContextMenuItem::init(menu))
        return false;

    auto const& style = m_parentMenu->getStyle();

    m_menuJson = json;
    m_arrow = CCSprite::createWithSpriteFrameName(style.arrowSprite.c_str());
    m_arrow->setFlipX(style.flipArrow);
    m_arrow->setColor(to3B(style.arrowColor));
    m_arrow->setOpacity(style.arrowColor.a);
    limitNodeSize(m_arrow, { style.arrowSize, style.arrowSize }, 1.f, .1f);
    this->addChild(m_arrow);

    return true;
}

SubMenuItem* SubMenuItem::create(ContextMenu* menu, json::Value const& json) {
    auto ret = new SubMenuItem;
    if (ret && ret->init(menu, json)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

float SubMenuItem::getPreferredWidth() {
    return ContextMenuItem::getPreferredWidth();
}

void SubMenuItem::fitToWidth(float width) {
    ContextMenuItem::fitToWidth(width);
    auto const& style = m_parentMenu->getStyle();
    m_arrow->setPosition({ width - style.height / 2, style.height / 2 });
}

void SubMenuItem::draw() {
    ContextMenuItem::draw();
    if (this->isHovered()) {
        this->select();
    }
    else if (m_menu) {
        m_menu->hide();
        m_menu = nullptr;
    }
}

void SubMenuItem::select() {
    if (!m_menu && m_parentMenu) {
        auto bbox = this->boundingBox();
        if (m_pParent) {
            bbox.origin = m_pParent->convertToWorldSpace(bbox.origin);
        }
        m_menu = ContextMenu::create(m_parentMenu->getTarget(), m_menuJson, m_parentMenu);
        m_menu->show({ bbox.getMaxX(), bbox.getMaxY() });
    }
}

bool SubMenuItem::isHovered() {
    return MouseAttributes::from(this)->isHovered() || 
        (m_menu ? m_menu->isHovered() : false);
}

void SubMenuItem::hide() {
    if (m_menu) {
        m_menu->hide();
        m_menu = nullptr;
    }
    m_parentMenu = nullptr;
}

bool ContextMenu::init(CCNode* target, json::Value const& json, ContextMenu* parent) {
    if (!CCNode::init())
        return false;
    
    m_target = target;
    m_parentMenu = parent ? parent : this;

    json::Value items;
    if (json.is_object()) {
        try {
            m_style = json["style"].template as<ContextMenuStyle>();
        } catch(...) {}

        try {
            items = json["items"];
        } catch(...) {
            items = json::Array {
                json::Object {
                    { "text", "Missing \"items\"" },
                    { "click", "" },
                }
            };
        }
    }
    else {
        items = json;
    }

    if (m_style.bgSpriteIsFrame) {
        m_bg = CCScale9Sprite::createWithSpriteFrameName(m_style.bgSprite.c_str());
    }
    else {
        m_bg = CCScale9Sprite::create(m_style.bgSprite.c_str());
    }
    m_bg->setColor(to3B(m_style.bgColor));
    m_bg->setOpacity(m_style.bgColor.a);
    m_bg->setScale(.4f);
    this->addChild(m_bg);

    m_container = CCNode::create();
    m_container->setLayout(
        RowLayout::create()
            ->setGrowCrossAxis(true)
            ->setAxisAlignment(AxisAlignment::Even)
            ->setGap(m_style.itemGap)
    );
    this->addChild(m_container);

    this->parseItems(items);
    this->setAnchorPoint({ 0.f, 1.f });

    return true;
}

ContextMenu* ContextMenu::create(CCNode* target, json::Value const& json, ContextMenu* parent) {
    auto ret = new ContextMenu;
    if (ret && ret->init(target, json, parent)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

ActionMenuItem* ContextMenu::createError(std::string const& msg) {
    auto item = ActionMenuItem::create(this, "");
    item->setText(msg);
    return item;
}

void ContextMenu::parseItems(json::Value const& value) {
    std::vector<ItemRef> items;

    if (!value.is_array()) {
        items = { this->createError("Context menu is not an array") };
    }
    else for (auto val : value.as_array()) {
        if (!val.is_object()) {
            items.push_back(this->createError("Item is not an object"));
            continue;
        }
        auto obj = val.as_object();
        bool hasTextOrIcon = false;
        ContextMenuItem* item;
        if (obj.count("sub-menu")) {
            item = SubMenuItem::create(this, obj["sub-menu"]);
        }
        else if (obj.count("click")) {
            try {
                item = ActionMenuItem::create(this, obj["click"].as_string());
            } catch(...) {
                items.push_back(this->createError("Invalid \"click\""));
                continue;
            }
        }
        else if (obj.count("drag")) {
            try {
                item = DragMenuItem::create(this, obj["drag"].as_string());
                if (obj.count("value")) {
                    static_cast<DragMenuItem*>(item)->setValue(
                        static_cast<float>(obj["value"].as_double())
                    );
                }
                if (obj.count("rate")) {
                    static_cast<DragMenuItem*>(item)->setRate(
                        static_cast<float>(obj["rate"].as_double())
                    );
                }
                if (obj.count("precision")) {
                    static_cast<DragMenuItem*>(item)->setPrecision(
                        static_cast<float>(obj["precision"].as_double())
                    );
                }
            } catch(...) {
                items.push_back(this->createError("Invalid \"drag\", \"value\", or \"rate\""));
                continue;
            }
        }
        else {
            items.push_back(this->createError("Missing \"click\" or \"sub-menu\""));
            continue;
        }
        if (obj.count("text")) {
            try {
                item->setText(obj["text"].as_string());
                hasTextOrIcon = true;
            } catch(...) {
                items.push_back(this->createError("Invalid \"text\""));
                continue;
            }
        }
        if (obj.count("frame")) {
            try {
                auto spr = obj["frame"].as_string();
                auto frame = CCSprite::createWithSpriteFrameName(spr.c_str());
                if (!frame) {
                    items.push_back(this->createError(fmt::format(
                        "No sprite frame \"{}\"", spr
                    )));
                    continue;
                }
                item->setIcon(frame);
                hasTextOrIcon = true;
            } catch(...) {
                items.push_back(this->createError("Invalid \"frame\""));
                continue;
            }
        }
        if (obj.count("ratio")) {
            try {
                item->setRatio(static_cast<float>(obj["ratio"].as_double()));
            } catch(...) {
                items.push_back(this->createError("Invalid \"ratio\""));
                continue;
            }
        }
        items.push_back(item);
    }

    m_items = items;

    float width = 0.f;
    for (auto& item : items) {
        auto w = item->getPreferredWidth();
        if (w > width) {
            width = w;
        }
    }
    width = clamp(width, m_style.minWidth, m_style.maxWidth);

    for (auto& item : items) {
        m_container->addChild(item);
        item->fitToWidth(width * item->m_ratio);
        item->updateLayout();
    }
    auto containerHeight = static_cast<RowLayout*>(m_container->getLayout())
        ->getSizeHint(m_container).height;
    
    m_container->setContentSize({ width, containerHeight });
    m_container->updateLayout();

    auto height = clamp(m_container->getScaledContentSize().height, 0.f, m_style.maxHeight);
    this->setContentSize({ width, height });
    m_bg->setContentSize(m_obContentSize / m_bg->getScale());
    m_bg->setPosition(m_obContentSize / 2);
}

CCNode* ContextMenu::getTarget() const {
    return m_target;
}

ContextMenuStyle const& ContextMenu::getStyle() const {
    return m_style;
}

ContextMenu* ContextMenu::getParentMenu() const {
    return m_parentMenu;
}

ContextMenu* ContextMenu::getTopMostMenu() const {
    if (m_parentMenu == this) {
        return m_parentMenu;
    }
    return m_parentMenu->getParentMenu();
}

bool ContextMenu::isHovered() {
    if (MouseAttributes::from(this)->isHovered()) {
        return true;
    }
    for (auto& item : m_items) {
        if (item->isHovered()) {
            return true;
        }
    }
    return false;
}

void ContextMenu::show(CCPoint const& pos) {
    if (!m_pParent) {
        CCScene::get()->addChild(this);
    }
    this->setZOrder(CCScene::get()->getHighestChildZ() + 100);

    auto winSize = CCDirector::get()->getWinSize();
    auto clampedPos = pos;
    while (clampedPos.x + this->getScaledContentSize().width > winSize.width) {
        clampedPos.x -= this->getScaledContentSize().width;
    }
    while (clampedPos.y - this->getScaledContentSize().height < 0.f) {
        clampedPos.y += this->getScaledContentSize().height;
    }
    if (
        (clampedPos.x != pos.x && m_parentMenu != this) ||
        m_parentMenu->getPositionX() + m_parentMenu->getScaledContentSize().width * 2 > winSize.width
    ) {
        if (clampedPos.x == pos.x) {
            clampedPos.x -= this->getScaledContentSize().width;
        }
        clampedPos.x -= m_parentMenu->getScaledContentSize().width;
    }
    this->setPosition(this->getParent()->convertToNodeSpace(clampedPos));
}

void ContextMenu::hide() {
    for (auto& item : m_items) {
        item->hide();
    }
    this->removeFromParent();
}

$execute {
    new EventListener<AttributeSetFilter>(
        +[](AttributeSetEvent* event) {
            auto node = event->node;
            auto value = event->value;
            if (node->getEventListener("context-menu"_spr)) {
                return;
            }
            node->template addEventListener<MouseEventFilter>(
                "context-menu"_spr,
                [=](MouseEvent* event) {
                    auto click = typeinfo_cast<MouseClickEvent*>(event);
                    if (
                        !click || !click->isDown() ||
                        click->getButton() != MouseButton::Right
                    ) {
                        return MouseResult::Leave;
                    }
                    auto menu = ContextMenu::create(node, value);
                    menu->setID("context-menu"_spr);
                    menu->show(event->getPosition());
                    return MouseResult::Swallow;
                }
            );
            Mouse::updateListeners();
        },
        AttributeSetFilter("context-menu"_spr)
    );
    // close all context menus when clicked anywhere
    new EventListener<MouseEventFilter>(
        +[](MouseEvent* event) {
            auto click = typeinfo_cast<MouseClickEvent*>(event);
            if (!click || !click->isDown()) {
                return MouseResult::Leave;
            }
            while (auto old = static_cast<ContextMenu*>(
                CCScene::get()->getChildByID("context-menu"_spr)
            )) {
                old->setID("context-menu-checked"_spr);
                if (!old->getTopMostMenu()->isHovered()) {
                    old->hide();
                }
            }
            // reset IDs of unhidden menus
            while (auto old = static_cast<ContextMenu*>(
                CCScene::get()->getChildByID("context-menu-checked"_spr)
            )) {
                old->setID("context-menu"_spr);
            }
            return MouseResult::Leave;
        },
        MouseEventFilter(nullptr)
    );
}
