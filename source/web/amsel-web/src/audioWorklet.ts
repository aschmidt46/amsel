import CBuffer from "cbuffer";


class AudioQueueProcessor extends AudioWorkletProcessor {
  queue0 : CBuffer<number>;
  queue1 : CBuffer<number>;

  constructor() {
    super();

    this.queue0 = new CBuffer(2048);
    this.queue1 = new CBuffer(2048);
    this.port.onmessage = e => {
      // this.queue0.push(e.data.left);
      // this.queue1.push(e.data.right);
      e.data.left.forEach((element : number) => {
        this.queue0.push(element);
      });
      e.data.right.forEach((element : number) => {
        this.queue1.push(element);
      });
    };

  }
  process(_inputs : Float32Array[][], outputs : Float32Array[][]) {

    const output = outputs[0];
    const channel0 = output[0];
    const channel1 = output[1];

    for (let i = 0; i < channel0.length; i++) {
      if (this.queue0.length > 0) {
        channel0[i] = this.queue0.shift();
      }
    }
    for (let i = 0; i < channel1.length; i++) {
      if (this.queue0.length > 0) {
        channel1[i] = this.queue1.shift();
      }
    }
    if(this.queue0.length < 1024){
      this.port.postMessage(this.queue0.length);
    }

    return true;
  }
}

registerProcessor("audioWorklet", AudioQueueProcessor);