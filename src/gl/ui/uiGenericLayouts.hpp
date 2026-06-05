#include "ui.hpp"

class UISimpleContainer : public UILayout {
private:

public:

};

typedef i32 UITabID;

struct UITab {
    
};

class UITabContainer : public UILayout {
private:

public:
    UITabID addTab(std::string name, i32 idx = -1);
};