
// Reihenfolge wichtig
const localStrings = [
    "ChooseRom",
    "RomFilter",
    "GamepadKeyBack",
    "DpadUp",
    "DpadDown",
    "DpadLeft",
    "DpadRight",
    "KeySpace",
    "KeyEsc",
    "KeyEnter",
    "KeyTab",
    "KeyBackspace",
    "KeyInsert",
    "KeyDelete",
    "KeyRight",
    "KeyLeft",
    "KeyDown",
    "KeyUp",
    "KeyPageUp",
    "KeyPageDown",
    "KeyHome",
    "KeyEnd",
    "KeyCapsLock",
    "KeyScrollLock",
    "KeyNumLock",
    "KeyPrint",
    "KeyPause",
    "KeyF1",
    "KeyF2",
    "KeyF11",
    "KeyNumEnter",
    "KeyLShift",
    "KeyLCRTL",
    "KeyLAlt",
    "KeyLSuper",
    "KeyRShift",
    "KeyRCRTL",
    "KeyRAlt",
    "KeyRSuper",
    "KeyMenu",
    "DebuggerJump",
    "DebuggerAddress",
    "DebuggerRead1Byte",
    "DebuggerRead2Byte",
    "DebuggerHex",
    "DebuggerOpcode",
    "DebuggerBin",
    "DebuggerAdd",
    "DebuggerAddOp",
    "DebuggerBreakOp",
    "DebuggerBreakAddr",
    "DebuggerDebugger",
    "DebuggerBreakButton",
    "DebuggerStep",
    "DebuggerRegister",
    "DebuggerRam",
    "DebuggerBreakpoints",
    "TestOutput",
    "TestOutputStartAddress",
    "Controls",
    "Controller1",
    "Controller2",
    "SAction",
    "BindingKeyboard",
    "BindingGamepad",
    "BindingDpadUp",
    "BindingDpadDown",
    "BindingDpadLeft",
    "BindingDpadRight",
    "BindingAButton",
    "BindingBButton",
    "BindingStartButton",
    "BindingSelectButton",
    "GamepadColon",
    "PressButtonPrompt",
    "MenuFile",
    "FileLoad",
    "MenuSettings",
    "SettingsControls",
    "SettingsFullscreen",
    "SettingsCRTShader",
    "SettingsSound",
    "MenuDebugger",
    "DebuggerDBItem",
    "DebuggerText",
    "SaveFileCreated",
    "GameError",
    "MapperNotSupported",
    "EmulatorError",
    "UnimplementedInstruction",
    "UnsafeInstruction",
    "EmulatorWarning",
    "BRKExecuted",
    "SettingsLanguage",
    "AboutTitle",
    "AboutContent",
    "AboutSystem",
    "ConsoleDescriptionCGB",
    "ConsoleDescriptionNES",
    "AboutSource",
    "Close",
    "DownloadSave",
    "MoodLighting",
    "Read4Bytes",
    "LastTransaction",
    "ConsoleDescriptionGBA",
    "AdditionalRequiredFiles"
];

import german from './assets/locales/de.json'
import english from './assets/locales/en.json'

const langs = new Map([["de", german], ["en", english]]);

export class LocalizationProvider{
    language : string;
    displayName : string;
    private translations : Map<string, string>; // (key -> translation)
    constructor(lang : string){
        let langObj = langs.get(lang);
        if(!langObj){
            langObj = english;
            lang = "en";
        }
        this.displayName = langObj.displayName;
        this.language = lang;
        this.translations = new Map();
        for(let i = 0; i < localStrings.length; i++){
            this.translations.set(localStrings[i], langObj.strings[i]);
        }
    }

    getTranslation(key : string){
        return this.translations.get(key);
    }
}

