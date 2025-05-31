

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "audio.h"

#define M_PI 3.14159265358979323846
#define GAIN 500

// SDL_AudioDeviceID audio_device;
// SDL_AudioSpec audio_spec;
int bpm = 100;
int channel = 0;
int volume = 10;


const float frequencies[12][7] = {
    { 25.96f, 51.91f, 103.83f, 207.65f, 415.30f, 830.61f, 1661.22f },
    { 27.50f, 55.00f, 110.00f, 220.00f, 440.00f, 880.00f, 1760.00f },
    { 29.14f, 58.27f, 116.54f, 233.08f, 466.16f, 932.33f, 1864.66f },
    { 30.87f, 61.74f, 123.47f, 246.94f, 493.88f, 987.77f, 1975.53f },
    { 32.70f, 65.41f, 130.81f, 261.63f, 523.25f, 1046.50f, 2093.00f },
    { 34.65f, 69.30f, 138.59f, 277.18f, 554.37f, 1108.73f, 2217.46f },
    { 36.71f, 73.42f, 146.83f, 293.66f, 587.33f, 1174.66f, 2349.32f },
    { 38.89f, 77.78f, 155.56f, 311.13f, 622.25f, 1244.51f, 2489.02f },
    { 41.20f, 82.41f, 164.81f, 329.63f, 659.26f, 1318.51f, 2637.02f },
    { 43.65f, 87.31f, 174.61f, 349.23f, 698.46f, 1396.91f, 2793.83f },
    { 46.25f, 92.50f, 185.00f, 369.99f, 739.99f, 1479.98f, 2959.96f },
    { 49.00f, 98.00f, 196.00f, 392.00f, 783.99f, 1567.98f, 3135.96f },
};

void audio_init(){
    // set up audio
    // SDL_Init(SDL_INIT_AUDIO);
    // Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024);
    // Mix_AllocateChannels(4);
}

void queue_freq_sin(float freq, int ms, int vol, int chan) {
    if(vol < 0 || vol > 10 || chan < 0 || chan > 3) {
        return;
    }
    float x = 0;
    int samples = (44100/1000) * ms;
    int16_t* buffer = (int16_t*)calloc(samples, sizeof(int16_t));
    for (int i = 0; i < samples; i++) {
        x += 2 * M_PI * freq / 44100;
        buffer[i] = sin(x) * GAIN * vol;
    }
    // Mix_Chunk* chunk = Mix_QuickLoad_RAW((Uint8*)buffer, samples * sizeof(int16_t));
    // Mix_PlayChannel(chan, chunk, 0);
}

void queue_freq_saw(float freq, int ms, int vol, int chan) {
    if(vol < 0 || vol > 10 || chan < 0 || chan > 3) {
        return;
    }
    float x = 0;
    int samples = (44100/1000) * ms;
    int16_t* buffer = (int16_t*)malloc(samples * sizeof(int16_t));
    for (int i = 0; i < samples; i++) {
        x += freq / 44100;
        if (x >= 1.0f) x -= 1.0f;
        buffer[i] = (x * 2 - 1) * GAIN * vol; 
    }
    // Mix_Chunk* chunk = Mix_QuickLoad_RAW((Uint8*)buffer, samples * sizeof(int16_t));
    // Mix_PlayChannel(chan, chunk, 0);
}

void queue_freq_square(float freq, int ms, int vol, int chan) {
    if(vol < 0 || vol > 10 || chan < 0 || chan > 3) {
        return;
    }
    float x = 0;
    int samples = (44100/1000) * ms;
    int16_t* buffer = (int16_t*)malloc(samples * sizeof(int16_t));
    for (int i = 0; i < samples; i++) {
        x += freq / 44100;
        if (x >= 1.0f) x -= 1.0f;
        buffer[i] = (x < 0.5f ? -1 : 1) * GAIN * vol; 
    }
    // Mix_Chunk* chunk = Mix_QuickLoad_RAW((Uint8*)buffer, samples * sizeof(int16_t));
    // Mix_PlayChannel(chan, chunk, 0);
}

void play_noise(int eights, int vol, int chan) {
    // if(vol < 0 || vol > 10 || chan < 0 || chan > 3) {
    //     return;
    // }
    // int ms = ((60000 / bpm) / 8) * eights;
    // int samples = (44100/1000) * ms;
    // int16_t* buffer = (int16_t*)malloc(samples * sizeof(int16_t));
    // for (int i = 0; i < samples; i++) {
    //     buffer[i] = (rand() % ((GAIN * vol) * 2)) - (GAIN * vol); 
    // }
    // Mix_Chunk* chunk = Mix_QuickLoad_RAW((Uint8*)buffer, samples * sizeof(int16_t));
    // Mix_PlayChannel(chan, chunk, 0);
}

void play_tone(TONE tone, int octave, int eights, WAVEFORM w, int vol, int chan) {

    // // tone 
    // if (octave < 0 || octave > 6 || tone < 0 || tone > 11 || eights < 0 || vol < 0 || vol > 10 || chan < 0 || chan > 3) {
    //     return;
    // }


    // int ms = ((60000 / bpm) / 8) * eights;
    // float freq = frequencies[tone][octave];

    // switch (w) {
    // case SINE:
    //     queue_freq_sin(freq, ms, vol, chan);
    //     break;
    // case SAW:
    //     queue_freq_saw(freq, ms, vol, chan);
    //     break;
    // case SQUARE:
    //     queue_freq_square(freq, ms, vol, chan);
    //     break;
    // }
}

void set_bpm(int new_bpm) {
    bpm = new_bpm;
}

void set_channel(int new_chan) {
    if(new_chan < 0 || new_chan > 3) {
        return;
    }
    channel = new_chan;
}

void set_volume(int new_vol) {
    if(new_vol < 0 || new_vol > 10) {
        return;
    }
    volume = new_vol;
}