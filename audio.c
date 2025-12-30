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
#define VOICES_PER_CHANNEL ABC_MAX_VOICES

int bpm = 150;
int channel = 0;
int volume = 10;

// Channel state for playback - each channel can have multiple voices from ABC
struct channel_state {
    struct sheet sheet;
    NotePool pools[VOICES_PER_CHANNEL];  // Note pools for voices in this channel

    // Per-voice playback state
    struct {
        struct note *current_note;
        uint32_t sample_processed;
        uint32_t total_samples;
        float phase[ABC_MAX_CHORD_NOTES];  // Phase per chord note
        WAVEFORM waveform;
        bool active;
    } voices[VOICES_PER_CHANNEL];

    bool repeat;
    bool channel_active;
};

static struct channel_state channels[NUM_CHANNELS];

// Map voice ID to waveform
static WAVEFORM voice_id_to_waveform(const char *voice_id) {
    if (!voice_id || voice_id[0] == '\0') return SQUARE;
    if (strcmp(voice_id, "SINE") == 0) return SINE;
    if (strcmp(voice_id, "SAW") == 0) return SAW;
    if (strcmp(voice_id, "SQUARE") == 0) return SQUARE;
    if (strcmp(voice_id, "NOISE") == 0) return NOISE;
    return SINE;  // Default
}

// Get frequency from note's chord array
static float get_frequency_from_chord(struct note *n, uint8_t chord_idx) {
    if (!n || chord_idx >= n->chord_size) return 0.0f;
    if (n->note_name[chord_idx] == NOTE_REST) return 0.0f;
    return n->frequency_x10[chord_idx] / 10.0f;
}

// Check if note is a rest
static bool is_rest_note(struct note *n) {
    if (!n || n->chord_size == 0) return true;
    return (n->chord_size == 1 && n->note_name[0] == NOTE_REST);
}

// Calculate samples for a note based on its duration in ms
static uint32_t duration_ms_to_samples(uint16_t duration_ms) {
    return (uint32_t)duration_ms * TB_AUDIO_SAMPLE_RATE / 1000;
}

void tb_audio_init() {
    // Initialize all channels
    for (int i = 0; i < NUM_CHANNELS; i++) {
        // Initialize note pools for this channel
        for (int v = 0; v < VOICES_PER_CHANNEL; v++) {
            note_pool_init(&channels[i].pools[v]);
        }

        // Initialize sheet with pools
        sheet_init(&channels[i].sheet, channels[i].pools, VOICES_PER_CHANNEL);

        // Initialize voice states
        for (int v = 0; v < VOICES_PER_CHANNEL; v++) {
            channels[i].voices[v].current_note = NULL;
            channels[i].voices[v].sample_processed = 0;
            channels[i].voices[v].total_samples = 0;
            channels[i].voices[v].waveform = SINE;
            channels[i].voices[v].active = false;
            for (int c = 0; c < ABC_MAX_CHORD_NOTES; c++) {
                channels[i].voices[v].phase[c] = 0.0f;
            }
        }

        channels[i].repeat = true;
        channels[i].channel_active = false;
    }

    // Example ABC notation with multiple voices
    const char *abc_music =
        "X:1\n"
        "T:Super Mario Theme\n"
        "M:4/4\n"
        "L:1/8\n"
        "Q:1/4=105\n"
        "K:G\n"
        "V:SINE\n"
        "[e/2c/2][ce][ec][c/2A/2][ce] g/2z3z/2|c/2zG/2 zE/2zAB^A/2=A| (3Geg a=f/2gec/2 d/2B/2z|c/2zG/2 zE/2zAB^A/2=A|\n"
        "V:2\n"
        "E4 G4 | C4 z4 | G4 D4 | C4 z4 | \n"
        "V:SINE\n"
        "(3Geg a=f/2gec/2 d/2B/2z|zg/2^f/2 =f/2^de^G/2A/2cA/2c/2=d/2|zg/2^f/2 =f/2^dec'c'/2 c'/2z3/2|zg/2^f/2 =f/2^de^G/2A/2cA/2c/2=d/2|\n"
        "V:2\n"
        "G4 D4 | z G3 E4 | z G3 c4 | z G3 E4 | \n"
        "V:SINE\n"
        "z^d/2z=d/2z c/2z3z/2|]\n"
        "V:2\n"
        "z ^D3 C/z3z/2|]\n";

    // Parse ABC for channel 0
    if (abc_parse(&channels[0].sheet, abc_music) == 0) {
        channels[0].repeat = true;
        channels[0].channel_active = true;

        // Initialize each voice from the parsed sheet
        for (uint8_t v = 0; v < channels[0].sheet.voice_count && v < VOICES_PER_CHANNEL; v++) {
            NotePool *pool = &channels[0].pools[v];
            channels[0].voices[v].current_note = pool_first_note(pool);
            channels[0].voices[v].waveform = voice_id_to_waveform(pool->voice_id);
            channels[0].voices[v].active = (channels[0].voices[v].current_note != NULL);

            if (channels[0].voices[v].current_note) {
                channels[0].voices[v].total_samples = duration_ms_to_samples(
                    channels[0].voices[v].current_note->duration_ms);
                channels[0].voices[v].sample_processed = 0;

                // Reset phases for all chord notes
                for (int c = 0; c < ABC_MAX_CHORD_NOTES; c++) {
                    channels[0].voices[v].phase[c] = 0.0f;
                }
            }
        }
    }
}

// Advance to the next note in a voice
static void advance_to_next_note(struct channel_state *ch, int voice_idx) {
    if (voice_idx >= VOICES_PER_CHANNEL) return;

    NotePool *pool = &ch->pools[voice_idx];
    struct note *next = note_next(pool, ch->voices[voice_idx].current_note);

    // If no next note and repeat is enabled, loop back to start
    if (!next && ch->repeat) {
        next = pool_first_note(pool);
    }

    if (next) {
        ch->voices[voice_idx].current_note = next;
        ch->voices[voice_idx].total_samples = duration_ms_to_samples(next->duration_ms);
        ch->voices[voice_idx].sample_processed = 0;
        // Don't reset phase - preserve for smooth transitions
    } else {
        ch->voices[voice_idx].active = false;
    }
}

// Generate sample for a single waveform
static float generate_waveform(WAVEFORM waveform, float phase) {
    switch (waveform) {
    case SQUARE:
        return (phase < 0.5f ? -1.0f : 1.0f);
    case SAW:
        return (phase * 2.0f - 1.0f);
    case SINE:
        return sinf(2.0f * (float)M_PI * phase);
    case NOISE:
        return (rand() / (float)RAND_MAX * 2.0f - 1.0f);
    case REST:
    default:
        return 0.0f;
    }
}

// Process audio for the current frame
void process_audio() {
    memset(tinybit_audio_buffer, 0, TB_AUDIO_FRAME_BUFFER_SIZE);

    for (int ch_idx = 0; ch_idx < NUM_CHANNELS; ch_idx++) {
        struct channel_state *channel = &channels[ch_idx];

        if (!channel->channel_active) continue;

        // Process each voice in this channel
        for (uint8_t v = 0; v < channel->sheet.voice_count && v < VOICES_PER_CHANNEL; v++) {
            if (!channel->voices[v].active || !channel->voices[v].current_note) continue;

            for (int i = 0; i < TB_AUDIO_FRAME_SAMPLES; i++) {
                // Check if we need to advance to next note
                if (channel->voices[v].sample_processed >= channel->voices[v].total_samples) {
                    advance_to_next_note(channel, v);
                    if (!channel->voices[v].active || !channel->voices[v].current_note) break;
                }

                struct note *note = channel->voices[v].current_note;

                // Calculate envelope gain
                int gain = GAIN / (channel->sheet.voice_count > 0 ? channel->sheet.voice_count : 1);

                // Decay envelope
                if (channel->voices[v].total_samples - channel->voices[v].sample_processed < ENVELOPE_SAMPLES) {
                    gain = (gain * (channel->voices[v].total_samples - channel->voices[v].sample_processed)) / ENVELOPE_SAMPLES;
                }
                // Attack envelope
                else if (channel->voices[v].sample_processed < ENVELOPE_SAMPLES) {
                    gain = (gain * channel->voices[v].sample_processed) / ENVELOPE_SAMPLES;
                }

                // Skip if rest note
                if (!is_rest_note(note)) {
                    float sample = 0.0f;

                    // Sum all chord notes
                    for (uint8_t c = 0; c < note->chord_size && c < ABC_MAX_CHORD_NOTES; c++) {
                        float freq = get_frequency_from_chord(note, c);
                        if (freq > 0.0f) {
                            // Update phase for this chord note
                            channel->voices[v].phase[c] += freq / TB_AUDIO_SAMPLE_RATE;
                            if (channel->voices[v].phase[c] >= 1.0f) {
                                channel->voices[v].phase[c] -= 1.0f;
                            }

                            // Generate and accumulate waveform
                            sample += generate_waveform(channel->voices[v].waveform,
                                                        channel->voices[v].phase[c]);
                        }
                    }

                    // Normalize by chord size to prevent clipping
                    if (note->chord_size > 1) {
                        sample /= note->chord_size;
                    }

                    tinybit_audio_buffer[i] += (int16_t)(sample * gain);
                }

                channel->voices[v].sample_processed++;
            }
        }
    }
}

// Set the beats per minute for timing calculations
void set_bpm(int new_bpm) {
    if (new_bpm > 0) {
        bpm = new_bpm;
    }
}

// Load ABC notation into a specific channel
// The ABC can contain multiple voices which will be parsed into separate voice states
int audio_load_abc(int channel_num, const char *abc_string, WAVEFORM waveform, bool repeat) {
    if (channel_num < 0 || channel_num >= NUM_CHANNELS) return -1;
    if (!abc_string) return -1;

    struct channel_state *ch = &channels[channel_num];

    // Reset the sheet (also resets all pools)
    sheet_reset(&ch->sheet);

    // Reset voice states
    for (int v = 0; v < VOICES_PER_CHANNEL; v++) {
        ch->voices[v].current_note = NULL;
        ch->voices[v].sample_processed = 0;
        ch->voices[v].total_samples = 0;
        ch->voices[v].active = false;
        for (int c = 0; c < ABC_MAX_CHORD_NOTES; c++) {
            ch->voices[v].phase[c] = 0.0f;
        }
    }

    // Parse the ABC notation
    if (abc_parse(&ch->sheet, abc_string) != 0) {
        ch->channel_active = false;
        return -1;
    }

    ch->repeat = repeat;

    // Initialize each parsed voice
    for (uint8_t v = 0; v < ch->sheet.voice_count && v < VOICES_PER_CHANNEL; v++) {
        NotePool *pool = &ch->pools[v];
        ch->voices[v].current_note = pool_first_note(pool);

        // Use voice ID to determine waveform, fallback to parameter
        if (pool->voice_id[0] != '\0') {
            ch->voices[v].waveform = voice_id_to_waveform(pool->voice_id);
        } else {
            ch->voices[v].waveform = waveform;
        }

        ch->voices[v].active = (ch->voices[v].current_note != NULL);

        if (ch->voices[v].current_note) {
            ch->voices[v].total_samples = duration_ms_to_samples(
                ch->voices[v].current_note->duration_ms);
        }
    }

    // Channel is active if at least one voice is active
    ch->channel_active = false;
    for (int v = 0; v < VOICES_PER_CHANNEL; v++) {
        if (ch->voices[v].active) {
            ch->channel_active = true;
            break;
        }
    }

    return 0;
}

// Stop a channel
void audio_stop_channel(int channel_num) {
    if (channel_num < 0 || channel_num >= NUM_CHANNELS) return;
    channels[channel_num].channel_active = false;
    for (int v = 0; v < VOICES_PER_CHANNEL; v++) {
        channels[channel_num].voices[v].active = false;
    }
}

// Stop all channels
void audio_stop_all() {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        audio_stop_channel(i);
    }
}
