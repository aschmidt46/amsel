#include "control.h"

LocalizedString amselActionToLocale(AmselControlAction a){
    switch(a){
        case AmselControlAction_DpadUp : return BindingDpadUp;
        case AmselControlAction_DpadDown : return BindingDpadDown;
        case AmselControlAction_DpadLeft : return BindingDpadLeft;
        case AmselControlAction_DpadRight : return BindingDpadRight;
        case AmselControlAction_ButtonA : return BindingAButton;
        case AmselControlAction_ButtonB : return BindingBButton;
        case AmselControlAction_ButtonL : return BindingLButton;
        case AmselControlAction_ButtonR : return BindingRButton;
        case AmselControlAction_ButtonStart : return BindingStartButton;
        case AmselControlAction_ButtonSelect : return BindingSelectButton;
        default: return LocalizedString::GameError;
    }
}
