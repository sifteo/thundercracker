#ifndef UI_LOWBATT_H
#define UI_LOWBATT_H

#include "ui_menu.h"
#include "batterylevel.h"

class UILowBatt
{
public:
    enum Action {
        WARNING = 0,
        QUIT,
        NUM_ITEMS
    };

    UILowBatt(UICoordinator &uic);

    void init(uint8_t _cubeNum = BatteryLevel::BASE);
    bool quitWasSelected() const;

    ALWAYS_INLINE void animate() {
        if (cubeNum == BatteryLevel::BASE)
            return baseMenu.animate();
        else
            return cubeMenu.animate();
    }

    ALWAYS_INLINE bool isDone() const {
        if (cubeNum == BatteryLevel::BASE)
            return baseMenu.isDone();
        else
            return cubeMenu.isDone();
    }

private:
    UIMenu baseMenu;
    UIMenu cubeMenu;
    uint8_t cubeNum;

    static const UIMenu::Item baseItems[NUM_ITEMS];
    static const UIMenu::Item cubeItems[NUM_ITEMS];
};

#endif // UILowBatt

