#include "audiosystem.h"
#include <thread>

static NES* n;

AudioSystem::AudioSystem(NES *console)
{
    nes = console;
    n = console;
}

AudioSystem::~AudioSystem()
{
    if ( dac.isStreamOpen() ) dac.closeStream();
}

int waveFun( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
         double streamTime, RtAudioStreamStatus status, void *userData )
{
  unsigned int i, j;
  double *buffer = (double *) outputBuffer;
  unsigned int *lastValues = (unsigned int *) userData;
  
  if ( status )
  std::cout << "Stream underflow detected!" << std::endl;
  
  for(int i = 0; i < nBufferFrames; i++){
      while(!n->clock()) {}
        buffer[2*i + 0] = n->audioSample;
        buffer[2*i + 1] = n->audioSample;
        // double wave = sin(440.0 * lastValues[0] * 2 * 3.1415296 / 22000);
        // buffer[2*i + 0] = wave;
        // buffer[2*i + 1] = wave;
        // lastValues[0]+= 1;
      }
  return 0;
}

void AudioSystem::start()
{
      std::vector<unsigned int> deviceIds = dac.getDeviceIds();
  if ( deviceIds.size() < 1 ) {
    std::cout << "\nNo audio devices found!\n";
    exit( 0 );
  }
 
  RtAudio::StreamParameters parameters;
  parameters.deviceId = dac.getDefaultOutputDevice();
  parameters.nChannels = 2;
  parameters.firstChannel = 0;
  unsigned int bufferFrames = 256; // 256 sample frames
  unsigned int data[2] = {0, 0};
 
  if ( dac.openStream( &parameters, NULL, RTAUDIO_FLOAT64, sampleRate,
                       &bufferFrames, &waveFun, (void *)&data ) ) {
    std::cout << '\n' << dac.getErrorText() << '\n' << std::endl;
    exit( 0 ); // problem with device settings
}
auto err = dac.startStream();
while(!close){
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
}
