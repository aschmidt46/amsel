import { useContext, useEffect, useRef, useState } from 'react'
import Module from './emscripten/AMSEL.js'
import { InputHandler } from './input.js'
import type { CXXConsole, MainModule, vec_str } from './emscripten/AMSEL.js'
import { LocalizationContext } from './LocalizationContext.js'
import { GameController } from './GameController.js'
import { GlCanvas } from './GlCanvas.js'


function App() {
  const [emu, setEmu] = useState<CXXConsole | null>(null);
  const [input, setInput] = useState<InputHandler>(new InputHandler(emu!));
  const inputFile = useRef<HTMLInputElement | null>(null);
  const inputSpecialFile = useRef<HTMLInputElement | null>(null);
  const infoModal = useRef<HTMLDialogElement | null>(null);

  const requiredFilesModal = useRef<HTMLDialogElement | null>(null);

  const [moodLighting, setMoodLighting] = useState(true);

  const [volume, setVolume] = useState(1);

  const [gameTitle, setGameTitle] = useState("");

  const lang = useContext(LocalizationContext);

  const [canSave, setCanSave] = useState(false);

  const [requiredFiles, setRequiredFiles] = useState<string[]>([]);

  const [mainModule, setMainModule] = useState<MainModule | null>(null);

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
      setMainModule(instance);
      const cons = instance.createConsole(files[0].name, vec);
      cons.setVolume(volume);
      
      const list : vec_str = cons.getRequiredFiles();
      if(list.size() > 0){
        const jsarr : string[] = [];
        for(let i : number = 0; i < list.size(); i++){
          jsarr.push(list.get(i)!);
        }
        cons.setHalt(true);
        setRequiredFiles(jsarr);
        requiredFilesModal.current?.showModal();
      }
      setEmu(cons);
      setGameTitle(files[0].name);
      setInput(new InputHandler(cons!));
      setCanSave(cons.canSave());

    })})
  }

  const handleSpecialFile = (e: React.ChangeEvent<HTMLInputElement>, name: string): void => {
    const files = Array.from(e.target.files!)
    e.target.blur();
    files[0].bytes().then((bytes) => {
      const vec = new mainModule!.vec_u8;
      bytes.forEach(element => {
        vec.push_back(element)
      });
      emu?.loadSpecialFile(name, vec);

      const newRequiredFiles = requiredFiles.filter((s:string) => (s != name));
      if(newRequiredFiles.length == 0){
        emu?.setHalt(false);
        requiredFilesModal.current?.close();
      }
      setRequiredFiles(newRequiredFiles);

    })
  }

  function preventInput(event : React.KeyboardEvent<HTMLInputElement | HTMLButtonElement>) {
      event.preventDefault();
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

  function printFileList(): import("react").ReactNode {
    const listItems = requiredFiles.map(file =>
      <li className='list-row' key={file}>
        <div className='text-xl'>{file}</div>
        <input type='file' id='filespecial' ref={inputSpecialFile} onChange={(e) => {handleSpecialFile(e, file);}} onKeyDown={preventInput} onKeyUp={preventInput} style={{display: 'none'}}/>
        <button onClick={() => inputSpecialFile.current?.click()} onKeyDown={preventInput} onKeyUp={preventInput} className='btn  btn-square btn-soft btn-primary right-0'>
          <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="size-6">
            <path strokeLinecap="round" strokeLinejoin="round" d="M3 16.5v2.25A2.25 2.25 0 0 0 5.25 21h13.5A2.25 2.25 0 0 0 21 18.75V16.5m-13.5-9L12 3m0 0 4.5 4.5M12 3v13.5" />
          </svg>

          </button>
      </li>
    );
    return <ul className='list'>{listItems}</ul>;
  }

  return (
    <>
      <section className='flex flex-col h-screen w-screen'>
        <div className="navbar bg-base-200 shadow-sm">
          <div className="flex-none navbar-start">
            <div className="avatar tooltip tooltip-right" data-tip={lang.getTranslation("AboutTitle")}>
              <div className="btn btn-ghost btn-circle rounded-full" onClick={() => infoModal.current?.showModal()}>
                <img src="./favicon.png" />
              </div>
            </div>
            <div>
              <input type='file' accept='.gb, .gbc, .nes, .gba' id='file' ref={inputFile} onChange={(e) => {handleFileSelected(e); }} onKeyDown={preventInput} onKeyUp={preventInput} style={{display: 'none'}}/>
              <button onClick={() => inputFile.current?.click()} onKeyDown={preventInput} onKeyUp={preventInput} className='btn btn-ghost'>{lang.getTranslation("ChooseRom")}</button>
            </div>
            <span>{showTitle()}</span>
          </div>
          <div className="flex-1 navbar-center">
            <a className="hidden md:block text-xl tooltip tooltip-bottom navbar-center" data-tip="Anton's Multi-System-Emulator">AMSEL</a>
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
                <li className='hidden dark:block'>
                  <a>
                    <input type="checkbox" defaultChecked value={(() => {if(moodLighting){return "on"}else{return "off"}})()} onChange={(e) => {setMoodLighting(e.target.checked)}} className="toggle" />
                    {lang.getTranslation("MoodLighting")}
                    <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="size-6">
                      <path strokeLinecap="round" strokeLinejoin="round" d="M3.375 19.5h17.25m-17.25 0a1.125 1.125 0 0 1-1.125-1.125M3.375 19.5h1.5C5.496 19.5 6 18.996 6 18.375m-3.75 0V5.625m0 12.75v-1.5c0-.621.504-1.125 1.125-1.125m18.375 2.625V5.625m0 12.75c0 .621-.504 1.125-1.125 1.125m1.125-1.125v-1.5c0-.621-.504-1.125-1.125-1.125m0 3.75h-1.5A1.125 1.125 0 0 1 18 18.375M20.625 4.5H3.375m17.25 0c.621 0 1.125.504 1.125 1.125M20.625 4.5h-1.5C18.504 4.5 18 5.004 18 5.625m3.75 0v1.5c0 .621-.504 1.125-1.125 1.125M3.375 4.5c-.621 0-1.125.504-1.125 1.125M3.375 4.5h1.5C5.496 4.5 6 5.004 6 5.625m-3.75 0v1.5c0 .621.504 1.125 1.125 1.125m0 0h1.5m-1.5 0c-.621 0-1.125.504-1.125 1.125v1.5c0 .621.504 1.125 1.125 1.125m1.5-3.75C5.496 8.25 6 7.746 6 7.125v-1.5M4.875 8.25C5.496 8.25 6 8.754 6 9.375v1.5m0-5.25v5.25m0-5.25C6 5.004 6.504 4.5 7.125 4.5h9.75c.621 0 1.125.504 1.125 1.125m1.125 2.625h1.5m-1.5 0A1.125 1.125 0 0 1 18 7.125v-1.5m1.125 2.625c-.621 0-1.125.504-1.125 1.125v1.5m2.625-2.625c.621 0 1.125.504 1.125 1.125v1.5c0 .621-.504 1.125-1.125 1.125M18 5.625v5.25M7.125 12h9.75m-9.75 0A1.125 1.125 0 0 1 6 10.875M7.125 12C6.504 12 6 12.504 6 13.125m0-2.25C6 11.496 5.496 12 4.875 12M18 10.875c0 .621-.504 1.125-1.125 1.125M18 10.875c0 .621.504 1.125 1.125 1.125m-2.25 0c.621 0 1.125.504 1.125 1.125m-12 5.25v-5.25m0 5.25c0 .621.504 1.125 1.125 1.125h9.75c.621 0 1.125-.504 1.125-1.125m-12 0v-1.5c0-.621-.504-1.125-1.125-1.125M18 18.375v-5.25m0 5.25v-1.5c0-.621.504-1.125 1.125-1.125M18 13.125v1.5c0 .621.504 1.125 1.125 1.125M18 13.125c0-.621.504-1.125 1.125-1.125M6 13.125v1.5c0 .621-.504 1.125-1.125 1.125M6 13.125C6 12.504 5.496 12 4.875 12m-1.5 0h1.5m-1.5 0c-.621 0-1.125.504-1.125 1.125v1.5c0 .621.504 1.125 1.125 1.125M19.125 12h1.5m0 0c.621 0 1.125.504 1.125 1.125v1.5c0 .621-.504 1.125-1.125 1.125m-17.25 0h1.5m14.25 0h1.5" />
                    </svg>

                  </a>
                </li>
              </ul>
            </div>
          </div>
        </div>
        
        <GlCanvas emuObject={emu!} moodLighting={moodLighting}></GlCanvas>

        <div className='hidden max-md:flex fit justify-center'>
          <GameController inp={input}></GameController>
        </div>





        <dialog ref={infoModal} className='modal'>
          <div className='modal-box'>
            <h3 className="font-bold text-lg">AMSEL</h3>
            <p className="py-4">{lang.getTranslation("AboutContent")}</p>
            <p className="py-4">{lang.getTranslation("AboutSystem")}</p>
            <ul>
              <li>{lang.getTranslation("ConsoleDescriptionCGB")}</li>
              <li>{lang.getTranslation("ConsoleDescriptionNES")}</li>
              <li>{lang.getTranslation("ConsoleDescriptionGBA")}</li>
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

        <dialog ref={requiredFilesModal} className='modal'>
          <div className='modal-box'>
            <h3 className="font-bold text-lg">{lang.getTranslation("AdditionalRequiredFiles")}</h3>
              <div>
                {printFileList()}
              </div>
          </div>
        </dialog>
      </section>
    </>
  )
}

export default App
