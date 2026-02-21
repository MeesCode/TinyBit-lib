#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#define MY_ABC_MAX_CHORD_NOTES 3
#define MY_ABC_MAX_VOICES 3
#define MUSIC_MAX_NOTES 400
#define SFX_MAX_NOTES 10

#include "audio.h"
#include "tinybit.h"
#include "ABC-parser/abc_parser.h"
#include "memory.h"

#define M_PI 3.14159265358979323846
#define GAIN 4000
#define ENVELOPE_MS 10
#define ENVELOPE_SAMPLES ((TB_AUDIO_SAMPLE_RATE / 1000) * ENVELOPE_MS)

// Bandpass filter Q factor (higher = narrower bandwidth, more tonal)
#define NOISE_FILTER_Q 8.0f

int bpm = 150;
int channel = 0;
int volume = 10;

// Bandpass filter state for pitched noise
struct bandpass_state {
    float low;   // Low-pass output
    float band;  // Band-pass output
};

// Channel state for playback - each channel can have multiple voices from ABC
// Note storage is external (separate arrays for music vs SFX to save memory)
struct channel_state {
    struct sheet sheet;
    NotePool pools[MY_ABC_MAX_VOICES];

    // Per-voice playback state
    struct {
        struct note *current_note;
        uint32_t sample_processed;
        uint32_t total_samples;
        float phase[MY_ABC_MAX_CHORD_NOTES];  // Phase per chord note
        WAVEFORM waveform;
        bool active;
        struct bandpass_state bp[MY_ABC_MAX_CHORD_NOTES];  // Bandpass filter state per chord note
    } voices[MY_ABC_MAX_VOICES];

    bool repeat;
    bool channel_active;
};

static struct channel_state *channels;

// Map voice ID to waveform
static WAVEFORM voice_id_to_waveform(const char *voice_id) {
    if (!voice_id || voice_id[0] == '\0') return SINE;
    if (strcmp(voice_id, "SINE") == 0) return SINE;
    if (strcmp(voice_id, "SAW") == 0) return SAW;
    if (strcmp(voice_id, "SQUARE") == 0) return SQUARE;
    if (strcmp(voice_id, "NOISE") == 0) return NOISE;
    return SINE;  // Default
}

// Get frequency from note's chord array
static float get_frequency_from_chord(struct note *n, uint8_t chord_idx) {
    if (!n || chord_idx >= n->chord_size) return 0.0f;
    if (midi_is_rest(n->midi_note[chord_idx])) return 0.0f;
    return midi_to_frequency_x10(n->midi_note[chord_idx]) / 10.0f;
}

// Check if note is a rest
static bool is_rest_note(struct note *n) {
    if (!n || n->chord_size == 0) return true;
    return (n->chord_size == 1 && midi_is_rest(n->midi_note[0]));
}

// Calculate samples for a note based on its duration in ticks
static uint32_t duration_ticks_to_samples(uint8_t ticks, uint16_t bpm) {
    uint16_t duration_ms = ticks_to_ms(ticks, bpm);
    return (uint32_t)duration_ms * TB_AUDIO_SAMPLE_RATE / 1000;
}

// State-variable bandpass filter for pitched noise
// Returns the bandpass output centered on the given frequency
static float bandpass_filter(struct bandpass_state *state, float freq) {
    // Calculate filter coefficient from frequency
    float f = 2.0f * sinf((float)M_PI * freq / TB_AUDIO_SAMPLE_RATE);
    float q = 1.0f / NOISE_FILTER_Q;

    // State-variable filter update
    state->low += f * state->band;
    float high = (rand() / (float)RAND_MAX * 2.0f - 1.0f) - state->low - q * state->band;
    state->band += f * high;

    return state->band;
}

static void init_channel(struct channel_state *ch, struct note *storage[], uint16_t note_capacity) {
    for (int v = 0; v < MY_ABC_MAX_VOICES; v++) {
        note_pool_init(&ch->pools[v], storage[v], note_capacity, MY_ABC_MAX_CHORD_NOTES);
    }

    sheet_init(&ch->sheet, ch->pools, MY_ABC_MAX_VOICES);

    for (int v = 0; v < MY_ABC_MAX_VOICES; v++) {
        ch->voices[v].current_note = NULL;
        ch->voices[v].sample_processed = 0;
        ch->voices[v].total_samples = 0;
        ch->voices[v].waveform = SINE;
        ch->voices[v].active = false;
        for (int c = 0; c < MY_ABC_MAX_CHORD_NOTES; c++) {
            ch->voices[v].phase[c] = 0.0f;
            ch->voices[v].bp[c].low = 0.0f;
            ch->voices[v].bp[c].band = 0.0f;
        }
    }

    ch->repeat = true;
    ch->channel_active = false;
}

void tb_audio_init() {

    const size_t sfx_notes_size = sizeof(struct note) * MY_ABC_MAX_VOICES * SFX_MAX_NOTES;
    const size_t music_notes_size = sizeof(struct note) * MY_ABC_MAX_VOICES * MUSIC_MAX_NOTES;

    // Layout in audio_data: [sfx notes][music notes][channel states]
    struct note *sfx_base = (struct note *)tinybit_memory->audio_data;
    struct note *music_base = (struct note *)(tinybit_memory->audio_data + sfx_notes_size);
    channels = (struct channel_state *)(tinybit_memory->audio_data + sfx_notes_size + music_notes_size);

    struct note *sfx_ptrs[MY_ABC_MAX_VOICES] = {
        &sfx_base[0 * SFX_MAX_NOTES],
        &sfx_base[1 * SFX_MAX_NOTES],
        &sfx_base[2 * SFX_MAX_NOTES]
    };
    struct note *music_ptrs[MY_ABC_MAX_VOICES] = {
        &music_base[0 * MUSIC_MAX_NOTES],
        &music_base[1 * MUSIC_MAX_NOTES],
        &music_base[2 * MUSIC_MAX_NOTES]
    };

    init_channel(&channels[CHANNEL_MUSIC], music_ptrs, MUSIC_MAX_NOTES);
    init_channel(&channels[CHANNEL_SFX], sfx_ptrs, SFX_MAX_NOTES);
}

// Advance to the next note in a voice
static void advance_to_next_note(struct channel_state *ch, int voice_idx) {
    if (voice_idx >= MY_ABC_MAX_VOICES) return;

    NotePool *pool = &ch->pools[voice_idx];
    struct note *next = note_next(pool, ch->voices[voice_idx].current_note);

    // If no next note and repeat is enabled, loop back to start
    if (!next && ch->repeat) {
        next = pool_first_note(pool);
    } 

    ch->channel_active = next != NULL; // If we have a next note, channel is active; if not, it may become inactive

    if (next) {
        ch->voices[voice_idx].current_note = next;
        ch->voices[voice_idx].total_samples = duration_ticks_to_samples(next->duration, ch->sheet.tempo_bpm);
        ch->voices[voice_idx].sample_processed = 0;
        // Don't reset phase - preserve for smooth transitions
    } else {
        ch->voices[voice_idx].active = false;
    }
}

// Process audio for the current frame
void process_audio() {
    // printf("channels struct size %d\n", sizeof(channels[CHANNEL_MUSIC]) + sizeof(channels[CHANNEL_SFX]) + sizeof(struct note) * MY_ABC_MAX_VOICES * MUSIC_MAX_NOTES+sizeof(struct note) * MY_ABC_MAX_VOICES * SFX_MAX_NOTES);

    memset(tinybit_memory->audio_buffer, 0, TB_MEM_AUDIO_BUFFER_SIZE);

    for (int ch_idx = 0; ch_idx < NUM_CHANNELS; ch_idx++) {
        struct channel_state *channel = &channels[ch_idx];

        if (!channel->channel_active) continue;

        // Process each voice in this channel
        for (uint8_t v = 0; v < channel->sheet.voice_count && v < MY_ABC_MAX_VOICES; v++) {
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

                // apply simple attack/decay envelope for non-noise waveforms to reduce clicks
                if(channel->voices[v].waveform != NOISE){
                    // Decay envelope
                    if (channel->voices[v].total_samples - channel->voices[v].sample_processed < ENVELOPE_SAMPLES) {
                        gain = (gain * (channel->voices[v].total_samples - channel->voices[v].sample_processed)) / ENVELOPE_SAMPLES;
                    }
                    // Attack envelope
                    else if (channel->voices[v].sample_processed < ENVELOPE_SAMPLES) {
                        gain = (gain * channel->voices[v].sample_processed) / ENVELOPE_SAMPLES;
                    }
                }

                // Skip if rest note
                if (!is_rest_note(note)) {
                    float sample = 0.0f;

                    // Sum all chord notes
                    for (uint8_t c = 0; c < note->chord_size && c < MY_ABC_MAX_CHORD_NOTES; c++) {
                        float freq = get_frequency_from_chord(note, c);
                        if (freq > 0.0f) {

                            // Update phase for this chord note
                            channel->voices[v].phase[c] += freq / TB_AUDIO_SAMPLE_RATE;
                            if (channel->voices[v].phase[c] >= 1.0f) {
                                channel->voices[v].phase[c] -= 1.0f;
                            }

                            switch (channel->voices[v].waveform) {
                            case SQUARE:
                                // apply volume correction to match perceived loudness of sine wave
                                sample += (channel->voices[v].phase[c] < 0.5f ? -0.4f : 0.4f); 
                                break;
                            case SAW:
                                // apply volume correction to match perceived loudness of sine wave
                                sample += (channel->voices[v].phase[c] - 0.5f) * 0.4f; 
                                break;
                            case SINE:
                                sample += sinf(2.0f * (float)M_PI * channel->voices[v].phase[c]);
                                break;
                            case NOISE: {
                                sample += bandpass_filter(&channel->voices[v].bp[c], freq);
                                break;
                            }
                            case REST:
                            default:
                                sample += 0.0f;
                            }
                        }
                    }

                    // Normalize by chord size to prevent clipping
                    if (note->chord_size > 1) {
                        sample /= note->chord_size;
                    }

                    tinybit_memory->audio_buffer[i] += (int16_t)(sample * gain);
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

bool is_channel_active(int channel_num) {
    if (channel_num < 0 || channel_num >= NUM_CHANNELS) return false;
    return channels[channel_num].channel_active;
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
    for (int v = 0; v < MY_ABC_MAX_VOICES; v++) {
        ch->voices[v].current_note = NULL;
        ch->voices[v].sample_processed = 0;
        ch->voices[v].total_samples = 0;
        ch->voices[v].active = false;
        for (int c = 0; c < MY_ABC_MAX_CHORD_NOTES; c++) {
            ch->voices[v].phase[c] = 0.0f;
            ch->voices[v].bp[c].low = 0.0f;
            ch->voices[v].bp[c].band = 0.0f;
        }
    }

    // Parse the ABC notation
    if (abc_parse(&ch->sheet, abc_string) != 0) {
        ch->channel_active = false;
        return -1;
    }

    ch->repeat = repeat;

    // Initialize each parsed voice
    for (uint8_t v = 0; v < ch->sheet.voice_count && v < MY_ABC_MAX_VOICES; v++) {
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
            ch->voices[v].total_samples = duration_ticks_to_samples(
                ch->voices[v].current_note->duration, ch->sheet.tempo_bpm);
        }
    }

    // Channel is active if at least one voice is active
    ch->channel_active = false;
    for (int v = 0; v < MY_ABC_MAX_VOICES; v++) {
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
    for (int v = 0; v < MY_ABC_MAX_VOICES; v++) {
        channels[channel_num].voices[v].active = false;
    }
}

// Stop all channels
void audio_stop_all() {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        audio_stop_channel(i);
    }
}
