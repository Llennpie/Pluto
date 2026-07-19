#ifndef ASSETEXTRACT_SOUND_H
#define ASSETEXTRACT_SOUND_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <types.h>

#include "saturn/libs/dynamics.h"

typedef struct {
    uint32_t loc;
    const char* name;
} Sound_SampleAsset;

typedef struct {
    uint32_t* commands;
    size_t num_commands;
} Sound_Envelope;

typedef struct {
    const char* name;
    float tuning;
} Sound_Sample;

typedef struct {
    uint32_t release_rate;
    uint32_t envelope;
    Sound_Sample sound_lo, sound, sound_hi;
    uint8_t normal_range_hi, normal_range_lo, pan;
    bool is_perc;
} Sound_Instrument;

typedef struct {
    size_t index;
    const char* sample_bank;
    uint32_t date;

    Sound_Envelope* envelopes;
    size_t num_envelopes;

    Sound_Instrument* instruments;
    size_t num_instruments;

    Sound_Instrument* percussion;
    size_t num_percussion;

    int32_t* instrument_list;
    size_t num_instrument_list;

    bool is_shared;
} Sound_Bank;

typedef struct {
    Sound_Bank** banks;
    size_t num_banks;
} Sound_Sequence;

extern Sound_Sequence sound_sequences[];
extern Sound_Bank* sound_bank_list[];
extern int num_sound_sequences;
extern int num_sound_banks;

extern const u8 sound_player_sequence_data[];
extern int sound_player_sequence_data_size;

void sound_reassemble(
    set(Sound_SampleAsset) samples,
    const uint8_t* ctl_buf, size_t ctl_len,
    const uint8_t* tbl_buf, size_t tbl_len,
    uint8_t** out_ctl_buf, size_t* out_ctl_len,
    uint8_t** out_tbl_buf, size_t* out_tbl_len,
    uint8_t** out_seq_buf, size_t* out_seq_len,
    uint8_t** out_bnk_buf, size_t* out_bnk_len
);

#ifdef SOUND_DATA

#define Sequence Sound_Sequence
#define Bank static Sound_Bank

#define ARRAY(type, ...) (type[]){ __VA_ARGS__ }, sizeof((type[]){ __VA_ARGS__ }) / sizeof(type)

#define SEQUENCE(...) { .banks = ARRAY(Sound_Bank*, __VA_ARGS__) }
#define BANK(id, ...) { .index = id, __VA_ARGS__ }
#define SAMPLE_BANK(name) .sample_bank = name
#define DATE(y, m, d) .date = (((y) % 10000) * 10000 + ((m) % 100) * 100 + ((d) % 100))
#define SOUND(...) { __VA_ARGS__ }
#define ENVELOPES(...) .envelopes = ARRAY(Sound_Envelope, __VA_ARGS__)
#define ENVELOPE(...) { .commands = ARRAY(uint32_t, __VA_ARGS__) }
#define INSTRUMENTS(...) .instruments = ARRAY(Sound_Instrument, __VA_ARGS__)
#define INSTRUMENT_LIST(...) .instrument_list = ARRAY(int32_t, __VA_ARGS__)
#define PERCUSSION(...) .percussion = ARRAY(Sound_Instrument, __VA_ARGS__)
#define PERC_INSTRUMENT() { .is_perc = true }
#define NULL_INSTRUMENT -1

#define ENV_CUSTOM(a, b) ((((a) & 0xFFFF) << 16) | ((b) & 0xFFFF))
#define ENV_STOP() ENV_CUSTOM(0, 0)
#define ENV_HANG() ENV_CUSTOM((1 << 16) - 1, 0)
#define ENV_RESTART() ENV_CUSTOM((1 << 16) - 3, 0)
#define ENV_GOTO(pos) ENV_CUSTOM((1 << 16) - 2, pos)

#endif

#endif