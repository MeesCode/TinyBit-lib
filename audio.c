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
    bool repeat;
    bool stop;
};

struct needle audio_needle;

char *strnstr(const char *s, const char *find, size_t slen)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != '\0') {
		len = strlen(find);
		do {
			do {
				if (slen-- < 1 || (sc = *s++) == '\0')
					return (NULL);
			} while (sc != c);
			if (len > slen)
				return (NULL);
		} while (strncmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}

void tb_audio_init(){
    strncpy(audio_string_buffer, "S:C4:2 S:E4:2 S:F4:2", sizeof(audio_string_buffer));
    memset(&audio_needle, 0, sizeof(struct needle));
    audio_needle.repeat = true;
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

    // float x = 0;
    // for (int i = 0; i < TB_AUDIO_FRAME_SAMPLES; i++) {
    //     x += freq / TB_AUDIO_SAMPLE_RATE;
    //     if (x >= 1.0f) x -= 1.0f;
    //     tinybit_audio_buffer[i] = (uint8_t)(x < 0.5f ? -1 : 1) * GAIN * vol; 
    // }
    // audio_needle.sample_processed_note = 0;
    // audio_needle.phase = 0;
}

void get_next_note_from_string() {

    if(audio_needle.stop) {
        printf("Audio needle stopped.\n");
        return;
    }

    // printf("Getting next note from string at pos: %ld\n", audio_needle.string_pos - audio_string_buffer);

    char* pos = audio_needle.string_pos;

    // needle not yet started
    if (!pos) {
        // printf("starting audio string.\n");
        pos = audio_string_buffer;
        audio_needle.string_pos = audio_string_buffer;
        audio_needle.stop = false;
    } else {
        pos = strnstr(pos, " ", 7); // skip to next note

        // check for end of string
        if (!pos || *pos == '\0') {
            // loop or stop
            if(audio_needle.repeat) {
                // printf("Looping audio string.\n");
                pos = audio_string_buffer;
                audio_needle.string_pos = audio_string_buffer;
            } else {
                // printf("Stopping audio needle.\n");
                audio_needle.stop = true;
                return;
            }
        }else {
            pos += 1; // move to character after space
        }

    }


    // parse waveform
    switch(*pos) {
        case 'S': audio_needle.waveform_note = SINE; break;
        case 'Q': audio_needle.waveform_note = SQUARE; break;
        case 'W': audio_needle.waveform_note = SAW; break;
        case 'N': audio_needle.waveform_note = NOISE; break;
        default: 
            printf("Invalid waveform character: %c\n", *pos);
            audio_needle.stop = true; return; // invalid waveform
    }

    // parse tone
    pos = strnstr(pos, ":", 4) + 1; // move to character after ':'
    switch(*pos) {
        case 'C': audio_needle.tone_note = C; break;
        case 'D': audio_needle.tone_note = D; break;
        case 'E': audio_needle.tone_note = E; break;
        case 'F': audio_needle.tone_note = F; break;
        case 'G': audio_needle.tone_note = G; break;
        case 'A': audio_needle.tone_note = A; break;
        case 'B': audio_needle.tone_note = B; break;
        default: 
            printf("Invalid tone character: %c\n", *pos);
            audio_needle.stop = true; return; // invalid tone
    }

    // check for sharp/flat
    pos += 1; 
    if (*pos == 's') {
        audio_needle.tone_note = (audio_needle.tone_note + 1) % 12;
        pos += 1; // move position forward
    } if( *pos == 'b') {
        audio_needle.tone_note = (audio_needle.tone_note - 1) % 12;
        pos += 1; // move position forward
    }

    // parse octave
    audio_needle.octave_note = *pos - '0';

    // parse duration
    pos = strnstr(pos, ":", 4) + 1; // move to character after ':'
    switch(*pos) {
        case '1': 
            if(* (pos + 1) == '6') {
                audio_needle.duration_note = SIXTEENTH; 
            } else {
                audio_needle.duration_note = WHOLE; 
            }
        break;
        case '2': audio_needle.duration_note = HALF; break;
        case '4': audio_needle.duration_note = QUARTER; break;
        case '8': audio_needle.duration_note = EIGHTH; break;
        default: 
            printf("Invalid duration character: %c\n", *pos);
            audio_needle.stop = true; return; // invalid duration
    }

    // calculate total samples for the note
    audio_needle.total_samples_note = ((60000 / bpm) / 16) * (16/audio_needle.duration_note) * (TB_AUDIO_SAMPLE_RATE / 1000);
    audio_needle.sample_processed_note = 0;
    audio_needle.string_pos = pos;
    // audio_needle.phase = 0;

    // printf("Next note: Waveform %d, Tone %d, Octave %d, Duration %d, Total Samples %d\n", 
    //     audio_needle.waveform_note,
    //     audio_needle.tone_note,
    //     audio_needle.octave_note,
    //     audio_needle.duration_note,
    //     audio_needle.total_samples_note
    // );
}

// Process audio for the current frame (placeholder - not implemented)
void process_audio() {
    float x = audio_needle.phase;

    if(audio_needle.sample_processed_note >= audio_needle.total_samples_note) {
        get_next_note_from_string();
    } else {
        float freq = frequencies[audio_needle.tone_note][audio_needle.octave_note];
        for (int i = 0; i < TB_AUDIO_FRAME_SAMPLES; i++) {
            if(audio_needle.sample_processed_note >= audio_needle.total_samples_note) {
                tinybit_audio_buffer[i] = 0; // silence after note ends
            } else {
                x += 2 * M_PI * freq / TB_AUDIO_SAMPLE_RATE;
                tinybit_audio_buffer[i] = (int16_t)(sin(x) * 4000); // 4000 is the gain for now
                //printf("Generating sample %f %f %d\n", freq, x, tinybit_audio_buffer[i]);
                audio_needle.sample_processed_note++;
            }
        }
        audio_needle.phase = x;
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