#pragma once

#include "gba/arm/bus_types.h"
#include <array>
#include <queue>


// Fast direkte 1-zu-1 Übersetzung des Rust Moduls vom gameboy, deswegen snake_case

namespace gba{

    class Bus;

    union SOUNDCNT_L_T {
        struct{
            HalfWord psgVolumeRight : 3;
            HalfWord _fill0 : 1;
            HalfWord psgVolumeLeft : 3;
            HalfWord _fill1 : 1;
            HalfWord enablePulse1Right : 1;
            HalfWord enablePulse2Right : 1;
            HalfWord enableWaveRight : 1;
            HalfWord enableNoiseRight : 1;
            HalfWord enablePulse1Left : 1;
            HalfWord enablePulse2Left : 1;
            HalfWord enableWaveLeft : 1;
            HalfWord enableNoiseLeft : 1;
        } state;
        HalfWord raw;
    };
    union SOUNDCNT_H_T {
        struct{
            HalfWord psgVolumeMaster : 2;
            HalfWord volumeA : 1;
            HalfWord volumeB : 1;
            HalfWord _fill : 4;
            HalfWord enableARight : 1;
            HalfWord enableALeft : 1;
            HalfWord useTimer1A : 1;
            HalfWord resetFIFOA : 1;
            HalfWord enableBRight : 1;
            HalfWord enableBLeft : 1;
            HalfWord useTimer1B : 1;
            HalfWord resetFIFOB : 1;
        } state;
        HalfWord raw;
    };

    union SOUNDBIAS_T {
        struct{
            Word _fill0 : 1;
            Word biasLevel : 9;
            Word _fill1 : 4;
            Word amplitude : 2;
            Word _fill2 : 16;
        } state;
        Word raw;
    };

    constexpr std::array<std::array<Byte, 8>, 4> WAVEFORM_PULSE = {{
    {1,0,0,0,0,0,0,0}, //12,5%
    {1,1,0,0,0,0,0,0}, //25%
    {1,1,1,1,0,0,0,0}, //50%
    {1,1,1,1,1,1,0,0}, //75%
    }};

    struct PulseChannel{
        Byte dac;
    
        //Register
        Byte sweep; // Nur Kanal 1
        Byte length_duty;
        Byte vol_env;
        Byte period_low;
        Byte period_high_control;
        bool has_sweep;
    
        // Internal
        HalfWord period_divider;
        size_t waveform_counter;
        Byte volume;
        size_t envelope_counter;
        bool sweep_enabled;
        size_t sweep_timer;
        HalfWord shadow_register;
        Byte length_timer;
        bool disabled_by_sweep;
        bool length_enabled;
        Byte sweep_pace;

        PulseChannel(bool has_sweep);
        bool active();
        void increment_length();
        Byte length_timer_base();
        bool envelope_increase_volume();
        Byte envelope_sweep_pace();
        Byte envelope_initial_volume();
        Byte get_sweep_pace();
        Byte get_sweep_individual_step();
        bool get_sweep_direction();
        void clock_envelope();
        void clock_sweep();
        HalfWord calc_freq();
        void on_write_period_high_control(Byte val);
        void on_write_period_low(Byte val);
        size_t duty();
        void on_divider_clock();
    };
    
    struct WaveChannel{
        bool dac_enable;

        //Register
        Byte initial_length_timer;
        Byte output_level;
        Byte period_low;
        Byte period_high_control;

        //Internal
        HalfWord period_divider;
        HalfWord length_timer;
        Byte volume;
        size_t wave_ram_index;
        
        bool length_enable;
        Byte dac;
        Byte sample_buffer;
        std::array<Byte, 16> wave_ram = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

        WaveChannel();
        void reset();
        bool active();
        Byte get_output_level();
        size_t get_output_shift();
        void set_volume();
        void increment_length();
        void on_write_control(Byte val);
        Byte on_read_control();
        void on_divider_clock();
    };
    
    struct NoiseChannel{
        //Register
        Byte length_timer_reg;
        Byte vol_env;
        Byte freq_rand;
        Byte control;

        //Internal
        Byte volume;
        size_t envelope_counter;
        bool length_enabled;
        Byte length_timer;
        Byte dac;
        HalfWord lfsr;
        size_t internal_divider;


        NoiseChannel();
        bool active();
        bool envelope_increase_volume();
        Byte envelope_sweep_pace();
        Byte envelope_initial_volume();
        Byte get_clock_shift();
        bool get_lfsr_width();
        Byte get_clock_divider();
        void clock_envelope();
        void increment_length();
        void on_write_control(Byte val);
        Byte on_read_control();
        void on_divider_clock();
        void lfsr_clock();
    };

    struct DMASoundChannel{
        Word startAddress;
        int8_t dac = 0;
        Byte FIFOData0 = 0;
        Byte FIFOData1 = 0;
        Byte FIFOData2 = 0;
        Byte FIFOData3 = 0;
        bool needsEnqueue = false;
        std::queue<Byte> queue;

        Bus* bus;

        DMASoundChannel(Bus* bus, Word addr) : startAddress(addr), bus(bus){};

        void onTimerOverflow();
        void onClockAPU();
        void reset();
    };
    
    class APU{
        private:
        Bus* bus;
        PulseChannel pulse1 = PulseChannel(true);
        PulseChannel pulse2 = PulseChannel(false);
        NoiseChannel noise;
        WaveChannel wave;
        Byte waveStub = 0;
        size_t total_clocks = 0;

        int16_t pulse1_sample = 0;
        int16_t pulse2_sample = 0;
        int16_t wave_sample = 0;
        int16_t noise_sample = 0;

        bool psgFIFOMasterEnable = false;

        DMASoundChannel A;
        DMASoundChannel B;

        SOUNDCNT_L_T SOUNDCNT_L = {};
        SOUNDCNT_H_T SOUNDCNT_H = {};
        SOUNDBIAS_T SOUNDBIAS = {.state = {.biasLevel = 0x100}};

        void onSOUNDCNT_H_Write();

        public:
        APU(Bus* bus) : bus(bus), A(bus, 0x040000A0), B(bus, 0x040000A4){};
        void clockPSG();
        void clockEnvelopes();
        void clockLengthCounters();
        void clockSweep();

        void onWrite(Word addr, Byte val);
        Byte onRead(Word addr);

        std::pair<float, float> getSample();


        void clockPCM();
        void onTimerOverflow(int index);
    };
    
};
