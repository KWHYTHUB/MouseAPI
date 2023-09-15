#pragma once

#include "API.hpp"
#include <Sapphire/DefaultInclude.hpp>

namespace mouse {
    class MOUSEAPI_DLL Tooltip : public cocos2d::CCNode {
    protected:
        bool init(std::string const& text);
    
    public:
        static Tooltip* create(std::string const& text);

        void move(cocos2d::CCPoint const& pos);
        void show(cocos2d::CCPoint const& pos);
        void show(cocos2d::CCNode* node);
        void hide();
    };
}
