#pragma once
#include "locale.h"

// Nimmt Nintendo Controller Layout an (a = rechts, b = unten, usw.)
enum AmselControlAction : int{
    AmselControlAction_DpadUp = 0,
    AmselControlAction_DpadDown = 1,
    AmselControlAction_DpadLeft = 2,
    AmselControlAction_DpadRight = 3,
    AmselControlAction_ButtonA = 4,
    AmselControlAction_ButtonB = 5,
    AmselControlAction_ButtonStart = 6,
    AmselControlAction_ButtonSelect = 7,
    AmselControlAction_ButtonL = 8,
    AmselControlAction_ButtonR = 9,
    AmselControlAction_Invalid = 99,
};

LocalizedString amselActionToLocale(AmselControlAction a);