
#include <libretro.h>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include "../console/console.h"
#include "../console/dummy_implementation.h"

static std::unique_ptr<Console> console = std::make_unique<DummyImplementation>();

std::string getExtensionListLRetro(){
    std::string extensions = "";
	#ifdef BUILD_NES
	extensions += "nes|";
	#endif
	#ifdef BUILD_CGB
	extensions += "gbc|gb|";
	#endif
	#ifdef BUILD_GBA
	extensions += "gba|";
	#endif
	extensions += "bin";

	return extensions;
}

static retro_environment_t environ_cb;
static retro_log_printf_t log_cb;
static struct retro_log_callback logging;

static bool use_audio_cb;

static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;


void retro_init(void)
{
   
}

void retro_deinit(void)
{

}

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{

}

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

   if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
      log_cb = logging.log;
   else
      log_cb = fallback_log;

//    static const struct retro_controller_description controllers[] = {
//       { "Nintendo DS", RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0) },
//    };

//    static const struct retro_controller_info ports[] = {
//       { controllers, 1 },
//       { NULL, 0 },
//    };

//    cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);
}


void retro_get_system_info(retro_system_info* info)
{
	std::memset(info, 0, sizeof(retro_system_info));
	info->library_name = "AMSEL-CORE";
	info->need_fullpath = false;
	info->valid_extensions = getExtensionListLRetro().c_str();
}


void retro_get_system_av_info(retro_system_av_info* info)
{
	std::memset(info, 0, sizeof(retro_system_av_info));
	info->timing.fps = 60.0f;
	info->timing.sample_rate = 20000;
	info->geometry.base_width = console->getX();
	info->geometry.base_height = console->getY();
	info->geometry.max_width = console->getX();
	info->geometry.max_height = console->getY();
	info->geometry.aspect_ratio = (float)console->getY() / (float)console->getX();
}

static void audio_callback(void)
{
	console->clockUntilSampleReady();
	auto sample = console->getSample();
	audio_cb(sample.first, sample.second);
}

static void audio_set_state(bool enable)
{
   (void)enable;
}

/**
	Load ROM
*/
bool retro_load_game(const retro_game_info* info)
{

   	enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
   	if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   	{
   	   log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported.\n");
   	   return false;
   	}


	if (info && info->data)
	{
		std::vector<uint8_t> rom;
		rom.assign((uint8_t*)info->data, (uint8_t*)info->data + info->size);
		console = createConsoleFromData(info->path, rom);
	}

	struct retro_audio_callback audio_cb = { audio_callback, audio_set_state };
   	use_audio_cb = environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK, &audio_cb);

	if(!use_audio_cb){
		return false;
	}

	return true;
}

static retro_video_refresh_t video_cb;

// Callback in Sample Rate wie oben
void retro_run(void)
{
	video_cb(console->accessFramebuffer(), console->getX(), console->getY(), console->getX() * sizeof(uint32_t));
}

void retro_unload_game(void)
{

}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   return false;
}

size_t retro_serialize_size(void)
{
   return 0;
}

bool retro_serialize(void *data_, size_t size)
{
   return false;
}

bool retro_unserialize(const void *data_, size_t size)
{
   return false;
}

void *retro_get_memory_data(unsigned id)
{
   (void)id;
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   (void)id;
   return 0;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

void retro_reset(void)
{

}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   log_cb(RETRO_LOG_INFO, "Plugging device %u into port %u.\n", device, port);
}
