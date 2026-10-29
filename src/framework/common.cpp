#include "common.h"

const std::vector<std::pair<int, int>> defaultController1 = {
    {265, 23},
    {264, 25},
    {263, 26},
    {262, 24},
    {83, 12},
    {65, 14},
    {257, 19},
    {259, 18},
    {81, 16},
    {87, 17}
};
const std::vector<std::pair<int, int>> defaultController2 = {
    {328, 23},
    {325, 25},
    {324, 26},
    {326, 24},
    {86, 12},
    {67, 14},
    {335, 19},
    {334, 18},
    {68, 16},
    {70, 17}
};

const std::vector<AmselControlAction> defaultMapping = {
    AmselControlAction_DpadUp,
    AmselControlAction_DpadDown,
    AmselControlAction_DpadLeft,
    AmselControlAction_DpadRight,
    AmselControlAction_ButtonA,
    AmselControlAction_ButtonB,
    AmselControlAction_ButtonStart,
    AmselControlAction_ButtonSelect,
    AmselControlAction_ButtonL,
    AmselControlAction_ButtonR
};
