#ifndef UI_LOWBATT_H
#define UI_LOWBATT_H

#include "ui_menu.h"

class UILowBatt
{
public:
    enum Action {
        WARNING = 0,
        QUIT,
        NUM_ITEMS
    };

    UILowBatt(UICoordinator &uic);

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

#endif // UILowBatt

