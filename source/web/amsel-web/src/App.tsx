import { useContext, useEffect, useRef, useState } from 'react'
import Module from './emscripten/AMSEL-web.js'
import { InputHandler } from './input.js'
import type { CXXConsole, MainModule } from './emscripten/AMSEL-web.js'
import { LocalizationContext } from './LocalizationContext.js'
import { GameController } from './GameController.js'
import { GlCanvas } from './GlCanvas.js'


function App() {
  const [emu, setEmu] = useState<CXXConsole | null>(null);
  const [input, setInput] = useState<InputHandler>(new InputHandler(emu!));
  const inputFile = useRef<HTMLInputElement | null>(null);
  const infoModal = useRef<HTMLDialogElement | null>(null);

  const [volume, setVolume] = useState(1);

  const [gameTitle, setGameTitle] = useState("");

  const lang = useContext(LocalizationContext);

  const [canSave, setCanSave] = useState(false);

  useEffect(() => {

    document.documentElement.lang = lang.language;

    input.setEmu(emu!);
    const onKeyDown = (event : KeyboardEvent) => input.onKeyAction(event.code, true);
    const onKeyUp = (event : KeyboardEvent) => input.onKeyAction(event.code, false);
      
    window.addEventListener('keydown', onKeyDown);
    window.addEventListener('keyup', onKeyUp);
    
    return () => {
      window.removeEventListener('keydown', onKeyDown);
      window.removeEventListener('keyup', onKeyUp);
    }
  }, [input, emu, lang]);

  const handleFileSelected = (e: React.ChangeEvent<HTMLInputElement>): void => {
    const files = Array.from(e.target.files!)
    e.target.blur();
    Module().then((instance : MainModule) => { files[0].bytes().then((bytes) => {
      const vec = new instance.vec_u8;
      bytes.forEach(element => {
        vec.push_back(element)
      });
      const cons = instance.createConsole(files[0].name, vec);
      setEmu(cons);
      setGameTitle(files[0].name);
      setInput(new InputHandler(cons!));
      setCanSave(cons.canSave());
    })})
  }

  const onButtonClick = () => {
    inputFile.current?.click();
  };

  function preventInput(event : React.KeyboardEvent<HTMLInputElement | HTMLButtonElement>) {
      event.preventDefault();
  }

  const onOpenModal = () => {
    infoModal.current?.showModal();
  }

  const startSaveDownload = () => {
    if(emu){
      const vec = emu.getSaveData();
      const arr : number[] = [];
      for(let i = 0; i < vec.size(); i++){
        arr.push(vec.get(i)!);
      }
      const blob = new Blob([new Uint8Array(arr)], { type: "application/octet-stream" });
      const url = URL.createObjectURL(blob);
      const link = document.createElement("a");
      link.download = emu.getGameTitle()+".sav";
      link.href = url;
      link.click();
    }
  }

  const renderSaveButton = () => {
    if(!canSave) return null;
    return <button onClick={startSaveDownload} className='btn btn-primary'>
      <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="size-6">
        <path strokeLinecap="round" strokeLinejoin="round" d="M3 16.5v2.25A2.25 2.25 0 0 0 5.25 21h13.5A2.25 2.25 0 0 0 21 18.75V16.5M16.5 12 12 16.5m0 0L7.5 12m4.5 4.5V3" />
      </svg>
      <span>
        {lang.getTranslation("DownloadSave")}
      </span>
    </button>;
  }

  const showTitle = () => {
    if(!gameTitle) return null;
    return <div className='p-1'>{gameTitle}</div>;
  }

  return (
    <>
      <section className='flex flex-col h-screen w-screen'>
        <div className="navbar bg-base-200 shadow-sm">
          <div className="flex-none navbar-start">
            <div className="avatar tooltip tooltip-right" data-tip={lang.getTranslation("AboutTitle")}>
              <div className="btn btn-ghost btn-circle rounded-full" onClick={onOpenModal}>
                <img src="./favicon.png" />
              </div>
            </div>
            <div>
              <input type='file' accept='.gb, .gbc, .nes' id='file' ref={inputFile} onChange={handleFileSelected} onKeyDown={preventInput} onKeyUp={preventInput} style={{display: 'none'}}/>
              <button onClick={onButtonClick} onKeyDown={preventInput} onKeyUp={preventInput} className='btn btn-ghost'>{lang.getTranslation("ChooseRom")}</button>
            </div>
            <span>{showTitle()}</span>
          </div>
          <div className="flex-1 navbar-center">
            <a className="hidden md:block text-xl tooltip tooltip-bottom navbar-center" data-tip="Anton's Multi-System-Emulator-Library">AMSEL</a>
          </div>
          <div className="flex-1 navbar-end">
            <div>
              <span>{renderSaveButton()}</span>
            </div>
            <div className="hidden md:block dropdown dropdown-end">
              <div tabIndex={0} role="button" className="btn m-1 btn-ghost">
                <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="size-6">
                  <path strokeLinecap="round" strokeLinejoin="round" d="M10.5 6h9.75M10.5 6a1.5 1.5 0 1 1-3 0m3 0a1.5 1.5 0 1 0-3 0M3.75 6H7.5m3 12h9.75m-9.75 0a1.5 1.5 0 0 1-3 0m3 0a1.5 1.5 0 0 0-3 0m-3.75 0H7.5m9-6h3.75m-3.75 0a1.5 1.5 0 0 1-3 0m3 0a1.5 1.5 0 0 0-3 0m-9.75 0h9.75" />
                </svg>

              </div>
              <ul tabIndex={-1} className="dropdown-content menu bg-base-100 rounded-box z-1 w-52 p-2 shadow-sm">
                <li><a>
                  <input type="range" min={0} max={100} value={volume * 100} onChange={(e) => {
                    if(emu){
                      const newVolume = Number(e.target.value) / 100.0;
                      setVolume(newVolume);
                      emu.setVolume(newVolume);
                    }
                    }} className="range" />
                  <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="size-6">
                    <path strokeLinecap="round" strokeLinejoin="round" d="M19.114 5.636a9 9 0 0 1 0 12.728M16.463 8.288a5.25 5.25 0 0 1 0 7.424M6.75 8.25l4.72-4.72a.75.75 0 0 1 1.28.53v15.88a.75.75 0 0 1-1.28.53l-4.72-4.72H4.51c-.88 0-1.704-.507-1.938-1.354A9.009 9.009 0 0 1 2.25 12c0-.83.112-1.633.322-2.396C2.806 8.756 3.63 8.25 4.51 8.25H6.75Z" />
                  </svg>
                </a></li>
              </ul>
            </div>
          </div>
        </div>
        
        <GlCanvas emuObject={emu!}></GlCanvas>

        <div className='hidden max-md:flex fit justify-center'>
          <GameController inp={input}></GameController>
        </div>





        <dialog ref={infoModal} className='modal'>
          <div className='modal-box'>
            <h3 className="font-bold text-lg">AMSEL</h3>
            <p className="py-4">{lang.getTranslation("AboutContent")}</p>
            <p className="py-4">{lang.getTranslation("AboutSystem")}</p>
            <ul>
              <li>{lang.getTranslation("AboutSystem1")}</li>
              <li>{lang.getTranslation("AboutSystem2")}</li>
            </ul>
            <p className='py-4'>{lang.getTranslation("SettingsControls")+":"}</p>
            {/* TODO */}
            <p>Arrow Keys, S, A, Enter, Backspace</p>
            <p className='pt-4'>
              <a href='...' className='underline'>Repository</a>
            </p>
            <div className="modal-action">
              <form method="dialog">
                <button className="btn">{lang.getTranslation("Close")}</button>
              </form>
            </div>
          </div>
        </dialog>
      </section>
    </>
  )
}

export default App
