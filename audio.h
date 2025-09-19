#ifndef AUDIO_H
#define AUDIO_H

typedef enum {
	Ab = 0,
	A = 1,
	As = 2, Bb = 2,
	B = 3,
	C = 4,
	Cs = 5, Db = 5,
	D = 6,
	Ds = 7, Eb = 7,
	E = 8,
	F = 9,
	Fs = 10, Gb = 10,
	G = 11, Gs = 11,
} TONE;

typedef enum {
	SINE,
	SAW,
	SQUARE
} WAVEFORM;

// extern SDL_AudioDeviceID audio_device;
// extern SDL_AudioSpec audio_spec;
extern int bpm;
extern int channel;
extern int volume;

// Audio function declarations
void lua_setup_audio();
void audio_init();
void set_bpm(int);
void set_channel(int);
void set_volume(int);
void play_tone(TONE tone, int octave, int eights, WAVEFORM w, int, int);
void play_noise(int eights, int, int);

#endif