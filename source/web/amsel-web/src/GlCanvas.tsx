import { useEffect, useRef, useState, type CSSProperties } from 'react'

import processorUrl from "./audioWorklet.ts?worker&url";
import type { CXXConsole } from './emscripten/AMSEL-web';


export function GlCanvas({emuObject, moodLighting} : {emuObject : CXXConsole, moodLighting: boolean}) {
    const myCanvas = useRef<HTMLCanvasElement>(null);
    const myCanvas2 = useRef<HTMLCanvasElement>(null);
    // const [gl, setGL] = useState<WebGL2RenderingContext | null>(null);
    const [actx, setActx] = useState(new AudioContext({sampleRate: 20000}));

    const [aspect, setAspect] = useState(1)
    const [anode, setAnode] = useState<{audioNode: AudioWorkletNode | null}>({audioNode: null});

    const vertexCode = `#version 300 es
        in vec3 aPos;
        in vec2 aTex;
        out vec2 TexCoord;
        void main(){
            TexCoord = aTex;
            gl_Position = vec4(aPos, 1.0);
        }`
    const fragmentCode = `#version 300 es
      uniform sampler2D screenTexture;
      in highp vec2 TexCoord;
      out highp vec4 FragColor;
      void main(){
          FragColor = texture(screenTexture, TexCoord);
      }`

    // Setup
    useEffect(() => {
        console.log("effect");
        let animationFrame = 0;
        const cur = myCanvas.current!;
        const gl = cur.getContext("webgl2", {preserveDrawingBuffer: true})!;
        if(!gl) return;

        if (!gl.getExtension('EXT_color_buffer_float')){
            throw new Error('Rendering to floating point textures is not supported on this platform');
        }

        if(emuObject){
            const ratio = emuObject.getX() / emuObject.getY();
            setAspect(ratio);
        }

        gl.clearColor(0,0,0,1);
        gl.clear(gl.COLOR_BUFFER_BIT);

        const program = gl.createProgram();

        const vertex = gl.createShader(gl.VERTEX_SHADER)!;
        const fragment = gl.createShader(gl.FRAGMENT_SHADER)!;
        gl.shaderSource(vertex, vertexCode);
        gl.shaderSource(fragment, fragmentCode);

        [vertex, fragment].forEach(shader => {
            gl.compileShader(shader);
            if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
                const info = gl.getShaderInfoLog(shader);
                throw new Error(`Could not compile WebGL program. \n\n${info}`);
            }
        });

        gl.attachShader(program, vertex!);
        gl.attachShader(program, fragment!);

        gl.linkProgram(program);

        if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
          const info = gl.getProgramInfoLog(program);
          throw new Error(`Could not compile WebGL program. \n\n${info}`);
        }

        gl.useProgram(program);

        const apos = gl.getAttribLocation(program, "aPos");
        const atex = gl.getAttribLocation(program, "aTex");

        
        const vboPos = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, vboPos);
        
        const quadPos = new Float32Array(
            [
                -1.0,-1.0,1.0,
                -1.0,1.0,1.0,
                1.0,-1.0,1.0,
                1.0,1.0,1.0
            ]);
            
        gl.bufferData(gl.ARRAY_BUFFER, quadPos, gl.STATIC_DRAW);

        const ebo = gl.createBuffer();
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, ebo);
        
        const quadIndices = new Int32Array(
            [
                0,1,2,3,2,1
            ]);
            
        gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, quadIndices, gl.STATIC_DRAW);

        const vboTex = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, vboTex);
        
        const quadTex = new Float32Array(
            [
                0,1,
                0,0,
                1,1,
                1,0
            ]);
            
        gl.bufferData(gl.ARRAY_BUFFER, quadTex, gl.STATIC_DRAW);
        
        gl.enableVertexAttribArray(apos);

        gl.bindBuffer(gl.ARRAY_BUFFER, vboPos);

        gl.vertexAttribPointer(apos, 3, gl.FLOAT, false, 0, 0);



        gl.enableVertexAttribArray(atex);

        gl.bindBuffer(gl.ARRAY_BUFFER, vboTex);

        gl.vertexAttribPointer(atex, 2, gl.FLOAT, false, 0, 0);

        const texture = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, texture);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);

        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA32F, 2, 2, 0, gl.RGBA, gl.FLOAT,
                new Float32Array([0, 0, 1, 1,   1, 0, 0, 1,     0, 1, 0, 1,     1, 1, 1, 1]));

        gl.viewport(0, 0, gl.canvas.width, gl.canvas.height);

        if(emuObject){
            let queueSize = 0;
            // Reset
            if(anode.audioNode){
                actx.close().then(() => {setActx(new AudioContext({sampleRate: 20000}))});
                setAnode({audioNode: null});
            }
            if(actx.state == "closed"){ return;}
            actx.audioWorklet.addModule(processorUrl).then(() => {
          
            anode.audioNode = new AudioWorkletNode(
              actx,
              "audioWorklet",
              {outputChannelCount: [2]}
            );
    
      
            anode.audioNode.port.onmessage = e => {
                if(emuObject && anode.audioNode && gl){
                    queueSize = e.data;
                    const q0 = [];
                    const q1 = [];
                    while(queueSize < 1024){
                        emuObject.clockUntilSampleReady();
                        const sample = emuObject.getSample();
                        q0.push(sample.first);
                        q1.push(sample.second);
                        queueSize += 1;
                    }
                    anode.audioNode?.port.postMessage({left: q0, right: q1});
                }
            };
          
            anode.audioNode?.connect(actx.destination);
    
            
            actx.resume();

            function render () {
                
                if(emuObject){
    
                    gl.bindTexture(gl.TEXTURE_2D, texture);
                    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA32F, emuObject.getX(), emuObject.getY(), 0, gl.RGBA, gl.FLOAT,
                    emuObject.accessFramebufferJS());
                    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
                    gl.drawElements(gl.TRIANGLES, 6, gl.UNSIGNED_INT, 0);
                    animationFrame = requestAnimationFrame(render);

                    if(myCanvas2.current){
                        const cur = myCanvas2.current!;
                        const gl2 = cur.getContext("2d")!;
                        gl2.drawImage(myCanvas.current!, 0, 0);
                        gl2.filter = 'blur(10px)';
                    }
                }
            }
    
            animationFrame = requestAnimationFrame(render);
    
            });
        }

        
        return () => {cancelAnimationFrame(animationFrame);}
    }, [emuObject, fragmentCode, vertexCode, actx, anode]);

    const style : CSSProperties = {
      imageRendering: 'pixelated',
      overflow: 'hidden',
      zIndex: 1
    }

    function getHeight(){
        if(emuObject) return emuObject.getY();
        else return 1;
    }
    function getWidth(){
        if(emuObject) return emuObject.getX();
        else return 1;
    }

    function getAspectRatio(){
        return "aspect-["+aspect.toString()+"] flex";
    }

    function maybeRenderMoody(){
        if(moodLighting){
            return (
                <div className='hidden dark:block mask-radial-from-current grow fixed top-0 left-0' style={{zIndex: 0, pointerEvents: 'none'}}>
                    <canvas ref={myCanvas2} width={getWidth()} style={{imageRendering: 'smooth'}} height={getHeight()} className="h-screen w-screen flex grow" ></canvas>
                </div>
            )
        }
        else{
            return (<span></span>)
        }
    }

    return (
        // Unschön, da bei Smartphones das Seitenverhältnis nicht ganz stimmt, ich finde keine andere funktionierende Lösung
        <div className='flex grow justify-center h-screen w-screen max-h-dvw'>
            <canvas ref={myCanvas} width={getWidth()} style={style} height={getHeight()} className={getAspectRatio()} ></canvas>
            {maybeRenderMoody()}
        </div>


    )
}