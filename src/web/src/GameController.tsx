import type { CSSProperties } from "react"
import { GameInput, type InputHandler } from "./input"


export function GameController ({inp} : {inp : InputHandler}){

    // Heroicons.com
    const arrowUp = () => { return (
        <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="size-6">
            <path strokeLinecap="round" strokeLinejoin="round" d="M8.25 6.75 12 3m0 0 3.75 3.75M12 3v18" />
        </svg>
    )}

    const arrowDown = () => { return (
        <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="size-6">
          <path strokeLinecap="round" strokeLinejoin="round" d="M19.5 13.5 12 21m0 0-7.5-7.5M12 21V3" />
        </svg>
    )}

    const arrowLeft = () => { return (
        <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="size-6">
    <path strokeLinecap="round" strokeLinejoin="round" d="M10.5 19.5 3 12m0 0 7.5-7.5M3 12h18" />
        </svg>
    )}

    const arrowRight = () => { return (
        <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="size-6">
            <path strokeLinecap="round" strokeLinejoin="round" d="M13.5 4.5 21 12m0 0-7.5 7.5M21 12H3" />
        </svg>
    )}

    function maybePress(input : GameInput){
        if(inp){
            inp.pressAction(input, true);
        }
    }
    function maybeUnpress(input : GameInput){
        if(inp){
            inp.pressAction(input, false);
        }
    }

    const pushToFront : CSSProperties = {
        zIndex: "99"
    }

    function pctx(event : React.MouseEvent){
        event.preventDefault();
        event.stopPropagation();
    }

    return(
        <div className="grid grid-cols-9 gap-2 m-10">
            <div style={{pointerEvents: "none"}}></div>
            <div>
                <button className="btn btn-square btn-xl" onContextMenu={pctx} 
                onTouchStart={ () => maybePress(GameInput.GameUp)} onTouchEnd={ () => maybeUnpress(GameInput.GameUp)}>
                    {arrowUp()}</button>
            </div>
            <div style={{pointerEvents: "none"}}></div>
            <div style={{pointerEvents: "none"}}></div>
            <div><button className="btn rounded-3xl w-15" onContextMenu={pctx}
            onTouchStart={ () => maybePress(GameInput.GameSelect)} onTouchEnd={ () => maybeUnpress(GameInput.GameSelect)}>
                Select</button></div>
            <div><button style={pushToFront} className="btn rounded-3xl w-15 ml-5" onContextMenu={pctx}
            onTouchStart={ () => maybePress(GameInput.GameStart)} onTouchEnd={ () => maybeUnpress(GameInput.GameStart)}>
                Start</button></div>
            <div style={{pointerEvents: "none"}}></div>
            <div style={{pointerEvents: "none"}}></div>
            <div style={{pointerEvents: "none"}}></div>

            <div><button className="btn btn-square btn-xl" onContextMenu={pctx}
            onTouchStart={ () => maybePress(GameInput.GameLeft)} onTouchEnd={ () => maybeUnpress(GameInput.GameLeft)}>
                {arrowLeft()}</button></div>
            <div style={{pointerEvents: "none"}}></div>
            <div><button className="btn btn-square btn-xl" onContextMenu={pctx}
            onTouchStart={ () => maybePress(GameInput.GameRight)} onTouchEnd={ () => maybeUnpress(GameInput.GameRight)}>
                {arrowRight()}</button></div>
            <div style={{pointerEvents: "none"}}></div>
            <div style={{pointerEvents: "none"}}></div>
            <div style={{pointerEvents: "none"}}></div>
            <div style={{pointerEvents: "none"}}></div>
            <div><button className="btn btn-square btn-xl btn-circle" onContextMenu={pctx}
            onTouchStart={ () => maybePress(GameInput.GameA)} onTouchEnd={ () => maybeUnpress(GameInput.GameA)}>
                A</button></div>
            <div style={{pointerEvents: "none"}}></div>

            <div style={{pointerEvents: "none"}}></div>
            <div><button className="btn btn-square btn-xl" onContextMenu={pctx}
            onTouchStart={ () => maybePress(GameInput.GameDown)} onTouchEnd={ () => maybeUnpress(GameInput.GameDown)}>
                {arrowDown()}</button></div>
            <div style={{pointerEvents: "none"}}></div>
            <div style={{pointerEvents: "none"}}></div>
            <div style={{pointerEvents: "none"}}></div>
            <div><button className="btn btn-square btn-xl btn-circle" onContextMenu={pctx}
            onTouchStart={ () => maybePress(GameInput.GameB)} onTouchEnd={ () => maybeUnpress(GameInput.GameB)}>
                B</button></div>
            <div style={{pointerEvents: "none"}}></div>
            <div style={{pointerEvents: "none"}}></div>
            <div style={{pointerEvents: "none"}}></div>
    </div>
    );
}