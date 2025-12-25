#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "tinybit.h"

#define M_PI 3.14159265358979323846
#define GAIN 500

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

char audio_string_buffer[256];

struct needle {
    char* string_pos;
    TONE tone_note;
    uint8_t octave_note;
    DURATION duration_note;
    WAVEFORM waveform_note;
    uint32_t sample_processed_note;
    uint32_t total_samples_note;
    float phase;
};

struct needle audio_needle;

void tb_audio_init(){
    strncpy(audio_string_buffer, "S:C4:4 Q:E4:8 N:0:8", sizeof(audio_string_buffer));
    audio_needle.string_pos = audio_string_buffer;
    audio_needle.sample_processed_note = 0;
    audio_needle.tone_note = F;
    audio_needle.octave_note = 3;
    audio_needle.duration_note = WHOLE;
    audio_needle.total_samples_note = ((60000 / bpm) / 8) * (16/audio_needle.duration_note) * (TB_AUDIO_SAMPLE_RATE / 1000);
}

// Generate and queue a sine wave audio buffer for playback
void queue_freq_sin(float freq, int ms, int vol, int chan) {
    if(vol < 0 || vol > 10 || chan < 0 || chan > 3) {
        return;
    }
    float x = 0;
    int samples = (44100/1000) * ms;
    int16_t* buffer = (int16_t*)calloc(samples, sizeof(int16_t));
    if (!buffer) return; // Memory allocation check
    
    for (int i = 0; i < samples; i++) {
        x += 2 * M_PI * freq / 44100;
        buffer[i] = sin(x) * GAIN * vol;
    }
    // Audio playback would happen here
    free(buffer);
}

// Generate and queue a sawtooth wave audio buffer for playback
void queue_freq_saw(float freq, int ms, int vol, int chan) {
    if(vol < 0 || vol > 10 || chan < 0 || chan > 3) {
        return;
    }
    float x = 0;
    int samples = (44100/1000) * ms;
    int16_t* buffer = (int16_t*)malloc(samples * sizeof(int16_t));
    if (!buffer) return; // Memory allocation check
    
    for (int i = 0; i < samples; i++) {
        x += freq / 44100;
        if (x >= 1.0f) x -= 1.0f;
        buffer[i] = (x * 2 - 1) * GAIN * vol; 
    }
    // Audio playback would happen here
    free(buffer);
}

// Generate and queue a square wave audio buffer for playback
void queue_freq_square(float freq, int ms, int vol, int chan) {
    if(vol < 0 || vol > 10 || chan < 0 || chan > 3) {
        return;
    }
    float x = 0;
    int samples = (44100/1000) * ms;
    int16_t* buffer = (int16_t*)malloc(samples * sizeof(int16_t));
    if (!buffer) return; // Memory allocation check
    
    for (int i = 0; i < samples; i++) {
        x += freq / 44100;
        if (x >= 1.0f) x -= 1.0f;
        buffer[i] = (x < 0.5f ? -1 : 1) * GAIN * vol; 
    }
    // Audio playback would happen here
    free(buffer);
}

// Play white noise for specified duration (placeholder - not implemented)
void play_noise(int eights, int vol, int chan) {
    //if(vol < 0 || vol > 10 || chan < 0 || chan > 3) {
    //    return;
    //}
    //int ms = ((60000 / bpm) / 8) * eights;
    //int samples = (44100/1000) * ms;
    //int16_t* buffer = (int16_t*)malloc(samples * sizeof(int16_t));
    //if (!buffer) return; // Memory allocation check
    //
    //for (int i = 0; i < samples; i++) {
    //    buffer[i] = (rand() % ((GAIN * vol) * 2)) - (GAIN * vol); 
    //}
    //// Audio playback would happen here
    //free(buffer);
}

// Play a musical tone with specified parameters (placeholder - not implemented)
void play_tone(TONE tone, int octave, int eights, WAVEFORM w, int vol, int chan) {
    // Validate parameters
    // if (octave < 0 || octave > 6 || tone < 0 || tone > 11 || eights < 0 || 
    //     vol < 0 || vol > 10 || chan < 0 || chan > 3) {
    //     return;
    // }

    // int ms = ((60000 / bpm) / 8) * eights;
    float freq = frequencies[tone][octave];

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

    // float x = 0;
    // for (int i = 0; i < TB_AUDIO_FRAME_SAMPLES; i++) {
    //     x += freq / TB_AUDIO_SAMPLE_RATE;
    //     if (x >= 1.0f) x -= 1.0f;
    //     tinybit_audio_buffer[i] = (uint8_t)(x < 0.5f ? -1 : 1) * GAIN * vol; 
    // }
    audio_needle.sample_processed_note = 0;
}

// Process audio for the current frame (placeholder - not implemented)
void process_audio() {
    // for now just use the current note, string processing will come later
    float x = audio_needle.phase;
    printf("new frame\n");


    if(audio_needle.sample_processed_note >= audio_needle.total_samples_note) {
        // printf("Note finished\n");
        return; // note finished
    } else {
        // generate square wave for current note
        float freq = frequencies[audio_needle.tone_note][audio_needle.octave_note];
        for (int i = 0; i < TB_AUDIO_FRAME_SAMPLES; i++) {
            if(audio_needle.sample_processed_note >= audio_needle.total_samples_note) {
                tinybit_audio_buffer[i] = 0; // silence after note ends
            } else {
                x += freq / TB_AUDIO_SAMPLE_RATE;
                if (x >= 1.0f) x -= 1.0f;
                printf("Generating sample %f %f %d\n", freq, x, (x < 0.5f ? -1 : 1) * 127);
                tinybit_audio_buffer[i] = (int8_t)(x < 0.5f ? -1 : 1) * 127; 
                audio_needle.sample_processed_note++;
            }
        }
        audio_needle.phase = x;
        // printf("Sample processed: %u / %u\n", audio_needle.sample_processed_note, audio_needle.total_samples_note);
    }
}

// Set the beats per minute for timing calculations
void set_bpm(int new_bpm) {
    if (new_bpm > 0) {
        bpm = new_bpm;
    }
}

// Set the current audio channel
void set_channel(int new_chan) {
    if(new_chan >= 0 && new_chan <= 3) {
        channel = new_chan;
    }
}

// Set the global audio volume
void set_volume(int new_vol) {
    if(new_vol >= 0 && new_vol <= 10) {
        volume = new_vol;
    }
}