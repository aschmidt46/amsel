#include "apu.h"
#include <math.h>
#include <cmath>
#include <bit>
#include "bus.h"
#include <iostream>

using namespace gba;

APU::APU(Bus* bus) : bus(bus), A(bus, 0x040000A0), B(bus, 0x040000A4){};

gba::PulseChannel::PulseChannel(bool has_sweep) :
dac(0), sweep(0), length_duty(0), vol_env(0), period_low(0), period_high_control(0), has_sweep(false), period_divider(0), waveform_counter(0), volume(0)
,envelope_counter(0), sweep_enabled(false), sweep_timer(0), shadow_register(0), length_timer(0), disabled_by_sweep(false), length_enabled(false), sweep_pace(0)
{}

bool gba::PulseChannel::active()
{
    bool l = length_enabled ? length_timer > 0 : true;
    return (vol_env & 0xF8) > 0 && !disabled_by_sweep && l;
}

void gba::PulseChannel::increment_length()
{
    if(length_timer >= 0){
        length_timer -= 1;
    }
}

Byte gba::PulseChannel::length_timer_base()
{
    return 64 - (length_duty & 0b00111111);
}

bool gba::PulseChannel::envelope_increase_volume()
{
    return (vol_env & 0b1000) > 0;
}

Byte gba::PulseChannel::envelope_sweep_pace()
{
    return vol_env & 0b111;
}

Byte gba::PulseChannel::envelope_initial_volume()
{
    return (vol_env & 0b11110000) >> 4;
}

Byte gba::PulseChannel::get_sweep_pace()
{
    return (sweep & 0b01110000) >> 4;
}

Byte gba::PulseChannel::get_sweep_individual_step()
{
    return sweep & 0b111;
}

bool gba::PulseChannel::get_sweep_direction()
{
    return (sweep & 0b00001000) > 0;
}

void gba::PulseChannel::clock_envelope()
{
    envelope_counter += 1;
    if(envelope_sweep_pace() > 0){
        if(envelope_counter % (envelope_sweep_pace()) == 0){
            if(envelope_increase_volume()){
                if(volume < 15) volume++;
            }
            else{
                if(volume > 0) volume--;
            }
        }
    }
}

void gba::PulseChannel::clock_sweep()
{
    if(sweep_timer > 0) sweep_timer -= 1;

    if(sweep_timer == 0){
        sweep_timer = sweep_pace == 0 ? 8 : sweep_pace;

        if(sweep_enabled && sweep_pace > 0){
            const auto new_freq = calc_freq();

            if(get_sweep_individual_step() > 0 && new_freq <= 2047){
                period_divider = new_freq;
                shadow_register = new_freq;
                period_low = (Byte)(new_freq & 255);
                period_high_control = (period_high_control & ~0b111u) | (Byte(new_freq >> 8));
            }
            sweep_pace = get_sweep_pace();
            calc_freq();
        }
    }
}

HalfWord gba::PulseChannel::calc_freq()
{
    HalfWord new_freq = shadow_register >> get_sweep_individual_step();
    if(get_sweep_direction()){
        new_freq = shadow_register - new_freq;
        // saturating sub
        if(new_freq > shadow_register) new_freq = 0;
    }
    else{
        new_freq = shadow_register + new_freq;
        if(new_freq < shadow_register) new_freq = std::numeric_limits<uint16_t>::max();
    }
    if(new_freq > 0x7FF){
        disabled_by_sweep = true;
    }
    return new_freq;
}

void gba::PulseChannel::on_write_period_high_control(Byte val)
{
    period_high_control = val;
    disabled_by_sweep = false;
    length_enabled = (period_high_control & 64) > 0;

    if((period_high_control & 128) > 0 && (vol_env & 0xF8) > 0){
        if(length_timer == 0){
            length_timer = length_timer_base();
        }
        envelope_counter = 0;

        period_divider = HalfWord(period_low) | ((HalfWord(period_high_control) & 0b111) << 8);
        volume = envelope_initial_volume();

        if(has_sweep){
            shadow_register = period_divider;
            sweep_timer = get_sweep_pace() > 0 ? get_sweep_pace() : 8;
            sweep_pace = get_sweep_pace();
            if(get_sweep_pace() > 0 || get_sweep_individual_step() > 0){
                sweep_enabled = true;
            }
            else{
                sweep_enabled = false;
            }
            if(get_sweep_individual_step() > 0){
                calc_freq();
            }
        }
    }
}

void gba::PulseChannel::on_write_period_low(Byte val)
{
    period_low = val;
}

size_t gba::PulseChannel::duty()
{
    return size_t((length_duty & 0b11000000) >> 6);
}

void gba::PulseChannel::on_divider_clock()
{
    if(period_divider >= 2047){
        period_divider = HalfWord(period_low) | ((HalfWord(period_high_control) & 0b111) << 8);
        waveform_counter += 1;
        if(waveform_counter >= 8) waveform_counter -= 8;
        dac = WAVEFORM_PULSE[duty()][waveform_counter];
    }
    else{
        period_divider += 1;
    }
}

gba::WaveChannel::WaveChannel() :
dac_enable(false), initial_length_timer(0), output_level(0), period_low(0), period_high_control(0), period_divider(0), length_timer(0), volume(0), wave_ram_index(0),
length_enable(false), dac(0), sample_buffer(0)
{}

void gba::WaveChannel::reset()
{
    dac_enable= false;
    initial_length_timer= 0;
    output_level= 0;
    period_low= 0;
    period_high_control= 0;
    period_divider= 0;
    length_timer= 0;
    volume= 0;
    wave_ram_index= 0;
    length_enable= false;
    dac= 0;
}

bool gba::WaveChannel::active()
{
    bool l = length_enable ? length_timer > 0 : true;
    return dac_enable && l;
}

Byte gba::WaveChannel::get_output_level()
{
    return (output_level & 0b01100000) >> 5;
}

size_t gba::WaveChannel::get_output_shift()
{
    switch(volume){
        case 0b00: return 4;
        case 0b01: return 0;
        case 0b10: return 1;
        case 0b11: return 2;
        default:   return 4;
    }
}

void gba::WaveChannel::set_volume()
{
    volume = get_output_level();
}

void gba::WaveChannel::increment_length()
{
    if(length_timer > 0){
        length_timer -= 1;
    }
}

void gba::WaveChannel::on_write_control(Byte val)
{
    period_high_control = val;
    length_enable = (period_high_control & 64) > 0;
    if((val & 128) > 0 && dac_enable){
        if(length_timer == 0){
            length_timer = 256 - HalfWord(initial_length_timer);
        }
        period_divider = HalfWord(period_low) | ((HalfWord(period_high_control) & 0b111) << 8);
        set_volume();
        wave_ram_index = 0;
    }
}

Byte gba::WaveChannel::on_read_control()
{
    return period_high_control & 64;
}

void gba::WaveChannel::on_divider_clock()
{
    if(period_divider >= 2047){
        period_divider = HalfWord(period_low) | ((HalfWord(period_high_control) & 0b111) << 8);

        size_t index = wave_ram_index / 2;
        size_t nibble = wave_ram_index % 2;
        Byte sample = wave_ram[index];
        sample_buffer = nibble > 0 ? (sample & 0b1111) : (sample >> 4);
        dac = sample_buffer >> get_output_shift();

        wave_ram_index += 1;
        if(wave_ram_index >= 32){
            wave_ram_index -= 32;
        }
    }
    else{
        period_divider += 1;
    }
}

gba::NoiseChannel::NoiseChannel() :
length_timer_reg(0), vol_env(0), freq_rand(0), control(0), volume(0), envelope_counter(0), length_enabled(false), length_timer(0), dac(0), lfsr(0), internal_divider(0)
{}

bool gba::NoiseChannel::active()
{
    bool l = length_enabled ? length_timer > 0 : true;
    return (vol_env & 0xF8) > 0 && l;
}

bool gba::NoiseChannel::envelope_increase_volume()
{
    return (vol_env & 0b1000) > 0;
}

Byte gba::NoiseChannel::envelope_sweep_pace()
{
    return vol_env & 0b111;
}

Byte gba::NoiseChannel::envelope_initial_volume()
{
    return (vol_env & 0b11110000) >> 4;
}

Byte gba::NoiseChannel::get_clock_shift()
{
    return (freq_rand & 0b11110000) >> 4;
}

bool gba::NoiseChannel::get_lfsr_width()
{
    return (freq_rand & 0b00001000) > 0;
}

Byte gba::NoiseChannel::get_clock_divider()
{
    return freq_rand & 0b111;
}

void gba::NoiseChannel::clock_envelope()
{
    envelope_counter += 1;
    if(envelope_sweep_pace() > 0){
        if(envelope_counter % envelope_sweep_pace() == 0){
            if(envelope_increase_volume()){
                if(volume < 15){
                    volume += 1;
                }
            }
            else{
                if(volume > 0) volume--;
            }
        }
    }
}

void gba::NoiseChannel::increment_length()
{
    if(length_timer > 0){
        length_timer -= 1;
    }
}

void gba::NoiseChannel::on_write_control(Byte val)
{
    control = val;
    if((control & 128) > 0 && (vol_env & 0xF8) > 0){


        if(length_timer == 0){
            length_timer = 64 - (length_timer_reg & 0b00111111);
        }
        envelope_counter = 0;
        volume = envelope_initial_volume();
        lfsr = 0;
    }
    length_enabled = (control & 64) > 0;
}

Byte gba::NoiseChannel::on_read_control()
{
    return Byte(length_enabled) << 6;
}

void gba::NoiseChannel::on_divider_clock()
{
    internal_divider += 1;
    float divider = get_clock_divider() > 0 ? float(get_clock_divider()) : 0.5f;
    size_t division = size_t(std::ceilf(divider * std::ceilf(powf(2.0f, float(get_clock_shift())))));
    if (internal_divider % division == 0){
        lfsr_clock();
    }
}

void gba::NoiseChannel::lfsr_clock()
{
    const bool shifted_out = (lfsr & 1) == ((lfsr & 2) >> 1);
    lfsr = (lfsr & ~0x8000u) | (HalfWord(shifted_out) << 15);
    if(get_lfsr_width()){
        lfsr = (lfsr & ~128u) | (HalfWord(shifted_out) << 7);
    }
    lfsr >>= 1;
    dac = Byte(shifted_out);
}

void gba::DMASoundChannel::onTimerOverflow()
{
    dac = queue.size() > 0 ? std::bit_cast<int8_t>(queue.front()) : 0;
    if(queue.size() > 0) queue.pop();
    if(queue.size() <= 16){
        // Timing stimmt nicht ganz, sollte in den meisten Spielen passen
        if(bus->dma[1].DestinationAddress.raw == startAddress){
            bus->scheduler->scheduleEvent({.timePoint = 3, .type = EVENT_DmaTransfer, .args={.index = 1}});
        }
        else if(bus->dma[2].DestinationAddress.raw == startAddress){
            bus->scheduler->scheduleEvent({.timePoint = 3, .type = EVENT_DmaTransfer, .args={.index = 2}});
        }
    }
}

void gba::DMASoundChannel::onClockAPU()
{
    if(needsEnqueue){
        needsEnqueue = false;
        queue.push(FIFOData0);
        queue.push(FIFOData1);
        queue.push(FIFOData2);
        queue.push(FIFOData3);
        FIFOData0 = 0;
        FIFOData1 = 0;
        FIFOData2 = 0;
        FIFOData3 = 0;
    }
}

void gba::DMASoundChannel::reset()
{
    queue = std::queue<Byte>();
    FIFOData0 = 0;
    FIFOData1 = 0;
    FIFOData2 = 0;
    FIFOData3 = 0;
}

void gba::APU::onSOUNDCNT_H_Write()
{
    if(SOUNDCNT_H.state.resetFIFOA){
        SOUNDCNT_H.state.resetFIFOA = 0;
        A.reset();
    }
    if(SOUNDCNT_H.state.resetFIFOB){
        SOUNDCNT_H.state.resetFIFOB = 0;
        B.reset();
    }
}

void gba::APU::clockPSG()
{
    total_clocks += 1;
    if (total_clocks % 2 == 0){
        wave.on_divider_clock();
    }
    if (total_clocks % 4 == 0){
        pulse1.on_divider_clock();
        pulse2.on_divider_clock();
    }
    if(total_clocks % 16 == 0){ // ~262.000 Hz
        noise.on_divider_clock();
    }
    pulse1_sample = 0;
    pulse2_sample = 0;
    wave_sample = 0;
    noise_sample = 0;
    if(pulse1.active()){
        pulse1_sample = pulse1.dac;
    }
    if(pulse2.active()){
        pulse2_sample = pulse2.dac;
    }
    if(wave.active()){
        wave_sample = wave.dac;
    }
    if(noise.active()){
        noise_sample = noise.dac;
    }
}

void gba::APU::clockEnvelopes()
{
    pulse1.clock_envelope();
    pulse2.clock_envelope();
    noise.clock_envelope();
}

void gba::APU::clockLengthCounters()
{
    pulse1.increment_length();
    pulse2.increment_length();
    wave.increment_length();
    noise.increment_length();
}

void gba::APU::clockSweep()
{
    pulse1.clock_sweep();
}

void gba::APU::clockFromDMA()
{
    A.onClockAPU();
    B.onClockAPU();
}

void gba::APU::onWrite(Word addr, Byte val)
{
    if(addr == 0x4000060){
        pulse1.sweep = val;
    }
    else if(addr == 0x4000062){
        pulse1.length_duty = val;
        pulse1.length_timer = pulse1.length_timer_base();
    }
    else if(addr == 0x4000063){
        pulse1.vol_env = val;
    }
    else if(addr == 0x4000064){
        pulse1.on_write_period_low(val);
    }
    else if(addr == 0x4000065){
        pulse1.on_write_period_high_control(val);
    }

    else if(addr == 0x4000068){
        pulse2.length_duty = val;
        pulse2.length_timer = pulse2.length_timer_base();
    }
    else if(addr == 0x4000069){
        pulse2.vol_env = val;
    }
    else if(addr == 0x400006C){
        pulse2.on_write_period_low(val);
    }
    else if(addr == 0x400006D){
        pulse2.on_write_period_high_control(val);
    }

    else if(addr == 0x4000070){
        // Hier wird die extra Funktionalität für den Kanal ausgewählt, hab ich NICHT implementiert
        wave.dac_enable = (val & 128) > 0;
        waveStub = val;
    }
    else if(addr == 0x4000072){
        wave.initial_length_timer = val;
        wave.length_timer = 256 - HalfWord(wave.initial_length_timer);
    }
    else if(addr == 0x4000073){
        wave.output_level = val & 0b01100000;
        wave.set_volume();
    }
    else if(addr == 0x4000074){
        wave.period_low = val;
    }
    else if(addr == 0x4000075){
        wave.on_write_control(val);
    }

    else if(addr == 0x4000078){
        noise.length_timer_reg = val & 0b00111111;
        noise.length_timer = 64 - noise.length_timer_reg;
    }
    else if(addr == 0x4000079){
        noise.vol_env = val;
    }
    else if(addr == 0x400007C){
        noise.freq_rand = val;
    }
    else if(addr == 0x400007D){
        noise.on_write_control(val);
    }

    else if(addr == 0x4000080){
        SOUNDCNT_L.raw = (SOUNDCNT_L.raw & 0xFF00) | val;
    }
    else if(addr == 0x4000081){
        SOUNDCNT_L.raw = (SOUNDCNT_L.raw & 0x00FF) | (HalfWord(val) << 8);
    }
    else if(addr == 0x4000082){
        SOUNDCNT_H.raw = (SOUNDCNT_H.raw & 0xFF00) | val;
        onSOUNDCNT_H_Write();
    }
    else if(addr == 0x4000083){
        SOUNDCNT_H.raw = (SOUNDCNT_H.raw & 0x00FF) | (HalfWord(val) << 8);
        onSOUNDCNT_H_Write();
    }
    else if(addr == 0x4000084){
        psgFIFOMasterEnable = val & 128;
        if(!psgFIFOMasterEnable){
            SOUNDCNT_L = {};
            pulse1 = PulseChannel(true);
            pulse2 = PulseChannel(false);
            noise = NoiseChannel();
            wave = WaveChannel();
            waveStub = 0;
            A = DMASoundChannel(bus, 0x040000A0);
            B = DMASoundChannel(bus, 0x040000A4);
        }
    }
    else if(addr == 0x4000088){
        SOUNDBIAS.raw = (SOUNDBIAS.raw & 0xFF00) | val;
    }
    else if(addr == 0x4000089){
        SOUNDBIAS.raw = (SOUNDBIAS.raw & 0x00FF) | (HalfWord(val) << 8);
    }

    else if(addr >= 0x4000090 && addr <= 0x400009F){
        wave.wave_ram[addr - 0x4000090] = val;
    }
    else if(addr == 0x40000A0){
        A.FIFOData0 = val;
        A.needsEnqueue = true;
    }
    else if(addr == 0x40000A1){
        A.FIFOData1 = val;
        A.needsEnqueue = true;
    }
    else if(addr == 0x40000A2){
        A.FIFOData2 = val;
        A.needsEnqueue = true;
    }
    else if(addr == 0x40000A3){
        A.FIFOData3 = val;
        A.needsEnqueue = true;
    }
    else if(addr == 0x40000A4){
        B.FIFOData0 = val;
        B.needsEnqueue = true;
    }
    else if(addr == 0x40000A5){
        B.FIFOData1 = val;
        B.needsEnqueue = true;
    }
    else if(addr == 0x40000A6){
        B.FIFOData2 = val;
        B.needsEnqueue = true;
    }
    else if(addr == 0x40000A7){
        B.FIFOData3 = val;
        B.needsEnqueue = true;
    }
}

Byte gba::APU::onRead(Word addr)
{
    if(addr == 0x4000060){
        return pulse1.sweep;
    }
    else if(addr == 0x4000062){
        return pulse1.length_duty & 0b11000000;
    }
    else if(addr == 0x4000063){
        return pulse1.vol_env;
    }
    else if(addr == 0x4000064){
        // sicher?
        return 0xFF;
    }
    else if(addr == 0x4000065){
        return pulse1.period_high_control & 0b01000000;
    }

    else if(addr == 0x4000068){
        return pulse2.length_duty & 0b11000000;
    }
    else if(addr == 0x4000069){
        return pulse2.vol_env;
    }
    else if(addr == 0x400006C){
        //...
        return 0xFF;
    }
    else if(addr == 0x400006D){
        return pulse2.period_high_control & 0b01000000;
    }

    else if(addr == 0x4000070){
        return waveStub;
    }
    else if(addr == 0x4000072){
        return 0xFF;
    }
    else if(addr == 0x4000073){
        return wave.output_level;
    }
    else if(addr == 0x4000074){
        return 0xFF;
    }
    else if(addr == 0x4000075){
        wave.on_read_control();
    }

    else if(addr == 0x4000078){
        return 0xFF;
    }
    else if(addr == 0x4000079){
        return noise.vol_env;
    }
    else if(addr == 0x400007C){
        return noise.freq_rand;
    }
    else if(addr == 0x400007D){
        return noise.on_read_control();
    }

    else if(addr == 0x4000080){
        return SOUNDCNT_L.raw & 0xFF;
    }
    else if(addr == 0x4000081){
        return (SOUNDCNT_L.raw >> 8) & 0xFF;
    }
    else if(addr == 0x4000082){
        return SOUNDCNT_H.raw & 0xFF;
    }
    else if(addr == 0x4000083){
        return (SOUNDCNT_H.raw >> 8) & 0xFF;
    }
    else if(addr == 0x4000084){
        return (Byte(psgFIFOMasterEnable) << 7) | pulse1.active() | (pulse2.active() << 1) | (wave.active() << 2) | (noise.active() << 3);
    }
    else if(addr == 0x4000088){
        return SOUNDBIAS.raw & 0xFF;
    }
    else if(addr == 0x4000089){
        return (SOUNDBIAS.raw >> 8) & 0xFF;
    }

    else if(addr >= 0x4000090 && addr <= 0x400009F){
        return wave.wave_ram[addr - 0x4000090];
    }

    return 0;
}

std::pair<float, float> gba::APU::getSample()
{
    if(psgFIFOMasterEnable){
        int16_t pcmOutLeft = (A.dac << (1 + SOUNDCNT_H.state.volumeA)) * SOUNDCNT_H.state.enableALeft + (B.dac << (1 + SOUNDCNT_H.state.volumeB)) * SOUNDCNT_H.state.enableBLeft;
        pcmOutLeft = std::clamp(pcmOutLeft, (int16_t)-0x200, (int16_t)0x1FF);

        int16_t pcmOutRight = (A.dac << (1 + SOUNDCNT_H.state.volumeA)) * SOUNDCNT_H.state.enableARight + (B.dac << (1 + SOUNDCNT_H.state.volumeB)) * SOUNDCNT_H.state.enableBRight;
        pcmOutRight = std::clamp(pcmOutRight, (int16_t)-0x200, (int16_t)0x1FF);

        int16_t pulse1Sample = 2 * pulse1_sample - int16_t(pulse1.volume);
        int16_t pulse2Sample = 2 * pulse2_sample - int16_t(pulse2.volume);
        int16_t noiseSample = 2 * noise_sample - int16_t(noise.volume);
        int16_t waveSample = 2 * wave_sample - int16_t(15);

        const int16_t pulse1SampleLeft = SOUNDCNT_L.state.enablePulse1Left ? pulse1Sample : 0;
        const int16_t pulse2SampleLeft = SOUNDCNT_L.state.enablePulse2Left ? pulse2Sample : 0;
        const int16_t noiseSampleLeft = SOUNDCNT_L.state.enableNoiseLeft ? noiseSample : 0;
        const int16_t waveSampleLeft = SOUNDCNT_L.state.enableWaveLeft ? waveSample : 0;

        int16_t psgSampleLeft = (pulse1SampleLeft + pulse2SampleLeft + noiseSampleLeft + waveSampleLeft) * SOUNDCNT_L.state.psgVolumeLeft;

        psgSampleLeft >>= 2 - (SOUNDCNT_H.state.psgVolumeMaster % 3);

        const int16_t pulse1SampleRight = SOUNDCNT_L.state.enablePulse1Right ? pulse1Sample : 0;
        const int16_t pulse2SampleRight = SOUNDCNT_L.state.enablePulse2Right ? pulse2Sample : 0;
        const int16_t noiseSampleRight = SOUNDCNT_L.state.enableNoiseRight ? noiseSample : 0;
        const int16_t waveSampleRight = SOUNDCNT_L.state.enableWaveRight ? waveSample : 0;

        int16_t psgSampleRight = (pulse1SampleRight + pulse2SampleRight + noiseSampleRight + waveSampleRight) * SOUNDCNT_L.state.psgVolumeRight;

        psgSampleRight >>= 2 - (SOUNDCNT_H.state.psgVolumeMaster % 3);

        const int16_t signedOutLeft = std::clamp(pcmOutLeft + psgSampleLeft, -0x200, 0x1FF);
        // const int finalOutLeft = std::clamp(signedOutLeft + int(SOUNDBIAS.state.biasLevel), 0, 0x3FF);

        const int16_t signedOutRight = std::clamp(pcmOutRight + psgSampleRight, -0x200, 0x1FF);
        // const int finalOutRight = std::clamp(signedOutRight + int(SOUNDBIAS.state.biasLevel), 0, 0x3FF);

        return {float(signedOutLeft) / 512.0f, float(signedOutRight) / 512.0f};
    }
    else{
        return {0,0};
    }
}

void gba::APU::clockPCM()
{
    A.onClockAPU();
    B.onClockAPU();
}

void gba::APU::onTimerOverflow(int index)
{
    if(psgFIFOMasterEnable){
        if(index == 0 && !SOUNDCNT_H.state.useTimer1A) A.onTimerOverflow();
        if(index == 1 && SOUNDCNT_H.state.useTimer1A) A.onTimerOverflow();
        if(index == 0 && !SOUNDCNT_H.state.useTimer1B) B.onTimerOverflow();
        if(index == 1 && SOUNDCNT_H.state.useTimer1B) B.onTimerOverflow();
    }
}
