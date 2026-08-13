#include "audiosystem.h"

static AudioSystem* t;

AudioSystem::AudioSystem()
{
    t = this;
}

AudioSystem::~AudioSystem()
{
    if ( dac.isStreamOpen() ) dac.closeStream();
}

int waveFun( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
         double streamTime, RtAudioStreamStatus status, void *userData )
{
  (void)userData; (void)streamTime; (void)inputBuffer;

  double *buffer = (double*)outputBuffer;
  
  if(status)
    std::cout << "Stream underflow detected!" << std::endl;

  {
    std::lock_guard lock{consoleLock};
    for(unsigned int i = 0; i < nBufferFrames; i++){
      if(!t->close && console->isLoaded()) {
        console->clockUntilSampleReady();
      }
      if(t->close) return 0;

      auto sample = console->getSample();
      buffer[2*i + 0] = globalConfig.unmute * globalConfig.volume * sample.first;
      buffer[2*i + 1] = globalConfig.unmute * globalConfig.volume * sample.second;
    }
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
  dac.startStream();
}
