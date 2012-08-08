#ifndef UI_CUBERANGE_H
#define UI_CUBERANGE_H

#include "ui_menu.h"

class UICubeRange
{
public:
    enum Action {
        CUBERANGE = 0,
        QUIT,
        NUM_ITEMS
    };

    UICubeRange(UICoordinator &uic);

    void init();
    bool quitWasSelected() const;

    ALWAYS_INLINE void animate() {
        return menu.animate();
    }

    ALWAYS_INLINE bool isDone() const {
        return menu.isDone();
    }

private:
    UIMenu menu;

    static const UIMenu::Item items[NUM_ITEMS];
};

#endif // UI_CUBERANGE_H
