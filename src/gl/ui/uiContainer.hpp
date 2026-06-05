#include "ui.hpp"

/**
 * 
 * UIContainer
 * 
 * base class of ui elements that are types of containers
 * such as group boxes, general containers, windows, and
 * other types of containers
 * 
 */
class UIContainer : public UIObject {
public:
    enum class OverflowMode {
        None,
        Hide,
        Scroll,
        Adjust
    };
private:
    OverflowMode o_mode = OverflowMode::None;
    //std::vector<UIElement> elems;
public:
    
};