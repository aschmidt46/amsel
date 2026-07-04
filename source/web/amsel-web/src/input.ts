import type { CXXConsole } from "./emscripten/AMSEL";

const jsToGLFW : Map<string, number> = new Map
    ([
        ["Backspace", 259],
        ["Tab", 258],
        ["Enter", 257],
        ["ShiftLeft", 340],
        ["ShiftRight", 344],
        ["ControlLeft", 341],
        ["ControlRight", 345],
        ["AltLeft", 342],
        ["AltRight", 346],
        ["Pause", 284],
        ["CapsLock", 280],
        ["Escape", 256],
        ["Space", 32],
        ["PageUp", 266],
        ["PageDown", 267],
        ["End", 269],
        ["Home", 268],
        ["ArrowLeft", 263],
        ["ArrowUp", 265],
        ["ArrowRight", 262],
        ["ArrowDown", 264],
        ["PrintScreen", 283],
        ["Insert", 260],
        ["Delete", 261],
        ["Digit0", 48],
        ["Digit1", 49],
        ["Digit2", 50],
        ["Digit3", 51],
        ["Digit4", 52],
        ["Digit5", 53],
        ["Digit6", 54],
        ["Digit7", 55],
        ["Digit8", 56],
        ["Digit9", 57],
        ["KeyA", 65],
        ["KeyB", 66],
        ["KeyC", 67],
        ["KeyD", 68],
        ["KeyE", 69],
        ["KeyF", 70],
        ["KeyG", 71],
        ["KeyH", 72],
        ["KeyI", 73],
        ["KeyJ", 74],
        ["KeyK", 75],
        ["KeyL", 76],
        ["KeyM", 77],
        ["KeyN", 78],
        ["KeyO", 79],
        ["KeyP", 80],
        ["KeyQ", 81],
        ["KeyR", 82],
        ["KeyS", 83],
        ["KeyT", 84],
        ["KeyU", 85],
        ["KeyV", 86],
        ["KeyW", 87],
        ["KeyX", 88],
        ["KeyY", 89],
        ["KeyZ", 90],
        ["MetaLeft", 343],
        ["MetaRight", 347],
        ["ContextMenu", -1],
        ["Numpad0", 320],
        ["Numpad1", 321],
        ["Numpad2", 322],
        ["Numpad3", 323],
        ["Numpad4", 324],
        ["Numpad5", 325],
        ["Numpad6", 326],
        ["Numpad7", 327],
        ["Numpad8", 328],
        ["Numpad9", 329],
        ["NumpadMultiply", 332],
        ["NumpadAdd", 334],
        ["NumpadSubtract", 333],
        ["NumpadDecimal", 330],
        ["NumpadDivide", 331],
        ["F1", 290],
        ["F2", 291],
        ["F3", 292],
        ["F4", 293],
        ["F5", 294],
        ["F6", 295],
        ["F7", 296],
        ["F8", 297],
        ["F9", 298],
        ["F10", 299],
        ["F11", 300],
        ["F12", 301],
        ["NumLock", 282],
        ["ScrollLock", 281],
        ["Semicolon", 59],
        ["Equal", 61],
        ["Comma", 44],
        ["Minus", 45],
        ["Period", 46],
        ["Slash", 47],
        ["Backquote", 96],
        ["BracketLeft", 91],
        ["Backslash", 92],
        ["BracketRight", 93],
        ["Quote", 39]
    ])




export enum GameInput {
    GameUp,
    GameDown,
    GameLeft,
    GameRight,
    GameA,
    GameB,
    GameStart,
    GameSelect
}


export class InputHandler{
    // private mapping : Map<GameInput, string>;
    private emuObject : CXXConsole;


    constructor(emu : CXXConsole){
        this.emuObject = emu;
        // this.mapping = new Map();
    }

    setEmu(emu : CXXConsole){
        this.emuObject = emu;
    }

    onKeyAction(key : string, action : boolean) : void{
        if(this.emuObject){
            const glfwCode = jsToGLFW.get(key)!;
            if(glfwCode > 0){
                this.emuObject.setController1Key(false, glfwCode, +action);
                this.emuObject.setController2Key(false, glfwCode, +action);
            }
        }
    }

    pressAction(inputAction : GameInput, press : boolean) : void{
        let key :string;
        switch(inputAction){
            case GameInput.GameUp:{
                key = "ArrowUp";
                break;
            }
            case GameInput.GameDown:{
                key = "ArrowDown";
                break;
            }
            case GameInput.GameLeft:{
                key = "ArrowLeft";
                break;
            }
            case GameInput.GameRight:{
                key = "ArrowRight";
                break;
            }
            case GameInput.GameA:{
                key = "KeyS";
                break;
            }
            case GameInput.GameB:{
                key = "KeyA";
                break;
            }
            case GameInput.GameStart:{
                key = "Enter";
                break;
            }
            case GameInput.GameSelect:{
                key = "Backspace";
                break;
            }
        }

        this.onKeyAction(key, press);
    }

    // setMapping(key : string, input : GameInput){
    //     // this.mapping[input] = key;
    // }
}

