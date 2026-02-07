#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>

typedef enum {
	SINE,
	SAW,
	SQUARE,
	NOISE,
	REST
} WAVEFORM;

extern int bpm;
extern int channel;
extern int volume;

// Audio initialization and processing
void tb_audio_init();
void process_audio();

// BPM control
void set_bpm(int new_bpm);

// ABC notation loading
// channel_num: 0-3, abc_string: ABC notation, waveform: synth type, repeat: loop playback
int audio_load_abc(int channel_num, const char *abc_string, WAVEFORM waveform, bool repeat);

// Channel control
void audio_stop_channel(int channel_num);
void audio_stop_all();

#endif
