import { CircularQueueNonBlocking } from './circularBuffer.js'




class AudioQueueProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super();

    this.queue0 = new CircularQueueNonBlocking(2048);
    this.queue1 = new CircularQueueNonBlocking(2048);
    this.port.onmessage = e => {
      e.data.left.forEach((element) => {
        this.queue0.enqueue(element);
      });
      e.data.right.forEach((element) => {
        this.queue1.enqueue(element);
      });
    };

  }
  process(inputs, outputs, parameters) {

    const output = outputs[0];
    const channel0 = output[0];
    const channel1 = output[1];

    for (let i = 0; i < channel0.length; i++) {
      if (!this.queue0.isEmpty()) {
        channel0[i] = this.queue0.dequeue();
      }
    }
    for (let i = 0; i < channel1.length; i++) {
      if (!this.queue1.isEmpty()) {
        channel1[i] = this.queue1.dequeue();
      }
    }
    this.port.postMessage(this.queue0.length());

    return true;
  }
}

registerProcessor("audioWorklet", AudioQueueProcessor);