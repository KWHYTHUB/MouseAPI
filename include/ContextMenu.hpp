#pragma once

#include "API.hpp"
#include <Sapphire/DefaultInclude.hpp>
#include <Sapphire/loader/Dispatch.hpp>
#include <Sapphire/utils/cocos.hpp>

namespace mouse {
    struct ContextMenuStyle {
        float padding = 3.f;
        float height = 15.f;
        float minWidth = 50.f;
        float maxWidth = 150.f;
        float maxHeight = 140.f;
        float itemGap = 0.f;
        std::string fontName = "chatFont.fnt";
        std::string arrowSprite = "sapphire.mouse-api/arrow.png";
        bool flipArrow = false;
        float arrowSize = 5.5f;
        std::string bgSprite = "square02b_small.png";
        bool bgSpriteIsFrame = false;
        std::string hoverSprite = "square02b_small.png";
        cocos2d::ccColor4B bgColor = { 0, 0, 0, 215 };
        cocos2d::ccColor4B textColor = { 255, 255, 255, 255 };
        cocos2d::ccColor4B hoverColor = { 255, 255, 255, 45 };
        cocos2d::ccColor4B arrowColor = { 255, 255, 255, 255 };
    };
}

// Defined inline so mods that depend on mouse-api optionally can convert 
// styles to JSON without needing to link
// Also converting the styles over as JSON ensures ABI compatability
template <>
struct json::Serialize<mouse::ContextMenuStyle> {
    static inline json::Value to_json(mouse::ContextMenuStyle const& style) {
        return json::Object {
            { "padding", style.padding },
            { "height", style.height },
            { "minWidth", style.minWidth },
            { "maxWidth", style.maxWidth },
            { "maxHeight", style.maxHeight },
            { "itemGap", style.itemGap },
            { "fontName", style.fontName },
            { "arrowSprite", style.arrowSprite },
            { "flipArrow", style.flipArrow },
            { "arrowSize", style.arrowSize },
            { "bgSprite", style.bgSprite },
            { "bgSpriteIsFrame", style.bgSpriteIsFrame },
            { "hoverSprite", style.hoverSprite },
            { "bgColor", style.bgColor },
            { "textColor", style.textColor },
            { "hoverColor", style.hoverColor },
            { "arrowColor", style.arrowColor },
        };
    }

    static inline mouse::ContextMenuStyle from_json(json::Value const& json) {
        return mouse::ContextMenuStyle {
            .padding = static_cast<float>(json["padding"].as_double()),
            .height = static_cast<float>(json["height"].as_double()),
            .minWidth = static_cast<float>(json["minWidth"].as_double()),
            .maxWidth = static_cast<float>(json["maxWidth"].as_double()),
            .maxHeight = static_cast<float>(json["maxHeight"].as_double()),
            .itemGap = static_cast<float>(json["itemGap"].as_double()),
            .fontName = json["fontName"].as_string(),
            .arrowSprite = json["arrowSprite"].as_string(),
            .flipArrow = json["flipArrow"].as_bool(),
            .arrowSize = static_cast<float>(json["arrowSize"].as_double()),
            .bgSprite = json["bgSprite"].as_string(),
            .bgSpriteIsFrame = json["bgSpriteIsFrame"].as_bool(),
            .hoverSprite = json["hoverSprite"].as_string(),
            .bgColor = json["bgColor"].template as<cocos2d::ccColor4B>(),
            .textColor = json["textColor"].template as<cocos2d::ccColor4B>(),
            .hoverColor = json["hoverColor"].template as<cocos2d::ccColor4B>(),
            .arrowColor = json["arrowColor"].template as<cocos2d::ccColor4B>(),
        };
    }
};

namespace mouse {
    class ContextMenu;
    class ActionMenuItem;

    using ContextMenuEvent = sapphire::DispatchEvent<cocos2d::CCNode*>;
    using ContextMenuFilter = sapphire::DispatchFilter<cocos2d::CCNode*>;

    using ContextMenuDragEvent = sapphire::DispatchEvent<cocos2d::CCNode*, float>;
    using ContextMenuDragFilter = sapphire::DispatchFilter<cocos2d::CCNode*, float>;
    using ContextMenuDragInitEvent = sapphire::DispatchEvent<cocos2d::CCNode*, float*>;
    using ContextMenuDragInitFilter = sapphire::DispatchFilter<cocos2d::CCNode*, float*>;

    // Defined inline so this can be used without linking
    struct ContextMenuBuilder {
        json::Value result = json::Object {
            { "items", json::Array() },
        };
        
        inline ContextMenuBuilder& setStyle(ContextMenuStyle const& style) {
            this->result["style"] = style;
            return *this;
        }

        inline ContextMenuBuilder& addItem(
            std::string const& text,
            std::string const& callbackID,
            float ratio = 1.f
        ) {
            this->result["items"].as_array().push_back(json::Object {
                { "text", text },
                { "click", callbackID },
                { "ratio", ratio },
            });
            return *this;
        }

        inline ContextMenuBuilder& addItem(
            std::string const& iconFrame,
            std::string const& text,
            std::string const& callbackID,
            float ratio = 1.f
        ) {
            this->result["items"].as_array().push_back(json::Object {
                { "text", text },
                { "frame", iconFrame },
                { "click", callbackID },
                { "ratio", ratio },
            });
            return *this;
        }

        inline ContextMenuBuilder& addSubMenu(
            std::string const& text,
            ContextMenuBuilder& subMenu
        ) {
            this->result["items"].as_array().push_back(json::Object {
                { "text", text },
                { "sub-menu", subMenu },
            });
            return *this;
        }

        inline ContextMenuBuilder& addSubMenu(
            std::string const& iconFrame,
            std::string const& text,
            ContextMenuBuilder& subMenu
        ) {
            this->result["items"].as_array().push_back(json::Object {
                { "text", text },
                { "frame", iconFrame },
                { "sub-menu", subMenu },
            });
            return *this;
        }

        inline operator json::Value() {
            return this->result;
        }
    };

    inline ContextMenuBuilder buildContextMenu() {
        return ContextMenuBuilder();
    }

    class MOUSEAPI_DLL ContextMenuItem : public cocos2d::CCNode {
    protected:
        ContextMenu* m_parentMenu;
        cocos2d::CCNode* m_icon = nullptr;
        cocos2d::CCLabelBMFont* m_label = nullptr;
        cocos2d::extension::CCScale9Sprite* m_hoverBG;
        cocos2d::CCPoint m_lastDrag;
        bool m_dragged = false;
        float m_ratio = 1.f;

        bool init(ContextMenu* menu);

        void draw() override;

        friend class ContextMenu;

    public:
        virtual void setIcon(cocos2d::CCNode* icon);
        virtual void setText(std::string const& text);
        virtual void setRatio(float ratio);

        virtual float getPreferredWidth();
        virtual void fitToWidth(float width);

        virtual bool isHovered();
        virtual void hide();
        virtual void select();
        virtual void drag(float delta);
    };

    using ItemRef = sapphire::Ref<ContextMenuItem>;

    class MOUSEAPI_DLL ActionMenuItem : public ContextMenuItem {
    protected:
        std::string m_eventID;

        bool init(ContextMenu* menu, std::string const& eventID);
    
    public:
        static ActionMenuItem* create(ContextMenu* menu, std::string const& eventID);

        void select() override;
    };

    class MOUSEAPI_DLL DragMenuItem : public ContextMenuItem {
    protected:
        std::string m_text;
        std::string m_eventID;
        float m_value = 0.f;
        float m_rate = 1.f;
        float m_precision = .25f;

        bool init(ContextMenu* menu, std::string const& eventID);

        void updateText();
    
    public:
        static DragMenuItem* create(ContextMenu* menu, std::string const& eventID);

        void setText(std::string const& text) override;

        void setValue(float value);
        void setRate(float rate);
        void setPrecision(float precision);

        void drag(float delta) override;
    };

    class MOUSEAPI_DLL SubMenuItem : public ContextMenuItem {
    protected:
        json::Value m_menuJson;
        ContextMenu* m_menu = nullptr;
        cocos2d::CCSprite* m_arrow;

        bool init(ContextMenu* menu, json::Value const& json);
    
        void draw() override;

    public:
        static SubMenuItem* create(ContextMenu* menu, json::Value const& json);

        float getPreferredWidth() override;
        void fitToWidth(float width) override;

        bool isHovered() override;
        void select() override;
        void hide() override;
    };

    class MOUSEAPI_DLL ContextMenu : public cocos2d::CCNode {
    protected:
        sapphire::Ref<cocos2d::CCNode> m_target;
        std::vector<ItemRef> m_items;
        cocos2d::CCNode* m_container;
        cocos2d::extension::CCScale9Sprite* m_bg;
        ContextMenuStyle m_style;
        ContextMenu* m_parentMenu = nullptr;

        bool init(cocos2d::CCNode* target, json::Value const& json, ContextMenu* parent);

        ActionMenuItem* createError(std::string const& msg);
        void parseItems(json::Value const& value);

    public:
        static ContextMenu* create(
            cocos2d::CCNode* target,
            json::Value const& json,
            ContextMenu* parent = nullptr
        );

        cocos2d::CCNode* getTarget() const;
        ContextMenuStyle const& getStyle() const;
        ContextMenu* getParentMenu() const;
        ContextMenu* getTopMostMenu() const;

        bool isHovered();
        void show(cocos2d::CCPoint const& pos);
        void hide();
    };
}
