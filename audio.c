#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "audio.h"
#include "tinybit.h"
#include "ABC-parser/abc_parser.h"

#define M_PI 3.14159265358979323846
#define GAIN 4000
#define ENVELOPE_MS 20
#define ENVELOPE_SAMPLES ((TB_AUDIO_SAMPLE_RATE / 1000) * ENVELOPE_MS)

#define NUM_CHANNELS 4

int bpm = 150;
int channel = 0;
int volume = 10;

// Channel state for playback
struct channel_state {
    struct sheet sheet;
    struct note *current_note;
    uint32_t sample_processed;
    uint32_t total_samples;
    float frequency;
    float phase;
    WAVEFORM waveform;
    bool repeat;
    bool active;
};

// Static allocation for ABC parser
static NotePool note_pools[NUM_CHANNELS];
static struct channel_state channels[NUM_CHANNELS];

// Convert ABC parser frequency to our format
static float get_frequency_from_note(struct note *n) {
    if (!n || n->note_name == NOTE_REST) return 0.0f;
    return n->frequency_x10 / 10.0f;
}

// Calculate samples for a note based on its duration in ms
static uint32_t duration_ms_to_samples(uint16_t duration_ms) {
    return (uint32_t)duration_ms * TB_AUDIO_SAMPLE_RATE / 1000;
}

void tb_audio_init() {
    // Initialize all note pools and channels
    for (int i = 0; i < NUM_CHANNELS; i++) {
        note_pool_init(&note_pools[i]);
        sheet_init(&channels[i].sheet, &note_pools[i]);
        channels[i].current_note = NULL;
        channels[i].sample_processed = 0;
        channels[i].total_samples = 0;
        channels[i].frequency = 0.0f;
        channels[i].phase = 0.0f;
        channels[i].waveform = SQUARE;
        channels[i].repeat = true;
        channels[i].active = false;
    }

    // Example ABC notation - Super Mario Bros theme approximation
    const char *abc_channel0 =
        "X:1\n"
        "T:Super Mario Theme\n"
        "M:4/4\n"
        "L:1/8\n"
        "Q:1/4=105\n"
        "K:G\n"
        "V:SINE\n"
        "[e/2c/2][ce][ec][c/2A/2][ce] g/2z3z/2|c/2zG/2 zE/2zAB^A/2=A| (3Geg a=f/2gec/2 d/2B/2z|c/2zG/2 zE/2zAB^A/2=A|\n"
        "V:SQUARE\n"
        "E4 G4 | C4 z4 | G4 D4 | C4 z4 | \n"
        "V:SINE\n"
        "(3Geg a=f/2gec/2 d/2B/2z|zg/2^f/2 =f/2^de^G/2A/2cA/2c/2=d/2|zg/2^f/2 =f/2^dec'c'/2 c'/2z3/2|zg/2^f/2 =f/2^de^G/2A/2cA/2c/2=d/2|\n"
        "V:SQUARE\n"
        "G4 D4 | z G3 E4 | z G3 c4 | z G3 E4 | \n"
        "V:SINE\n"
        "z^d/2z=d/2z c/2z3z/2|]\n"
        "V:SQUARE\n"
        "z ^D3 C/z3z/2|]\n";

    const char *abc_channel1 =
        "X:1\n"
        "T:Channel 2\n"
        "M:4/4\n"
        "L:1/8\n"
        "Q:200\n"
        "K:C\n"
        "G, G, z G, z G, G, z | G, z z2 G,, z z2 |\n"
        "E, z C, z G,, z | C, z D, z ^C, C, z |\n";

    // Parse ABC for channel 0
    if (abc_parse(&channels[0].sheet, abc_channel0) == 0) {
        channels[0].current_note = sheet_first_note(&channels[0].sheet);
        channels[0].waveform = SINE;
        channels[0].repeat = true;
        channels[0].active = true;
        if (channels[0].current_note) {
            channels[0].frequency = get_frequency_from_note(channels[0].current_note);
            channels[0].total_samples = duration_ms_to_samples(channels[0].current_note->duration_ms);
        }
    }

    // Parse ABC for channel 1
    if (abc_parse(&channels[1].sheet, abc_channel1) == 0) {
        channels[1].current_note = sheet_first_note(&channels[1].sheet);
        channels[1].waveform = SINE;
        channels[1].repeat = true;
        channels[1].active = false;
        if (channels[1].current_note) {
            channels[1].frequency = get_frequency_from_note(channels[1].current_note);
            channels[1].total_samples = duration_ms_to_samples(channels[1].current_note->duration_ms);
        }
    }
}

// Advance to the next note in the channel
static void advance_to_next_note(struct channel_state *ch) {
    if (!ch->active) return;

    struct note *next = note_next(&ch->sheet, ch->current_note);

    // If no next note and repeat is enabled, loop back to start
    if (!next && ch->repeat) {
        next = sheet_first_note(&ch->sheet);
    }

    if (next) {
        ch->current_note = next;
        ch->frequency = get_frequency_from_note(next);
        ch->total_samples = duration_ms_to_samples(next->duration_ms);
        ch->sample_processed = 0;
    } else {
        ch->active = false;
    }
}

// Process audio for the current frame
void process_audio() {
    memset(tinybit_audio_buffer, 0, TB_AUDIO_FRAME_BUFFER_SIZE);

    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
        struct channel_state *channel = &channels[ch];

        if (!channel->active || !channel->current_note) continue;

        float phase = channel->phase;

        for (int i = 0; i < TB_AUDIO_FRAME_SAMPLES; i++) {
            // Check if we need to advance to next note
            if (channel->sample_processed >= channel->total_samples) {
                advance_to_next_note(channel);
                if (!channel->active || !channel->current_note) break;
            }

            int gain = GAIN;

            // Decay envelope
            if (channel->total_samples - channel->sample_processed < ENVELOPE_SAMPLES) {
                gain = (gain * (channel->total_samples - channel->sample_processed)) / ENVELOPE_SAMPLES;
            }
            // Attack envelope
            else if (channel->sample_processed < ENVELOPE_SAMPLES) {
                gain = (gain * channel->sample_processed) / ENVELOPE_SAMPLES;
            }

            // Update phase
            phase += channel->frequency / TB_AUDIO_SAMPLE_RATE;
            if (phase >= 1.0f) phase -= 1.0f;

            // Check if this is a rest note
            bool is_rest = (channel->current_note->note_name == NOTE_REST);

            if (!is_rest) {
                switch (channel->waveform) {
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
                    // No sound for rest
                    break;
                }
            }

            channel->sample_processed++;
        }
        channel->phase = phase;
    }
}

// Set the beats per minute for timing calculations
void set_bpm(int new_bpm) {
    if (new_bpm > 0) {
        bpm = new_bpm;
    }
}

// Load ABC notation into a specific channel
int audio_load_abc(int channel_num, const char *abc_string, WAVEFORM waveform, bool repeat) {
    if (channel_num < 0 || channel_num >= NUM_CHANNELS) return -1;
    if (!abc_string) return -1;

    struct channel_state *ch = &channels[channel_num];

    // Reset the sheet and note pool for this channel
    sheet_reset(&ch->sheet);

    // Parse the ABC notation
    if (abc_parse(&ch->sheet, abc_string) != 0) {
        return -1;
    }

    // Initialize playback state
    ch->current_note = sheet_first_note(&ch->sheet);
    ch->waveform = waveform;
    ch->repeat = repeat;
    ch->phase = 0.0f;
    ch->sample_processed = 0;

    if (ch->current_note) {
        ch->frequency = get_frequency_from_note(ch->current_note);
        ch->total_samples = duration_ms_to_samples(ch->current_note->duration_ms);
        ch->active = true;
    } else {
        ch->active = false;
    }

    return 0;
}

// Stop a channel
void audio_stop_channel(int channel_num) {
    if (channel_num < 0 || channel_num >= NUM_CHANNELS) return;
    channels[channel_num].active = false;
}

// Stop all channels
void audio_stop_all() {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        channels[i].active = false;
    }
}
