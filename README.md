# MouseAPI

API mod for handling mouse events, such as mouse move, right click, and more. Should be backwards-compatible with the old touch system, though may cause discrepancies in some cases.

## Usage

Add `sapphire.mouse-api` to your dependencies in `mod.json`:

```json
{
    "dependencies": [
        {
            "id": "sapphire.mouse-api",
            "version": ">=1.0.0",
            "required": true
        }
    ]
}
```

Listening for events on a node is as simple as follows:

```cpp
#include <sapphire.mouse-api/include/API.hpp>

using namespace mouse;

node->template addEventListener<MouseEventFilter>([=](MouseEvent* event) {
    // Check if the node is being right-clicked
    if (MouseAttributes::from(node)->isHeld(MouseButton::Right)) {
        log::info("Right-clicked!");
    }
    // Check if the node is being hovered
    else if (MouseAttributes::from(node)->isHovered()) {
        log::info("Hovered!");
    }
    // Otherwise
    else {
        log::info("Hi");
    }
    // Use MouseResult::Eat to capture events without stopping propagation
    return MouseResult::Swallow;
});
```

## Tooltips & Context menu

This API also provides the `Tooltip` and `ContextMenu` classes, which are exposed through attributes, making them easily usable for any mod without needing to link to this dependency:

```cpp
node->setAttribute("sapphire.mouse-api/tooltip", "This text is shown when the node is hovered!");

$execute {
    // The node parameter on the callback is the node whose context menu was clicked
    // ContextMenuFilter is a DispatchFilter specialization so no linking needed :-)
    new EventListener<ContextMenuFilter>(+[](CCNode*) {
        FLAlertLayer::create("Hiii", "Clicked context menu item!", "OK")->show();
        return ListenerResult::Propagate;
    }, ContextMenuFilter("my-event-id"_spr));
}

node->setAttribute("sapphire.mouse-api/context-menu",
    json::Array {
        json::Object {
            { "text", "Item w/ icon" },
            { "frame", "GJ_infoIcon_001.png" },
            { "click", "my-event-id"_spr },
        },
        json::Object {
            { "text", "Item with only text" },
            { "click", "my-other-event-id"_spr },
        },
        json::Object {
            { "text", "Item with sub menu" },
            { "sub-menu", json::Array {
                json::Object {
                    { "text", "Sub menu!!" },
                    { "click", "my-event-id"_spr },
                },
            } },
        },
    }
);
```
