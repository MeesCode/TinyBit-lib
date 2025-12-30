#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "tinybit.h"
#include "helpers.h"

#define M_PI 3.14159265358979323846
#define GAIN 4000
#define ENVELOPE_MS 40
#define ENVELOPE_SAMPLES ((TB_AUDIO_SAMPLE_RATE / 1000) * ENVELOPE_MS) // 40ms just sounds pleasant

int bpm = 150;
int channel = 0;
int volume = 10;

const float frequencies[12][7] = {
    { 25.96f, 51.91f, 103.83f, 207.65f, 415.30f, 830.61f,  1661.22f },
    { 27.50f, 55.00f, 110.00f, 220.00f, 440.00f, 880.00f,  1760.00f },
    { 29.14f, 58.27f, 116.54f, 233.08f, 466.16f, 932.33f,  1864.66f },
    { 30.87f, 61.74f, 123.47f, 246.94f, 493.88f, 987.77f,  1975.53f },
    { 32.70f, 65.41f, 130.81f, 261.63f, 523.25f, 1046.50f, 2093.00f },
    { 34.65f, 69.30f, 138.59f, 277.18f, 554.37f, 1108.73f, 2217.46f },
    { 36.71f, 73.42f, 146.83f, 293.66f, 587.33f, 1174.66f, 2349.32f },
    { 38.89f, 77.78f, 155.56f, 311.13f, 622.25f, 1244.51f, 2489.02f },
    { 41.20f, 82.41f, 164.81f, 329.63f, 659.26f, 1318.51f, 2637.02f },
    { 43.65f, 87.31f, 174.61f, 349.23f, 698.46f, 1396.91f, 2793.83f },
    { 46.25f, 92.50f, 185.00f, 369.99f, 739.99f, 1479.98f, 2959.96f },
    { 49.00f, 98.00f, 196.00f, 392.00f, 783.99f, 1567.98f, 3135.96f },
};

struct needle {
    char audio_string_buffer[256];
    char* string_pos;
    TONE tone_note;
    uint8_t octave_note;
    DURATION duration_note;
    WAVEFORM waveform_note;
    uint32_t sample_processed_note;
    uint32_t total_samples_note;
    float frequency_note;
    float phase;
    bool repeat;
    bool stop;
};

struct needle n1 = {0};
struct needle n2 = {0};
struct needle n3 = {0};
struct needle sfx = {0};
struct needle audio_needles[4] = {0};

void tb_audio_init(){
    audio_needles[0] = n1;
    audio_needles[1] = n2;
    audio_needles[2] = n3;
    audio_needles[3] = sfx;
    strncpy(audio_needles[0].audio_string_buffer, "Q:E4:8 Q:E4:8 R:A0:8 Q:E4:8 R:A0:8 Q:C4:8 Q:E4:8 R:A0:8 Q:G4:8 R:A0:8 R:A0:4 Q:G4:8 R:A0:8 R:A0:4 Q:C4:8 R:A0:8 Q:G4:8 R:A0:8 Q:E4:8 R:A0:8 Q:A4:8 R:A0:8 Q:B4:8 R:A0:8 Q:As4:8 Q:A4:8 R:A0:8", sizeof(audio_needles[0].audio_string_buffer));
    strncpy(audio_needles[1].audio_string_buffer, "W:G3:8 W:G3:8 R:A0:8 W:G3:8 R:A0:8 W:G3:8 W:G3:8 R:A0:8 W:G3:8 R:A0:8 R:A0:4 W:G2:8 R:A0:8 R:A0:4 W:E3:8 R:A0:8 W:C3:8 R:A0:8 W:G2:8 R:A0:8 W:C3:8 R:A0:8 W:D3:8 R:A0:8 W:C#3:8 W:C3:8 R:A0:8", sizeof(audio_needles[1].audio_string_buffer));
    //strncpy(audio_needles[2].audio_string_buffer, "N:A0:16 R:A0:16 N:A0:8 N:A0:8 R:A0:16 N:A0:16 R:A0:16 N:A0:16 R:A0:16", sizeof(audio_needles[1].audio_string_buffer));
    audio_needles[0].repeat = true;
    audio_needles[1].repeat = true;
    audio_needles[2].repeat = true;
}

void get_next_note_from_string(struct needle* audio_needle) {

    if(audio_needle->stop) {
        return;
    }

    //printf("new note\n");

    char* pos = audio_needle->string_pos;

    // needle not yet started
    if (!pos) {
        pos = audio_needle->audio_string_buffer;
        audio_needle->string_pos = audio_needle->audio_string_buffer;
        audio_needle->stop = false;

        // no audio string
        if(*pos == '\0') {
            audio_needle->stop = true;
            return;
        }

    } else {
        pos = strnstr(pos, " ", 7); // skip to next note

        // check for end of string
        if (!pos || *pos == '\0') {
            // loop or stop
            if(audio_needle->repeat) {
                pos = audio_needle->audio_string_buffer;
                audio_needle->string_pos = audio_needle->audio_string_buffer;
            } else {
                audio_needle->stop = true;
                return;
            }
        }else {
            pos += 1; // move to character after space
        }
    }

    // parse waveform
    switch(*pos) {
        case 'S': audio_needle->waveform_note = SINE; break;
        case 'Q': audio_needle->waveform_note = SQUARE; break;
        case 'W': audio_needle->waveform_note = SAW; break;
        case 'N': audio_needle->waveform_note = NOISE; break;
        case 'R': audio_needle->waveform_note = REST; break;
        default: 
            printf("Invalid waveform character: %c\n", *pos);
            audio_needle->stop = true; return; // invalid waveform
    }

    // parse tone
    pos = strnstr(pos, ":", 4) + 1; // move to character after ':'
    switch(*pos) {
        case 'C': audio_needle->tone_note = C; break;
        case 'D': audio_needle->tone_note = D; break;
        case 'E': audio_needle->tone_note = E; break;
        case 'F': audio_needle->tone_note = F; break;
        case 'G': audio_needle->tone_note = G; break;
        case 'A': audio_needle->tone_note = A; break;
        case 'B': audio_needle->tone_note = B; break;
        default: 
            printf("Invalid tone character: %c\n", *pos);
            audio_needle->stop = true; return; // invalid tone
    }

    // check for sharp/flat
    pos += 1; 
    if (*pos == 's' || *pos == '#') {
        audio_needle->tone_note = (audio_needle->tone_note + 1) % 12;
        pos += 1; // move position forward
    } if( *pos == 'b' || *pos == 'f') {
        audio_needle->tone_note = (audio_needle->tone_note - 1) % 12;
        pos += 1; // move position forward
    }

    // parse octave
    audio_needle->octave_note = *pos - '0';
    if(audio_needle->octave_note < 0 || audio_needle->octave_note > 6) {
        printf("Invalid octave character: %c\n", *pos);
        audio_needle->stop = true; return; // invalid octave
    }

    // parse duration
    pos = strnstr(pos, ":", 4) + 1; // move to character after ':'
    switch(*pos) {
        case '1': 
            if(* (pos + 1) == '6') {
                audio_needle->duration_note = SIXTEENTH; 
            } else {
                audio_needle->duration_note = WHOLE; 
            }
        break;
        case '2': audio_needle->duration_note = HALF; break;
        case '4': audio_needle->duration_note = QUARTER; break;
        case '8': audio_needle->duration_note = EIGHTH; break;
        default: 
            printf("Invalid duration character: %c\n", *pos);
            audio_needle->stop = true; return; // invalid duration
    }

    // calculate total samples for the note
    audio_needle->frequency_note = frequencies[audio_needle->tone_note][audio_needle->octave_note];
    audio_needle->total_samples_note = ((60000 / bpm) / 16) * (16-audio_needle->duration_note) * (TB_AUDIO_SAMPLE_RATE / 1000);
    audio_needle->sample_processed_note = 0;
    audio_needle->string_pos = pos;

    // printf("Next note: Waveform %d, Tone %d, Octave %d, Duration %d, Total Samples %d\n", 
    //     audio_needle->waveform_note,
    //     audio_needle->tone_note,
    //     audio_needle->octave_note,
    //     audio_needle->duration_note,
    //     audio_needle->total_samples_note
    // );
}

// Process audio for the current frame (placeholder - not implemented)
void process_audio() {

    memset(tinybit_audio_buffer, 0, TB_AUDIO_FRAME_BUFFER_SIZE);

    for(int ch = 0; ch < 4; ch++) {
        struct needle *audio_needle = &audio_needles[ch];

        float phase = audio_needle->phase;
        //printf("new frame audio processing at phase %f\n", x);

        for (int i = 0; i < TB_AUDIO_FRAME_SAMPLES; i++) {

            // grab next note
            if(audio_needle->sample_processed_note >= audio_needle->total_samples_note) {
                get_next_note_from_string(audio_needle);
            }

            int gain = GAIN;
            // decay
            if(audio_needle->total_samples_note - audio_needle->sample_processed_note < ENVELOPE_SAMPLES) {
                gain = (gain * (audio_needle->total_samples_note - audio_needle->sample_processed_note)) / ENVELOPE_SAMPLES;
            } 
            // attack
            else if(audio_needle->sample_processed_note < ENVELOPE_SAMPLES) {
                gain = (gain * audio_needle->sample_processed_note) / ENVELOPE_SAMPLES;
            }

            // update phase
            phase += audio_needle->frequency_note / TB_AUDIO_SAMPLE_RATE;
            if (phase >= 1.0f) phase -= 1.0f;

            switch (audio_needle->waveform_note) {
            case SQUARE:
                tinybit_audio_buffer[i] += (int16_t)((phase < 0.5f ? -1 : 1) * gain);
                break;
            case SAW:
                tinybit_audio_buffer[i] += (int16_t)((phase * 2 - 1) * gain);
                break;
            case SINE:
                tinybit_audio_buffer[i] += (int16_t)((sin(2 * M_PI * phase)) * gain);
                break;
            case NOISE:
                tinybit_audio_buffer[i] += (int16_t)((rand() / (float)RAND_MAX * 2.0f - 1.0f) * gain);
                break;
            case REST:
                // For REST, no sound is added, so just break
                break;
            }

            audio_needle->sample_processed_note++;
        }
        audio_needle->phase = phase;
    }
}

// Set the beats per minute for timing calculations
void set_bpm(int new_bpm) {
    if (new_bpm > 0) {
        bpm = new_bpm;
    }
}