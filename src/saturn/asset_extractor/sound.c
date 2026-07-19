// these are the tools/assemble_sound.py and tools/disassemble_sound.py
// scripts rewritten in C

#include "sound.h"
#include "asset_extractor.h"

#include "sound/custom_sounds.h"

#include <platform_info.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define countof(...) (sizeof(__VA_ARGS__) / sizeof(*(__VA_ARGS__)))

#if IS_64_BIT
#define WORD_BYTES 8
#else
#define WORD_BYTES 4
#endif

#define align(val, al) (((val) + ((al) - 1)) & -(al))

typedef enum {
    TYPE_CTL = 1,
    TYPE_TBL = 2,
    TYPE_SEQ = 3,
} FileType;

typedef struct {
    uint8_t* buf;
    size_t size;
} Serializer;

typedef struct {
    uint32_t off, len;
} SeqEntry;

static void* memcpy_rev(void* out, const void* in, size_t size) {
    for (size_t i = 0; i < size; i++)
        ((uint8_t*)out)[size - i - 1] = ((uint8_t*)in)[i];
    return out;
}

static void*(*get_copy_func(const char** fmt))(void*, const void*, size_t) {
    if (**fmt == '=') {
        (*fmt)++;
        return memcpy;
    }
    else if (**fmt == '<' && ((*fmt)++ || 1)) {
#if IS_BIG_ENDIAN
        return memcpy_rev;
#else
        return memcpy;
#endif
    }
    else if (**fmt == '>' && ((*fmt)++ || 1)) {
#if IS_BIG_ENDIAN
        return memcpy;
#else
        return memcpy_rev;
#endif
    }
    return memcpy;
}

static uint8_t* vpack(const char* fmt, uint8_t* buf, va_list args) {
    void*(*copy)(void*, const void*, size_t) = get_copy_func(&fmt);

    char c;
    while ((c = *fmt++)) switch (c) {
        case 'x': buf++; break;
#if IS_64_BIT
        case 'X': buf += 4; break;
#endif
        case 'b': case 'B': case '?': case 'c':
            copy(buf, &(char){va_arg(args, int)}, 1), buf += 1; break;
        case 'h': case 'H':
            copy(buf, &(short){va_arg(args, int)}, 2), buf += 2; break;
        case 'i': case 'I': case 'l': case 'L':
            copy(buf, &(int){va_arg(args, int)}, 4), buf += 4; break;
        case 'q': case 'Q':
            copy(buf, &(long long){va_arg(args, long long)}, 8), buf += 8; break;
        case 'f':
            copy(buf, &(float){va_arg(args, double)}, 4), buf += 4; break;
        case 'd':
            copy(buf, &(double){va_arg(args, double)}, 4), buf += 4; break;
        case 'F':
            copy(buf, &(_Complex float){va_arg(args, _Complex float)}, 8), buf += 8; break;
        case 'D':
            copy(buf, &(_Complex double){va_arg(args, _Complex double)}, 16), buf += 16; break;
        case 'P':
            copy(buf, &(void*){va_arg(args, void*)}, sizeof(void*)), buf += sizeof(void*); break;
    }

    return buf;
}

static const uint8_t* vunpack(const char* fmt, const uint8_t* buf, va_list args) {
    void*(*copy)(void*, const void*, size_t) = get_copy_func(&fmt);

    char c;
    while ((c = *fmt++)) switch (c) {
        case 'x': buf++; break;
#if IS_64_BIT
        case 'X': buf += 4; break;
#endif
        default: {
            uint8_t size = (uint8_t[]){
                ['c'] = 1, ['b'] = 1, ['B'] = 1, ['?'] = 1,
                ['h'] = 2, ['H'] = 2,
                ['i'] = 4, ['I'] = 4, ['l'] = 4, ['L'] = 4, ['f'] = 4,
                ['q'] = 8, ['Q'] = 8, ['d'] = 8, ['F'] = 8,
                ['D'] = 16,

                ['P'] = sizeof(void*),
            }[c];
            if (size == 0) break;
            void* ptr = va_arg(args, void*);
            if (ptr)
                copy(ptr, buf, size);
            buf += size;
        } break;
    }

    return buf;
}

static size_t calcsize(const char* fmt) {
    if (*fmt == '=' || *fmt == '<' || *fmt == '>') fmt++;

    char c;
    size_t size = 0;
    while ((c = *fmt++)) switch (c) {
#if IS_64_BIT
        case 'X': size += 4; break;
#endif
        default: size += (uint8_t[]){
            ['c'] = 1, ['b'] = 1, ['B'] = 1, ['?'] = 1, ['x'] = 1,
            ['h'] = 2, ['H'] = 2,
            ['i'] = 4, ['I'] = 4, ['l'] = 4, ['L'] = 4, ['f'] = 4,
            ['q'] = 8, ['Q'] = 8, ['d'] = 8, ['F'] = 8,
            ['D'] = 16,

            ['P'] = sizeof(void*),
        }[c]; break;
    }

    return size;
}

static uint8_t* pack(const char* fmt, uint8_t* buf, ...) {
    va_list args;
    va_start(args, buf);
    buf = vpack(fmt, buf, args);
    va_end(args);
    return buf;
}

static const uint8_t* unpack(const char* fmt, const uint8_t* buf, ...) {
    va_list args;
    va_start(args, buf);
    buf = vunpack(fmt, buf, args);
    va_end(args);
    return buf;
}

static size_t ser_reserve(Serializer* ser, size_t size) {
    size_t old = ser->size;
    ser->size += size;
    ser->buf = realloc(ser->buf, ser->size);
    memset(ser->buf + old, 0, size);
    return old;
}

static void ser_add(Serializer* ser, const uint8_t* buf, size_t size) {
    size_t old = ser->size;
    ser->size += size;
    ser->buf = realloc(ser->buf, ser->size);
    memcpy(ser->buf + old, buf, size);
}

static void ser_align(Serializer* ser, size_t alig) {
    size_t alignment = align(ser->size, alig) - ser->size;
    ser_reserve(ser, alignment);
}

static void ser_pack(Serializer* ser, const char* fmt, ...) {
    size_t pos = ser_reserve(ser, calcsize(fmt));

    va_list args;
    va_start(args, fmt);
    vpack(fmt, ser->buf + pos, args);
    va_end(args);
}

static size_t ser_pack_reserved(Serializer* ser, size_t pos, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    uint8_t* buf = vpack(fmt, ser->buf + pos, args);
    va_end(args);
    return buf - ser->buf;
}

typedef struct {
    int16_t order, npredictors;
    int16_t* table;
} Book;

typedef struct {
    uint32_t start, end;
    int32_t count;
    int16_t state[16];
} Loop;

typedef struct {
    const uint8_t* data;
    size_t data_size;
    size_t tbl_offset, ctl_offset;
    Book book;
    Loop loop;
    float tuning;
} Aifc;

map(uint32_t, Aifc) AifcEntry;

typedef struct {
    const uint8_t* data;
    size_t size, offset;
    set(AifcEntry) samples;
} SampleBank;

typedef struct {
    uint32_t addr;
    float tuning;
} SampleAddr;

map(size_t, size_t) IndexMap;
map(const char*, Aifc) NameToSampleMap;
map(const char*, Aifc*) NameToSamplePtrMap;
map(const char*, int) NameToIndexMap;

static list(SeqEntry) parse_seqfile(const uint8_t* data, size_t size) {
    uint16_t num_entries;
    data = unpack(">HH", data, NULL, &num_entries);

    list(SeqEntry) entries = list_init(SeqEntry);
    for (size_t i = 0; i < num_entries; i++) {
        SeqEntry* entry = &push(entries);
        data = unpack(">II", data, &entry->off, &entry->len);
    }
    return entries;
}

static void parse_tbl(
    const uint8_t* data, size_t data_size, list(SeqEntry) entries,
    list(size_t)* tbls, list(SampleBank)* sample_banks, set(IndexMap)* sample_bank_map
) {
    smart set(size_t) seen = set_init(size_t, compare_u64);
    *tbls = list_init(size_t);
    *sample_banks = list_init(SampleBank);
    *sample_bank_map = set_init(IndexMap, compare_u64);

    foreach (*entry, entries) {
        if (!find(seen, &entry->off)) {
            SampleBank sample_bank = { data + entry->off, entry->len, entry->off, set_init(AifcEntry, compare_u32) };
            push(seen) = entry->off;
            push(*sample_banks) = sample_bank;
            push(*sample_bank_map) = (IndexMap){ .key = entry->off, .value = size(*sample_banks) - 1 };
        }
        push(*tbls) = entry->off;
    }
}

static void parse_ctl(const uint8_t* data, size_t size, SampleBank* sample_bank, size_t index) {
    uint32_t num_instruments, num_drums, drum_base_addr;
    data = unpack(">IIII", data, &num_instruments, &num_drums, NULL, NULL);

    const uint8_t* base = data;

    smart set(SampleAddr) sample_addrs = set_init(SampleAddr, compare_u32);

    data = unpack(">I", data, &drum_base_addr);
    if (num_drums != 0) {
        const uint8_t* drum_data = base + drum_base_addr;
        for (size_t i = 0; i < num_drums; i++) {
            uint32_t drum_addr;
            SampleAddr sample_addr;
            drum_data = unpack(">I", drum_data, &drum_addr);
            unpack(">BBBBIfI", base + drum_addr, NULL, NULL, NULL, NULL, &sample_addr.addr, &sample_addr.tuning, NULL);
            if (!find(sample_addrs, &sample_addr.addr)) push(sample_addrs) = sample_addr;
        }
    }

    for (size_t i = 0; i < num_instruments; i++) {
        uint32_t inst_addr;
        SampleAddr sample_addr, sample_lo_addr, sample_hi_addr;
        data = unpack(">I", data, &inst_addr);
        if (inst_addr == 0) continue;

        unpack(">BBBBIIfIfIf", base + inst_addr, NULL, NULL, NULL, NULL, NULL,
            &sample_lo_addr.addr, &sample_lo_addr.tuning,
            &sample_addr.addr,    &sample_addr.tuning,
            &sample_hi_addr.addr, &sample_hi_addr.tuning
        );
        if (!find(sample_addrs, &sample_addr.addr)) push(sample_addrs) = sample_addr;
        if (sample_lo_addr.addr != 0 && !find(sample_addrs, &sample_lo_addr.addr)) push(sample_addrs) = sample_lo_addr;
        if (sample_hi_addr.addr != 0 && !find(sample_addrs, &sample_hi_addr.addr)) push(sample_addrs) = sample_hi_addr;
    }

    foreach (*sample_addr, sample_addrs) {
        const uint8_t* sample_data = base + sample_addr->addr;
        uint32_t offset, loop, book, sample_size;
        unpack(">IIIII", sample_data, NULL, &offset, &loop, &book, &sample_size);
        if (find(sample_bank->samples, &offset)) continue;

        Aifc aifc = { .data = sample_bank->data + offset, .data_size = sample_size, .tbl_offset = offset, .tuning = sample_addr->tuning };

        const uint8_t* loop_data = base + loop;
        loop_data = unpack(">IIiI", loop_data, &aifc.loop.start, &aifc.loop.end, &aifc.loop.count, NULL);
        if (aifc.loop.count != 0) for (int i = 0; i < 16; i++)
            loop_data = unpack(">h", loop_data, &aifc.loop.state[i]);

        const uint8_t* book_data = base + book;
        book_data = unpack(">ii", book_data, &aifc.book.order, &aifc.book.npredictors);
        aifc.book.table = malloc(16 * aifc.book.order * aifc.book.npredictors);
        for (int i = 0; i < 8 * aifc.book.order * aifc.book.npredictors; i++)
            book_data = unpack(">h", book_data, &aifc.book.table[i]);

        push(sample_bank->samples) = (AifcEntry){ .key = offset, .value = aifc };
    }
}

static set(NameToSampleMap) sound_disassemble(
    set(Sound_SampleAsset) samples,
    const uint8_t* ctl_buf, size_t ctl_size,
    const uint8_t* tbl_buf, size_t tbl_size
) {
    smart list(SeqEntry) ctl_entries = parse_seqfile(ctl_buf, ctl_size);
    smart list(SeqEntry) tbl_entries = parse_seqfile(tbl_buf, tbl_size);

    smart list(size_t) tbls;
    smart list(SampleBank) sample_banks;
    smart set(IndexMap) sample_bank_map;
    parse_tbl(tbl_buf, tbl_size, tbl_entries, &tbls, &sample_banks, &sample_bank_map);

    for (size_t i = 0; i < size(ctl_entries); i++) {
        SampleBank* bank = &sample_banks[find(sample_bank_map, &tbls[i])->value];
        const uint8_t* entry = ctl_buf + ctl_entries[i].off;
        parse_ctl(entry, ctl_entries[i].len, bank, i);
    }

    size_t sample_index = 0;
    set(NameToSampleMap) name_to_sample = set_init(NameToSampleMap, compare_str);
    foreach (*sample_bank, sample_banks) {
        foreach (*entry, sample_bank->samples) {
            Sound_SampleAsset* asset = find(samples, &sample_index);
            if (asset)
                push(name_to_sample) = (NameToSampleMap){ .key = asset->name, .value = entry->value };
            sample_index++;
        }
        destroy(sample_bank->samples);
    }

    return name_to_sample;
}

static uint32_t to_bcd(uint32_t in) {
    uint32_t shift = 0, out = 0;
    while (in) {
        out |= (in % 10) << shift;
        shift += 4;
        in /= 10;
    }
    return out;
}

static void serialize_sound(Serializer* ser, Sound_Sample* sound, set(NameToSamplePtrMap) samples) {
    if (sound->name == NULL) {
        ser_pack(ser, "PfX", 0, 0.0f);
        return;
    }
    Aifc* sample = sound->name ? find(samples, &sound->name)->value : 0;
    ser_pack(ser, "PfX", sample->ctl_offset, sound->tuning == 0 ? sample->tuning : sound->tuning);
}

static void serialize_ctl(Serializer* ser, Sound_Bank* bank, set(NameToSampleMap) samples) {
    ser_pack(ser, "IIII", bank->num_instrument_list, bank->num_percussion, bank->is_shared ? 1 : 0, to_bcd(bank->date));
    size_t base = ser->size;
    size_t perc_pos_buf = ser_reserve(ser, WORD_BYTES);
    size_t inst_pos_buf = ser_reserve(ser, WORD_BYTES * bank->num_instrument_list);
    ser_align(ser, 16);

    smart list(const char*) used_samples = list_init(const char*);
    for (size_t i = 0; i < bank->num_instruments; i++) {
        Sound_Instrument* inst = &bank->instruments[i];
        if (inst->is_perc) for (size_t p = 0; p < bank->num_percussion; p++) {
            Sound_Instrument* perc = &bank->percussion[p];
            if (perc->sound.name) push(used_samples) = perc->sound.name;
        }
        else {
            if (inst->sound_lo.name) push(used_samples) = inst->sound_lo.name;
            if (inst->sound.name) push(used_samples) = inst->sound.name;
            if (inst->sound_hi.name) push(used_samples) = inst->sound_hi.name;
        }
    }

    smart set(NameToSamplePtrMap) sample_name_to_addr = set_init(NameToSamplePtrMap, compare_str);
    foreach (name, used_samples) {
        if (find(sample_name_to_addr, &name)) continue;

        char sample_name[256], *sample_name_ptr = sample_name;
        snprintf(sample_name, 255, "%s/%s", bank->sample_bank, name);
        Aifc* aifc = &find(samples, &sample_name_ptr)->value;
        aifc->ctl_offset = ser->size - base;

        push(sample_name_to_addr) = (NameToSamplePtrMap){
            .key = name,
            .value = aifc,
        };

        // sample
        ser_pack(ser, "IXP", 0, aifc->tbl_offset);
        size_t loop_addr_buf = ser_reserve(ser, WORD_BYTES);
        size_t book_addr_buf = ser_reserve(ser, WORD_BYTES);
        ser_pack(ser, "I", align(aifc->data_size, 2));
        ser_align(ser, 16);

        // book
        ser_pack_reserved(ser, book_addr_buf, "P", ser->size - base);
        ser_pack(ser, "ii", aifc->book.order, aifc->book.npredictors);
        for (size_t i = 0; i < 8 * aifc->book.order * aifc->book.npredictors; i++)
            ser_pack(ser, "h", aifc->book.table[i]);
        ser_align(ser, 16);

        // loop
        ser_pack_reserved(ser, loop_addr_buf, "P", ser->size - base);
        if (aifc->loop.count == 0)
            ser_pack(ser, "IIiI", 0, aifc->data_size / 9 * 16 + (aifc->data_size % 2) + (aifc->data_size % 9), 0, 0);
        else {
            ser_pack(ser, "IIiI", aifc->loop.start, aifc->loop.end, aifc->loop.count, 0);
            for (size_t i = 0; i < 16; i++)
                ser_pack(ser, "h", aifc->loop.state[i]);
        }
        ser_align(ser, 16);
    }

    size_t envelopes[bank->num_envelopes];
    for (size_t i = 0; i < bank->num_envelopes; i++) {
        Sound_Envelope* env = &bank->envelopes[i];
        envelopes[i] = ser->size - base;
        for (size_t i = 0; i < env->num_commands; i++)
            ser_pack(ser, ">I", env->commands[i]);
        ser_align(ser, 16);
    }

    for (size_t i = 0; i < bank->num_instruments; i++) {
        Sound_Instrument* inst = &bank->instruments[i];
        if (inst->is_perc) continue;
        inst->ctl_offset = ser->size - base;

        if (inst->normal_range_hi == 0)
            inst->normal_range_hi = 127;

        ser_pack(ser, "BBBBXP", 0, inst->normal_range_lo, inst->normal_range_hi, inst->release_rate, envelopes[inst->envelope]);
        serialize_sound(ser, &inst->sound_lo, sample_name_to_addr);
        serialize_sound(ser, &inst->sound,    sample_name_to_addr);
        serialize_sound(ser, &inst->sound_hi, sample_name_to_addr);
    }

    for (size_t i = 0; i < bank->num_instrument_list; i++) {
        if (bank->instrument_list[i] == -1) {
            inst_pos_buf = ser_pack_reserved(ser, inst_pos_buf, "P", 0);
            continue;
        }
        inst_pos_buf = ser_pack_reserved(ser, inst_pos_buf, "P", bank->instruments[bank->instrument_list[i]].ctl_offset);
    }
    if (bank->percussion) {
        size_t percussion[bank->num_percussion];
        for (size_t i = 0; i < bank->num_percussion; i++) {
            Sound_Instrument* inst = &bank->percussion[i];
            percussion[i] = ser->size - base;
            ser_pack(ser, "BBBBX", inst->release_rate, inst->pan, 0, 0);
            serialize_sound(ser, &inst->sound, sample_name_to_addr);
            ser_pack(ser, "P", envelopes[inst->envelope]);
        }
        ser_align(ser, 16);

        perc_pos_buf = ser_pack_reserved(ser, perc_pos_buf, "P", ser->size - base);
        for (size_t i = 0; i < bank->num_percussion; i++)
            ser_pack(ser, "P", percussion[i]);
    }
    ser_align(ser, 16);
}

static void serialize_tbl(Serializer* ser, Sound_Bank* bank, set(NameToSampleMap) extracted_samples) {
    size_t base = ser->size;
    foreach (*sample, extracted_samples) {
        int len = strlen(bank->sample_bank);
        if (strncmp(bank->sample_bank, sample->key, len) != 0) continue;
        if (sample->key[len] != '/') continue;

        ser_align(ser, 16);
        sample->value.tbl_offset = ser->size - base;
        ser_add(ser, sample->value.data, sample->value.data_size);
    }
    ser_align(ser, 16);
}

static void serialize_seqfile(
    Serializer* ser, FileType type, set(NameToSampleMap) extracted_samples,
    void(*serialize)(Serializer*, Sound_Bank*, set(NameToSampleMap)), list(int) indexes
) {
    size_t offsets[size(indexes)], sizes[size(indexes)];
    memset(offsets, 0, size(indexes) * sizeof(size_t));
    memset(sizes, 0, size(indexes) * sizeof(size_t));

    ser_pack(ser, "HHX", type, size(indexes));
    size_t table = ser_reserve(ser, calcsize("PIX") * size(indexes));
    ser_align(ser, 16);
    foreach (index, indexes) {
        size_t offset, size;
        if (offsets[index] == 0) {
            offset = offsets[index] = ser->size;
            serialize(ser, sound_bank_list[index], extracted_samples);
            size = sizes[index] = ser->size - offsets[index];
        }
        else {
            offset = offsets[index];
            size = sizes[index];
        }
        table = ser_pack_reserved(ser, table, "PIX", offset, size);
    }
    ser_align(ser, 64);
}

static void write_sequences(
    uint8_t** out_seq_buf, size_t* out_seq_len,
    uint8_t** out_bnk_buf, size_t* out_bnk_len
) {
    static struct {
        size_t id;
        const char* name;
    } sequence_table[] = {
        { 0x00, "sound/sequences/00_sound_player.m64" },
        { 0x01, "sound/sequences/us/01_cutscene_collect_star.m64" },
        { 0x02, "sound/sequences/us/02_menu_title_screen.m64" },
        { 0x03, "sound/sequences/us/03_level_grass.m64" },
        { 0x04, "sound/sequences/us/04_level_inside_castle.m64" },
        { 0x05, "sound/sequences/us/05_level_water.m64" },
        { 0x06, "sound/sequences/us/06_level_hot.m64" },
        { 0x07, "sound/sequences/us/07_level_boss_koopa.m64" },
        { 0x08, "sound/sequences/us/08_level_snow.m64" },
        { 0x09, "sound/sequences/us/09_level_slide.m64" },
        { 0x0A, "sound/sequences/us/0A_level_spooky.m64" },
        { 0x0B, "sound/sequences/us/0B_event_piranha_plant.m64" },
        { 0x0C, "sound/sequences/us/0C_level_underground.m64" },
        { 0x0D, "sound/sequences/us/0D_menu_star_select.m64" },
        { 0x0E, "sound/sequences/us/0E_event_powerup.m64" },
        { 0x0F, "sound/sequences/us/0F_event_metal_cap.m64" },
        { 0x10, "sound/sequences/us/10_event_koopa_message.m64" },
        { 0x11, "sound/sequences/us/11_level_koopa_road.m64" },
        { 0x12, "sound/sequences/us/12_event_high_score.m64" },
        { 0x13, "sound/sequences/us/13_event_merry_go_round.m64" },
        { 0x14, "sound/sequences/us/14_event_race.m64" },
        { 0x15, "sound/sequences/us/15_cutscene_star_spawn.m64" },
        { 0x16, "sound/sequences/us/16_event_boss.m64" },
        { 0x17, "sound/sequences/us/17_cutscene_collect_key.m64" },
        { 0x18, "sound/sequences/us/18_event_endless_stairs.m64" },
        { 0x19, "sound/sequences/us/19_level_boss_koopa_final.m64" },
        { 0x1A, "sound/sequences/us/1A_cutscene_credits.m64" },
        { 0x1B, "sound/sequences/us/1B_event_solve_puzzle.m64" },
        { 0x1C, "sound/sequences/us/1C_event_toad_message.m64" },
        { 0x1D, "sound/sequences/us/1D_event_peach_message.m64" },
        { 0x1E, "sound/sequences/us/1E_cutscene_intro.m64" },
        { 0x1F, "sound/sequences/us/1F_cutscene_victory.m64" },
        { 0x20, "sound/sequences/us/20_cutscene_ending.m64" },
        { 0x21, "sound/sequences/us/21_menu_file_select.m64" },
        { 0x22, "sound/sequences/us/22_cutscene_lakitu.m64" },
    };

    Serializer seq_ser = {}, bnk_ser = {};

    ser_pack(&seq_ser, "HHX", TYPE_SEQ, countof(sequence_table));

    size_t seq_table = ser_reserve(&seq_ser, calcsize("PIX") * countof(sequence_table));
    size_t bnk_table = ser_reserve(&bnk_ser, 2 * countof(sequence_table));

    ser_align(&seq_ser, 16);

    foreach (*seq, sequence_table) {
        size_t start = seq_ser.size, size;
        uint8_t* data = assetextract_get_asset(seq->name, &size);
        ser_add(&seq_ser, data, size);
        ser_align(&seq_ser, 16);
        seq_table = ser_pack_reserved(&seq_ser, seq_table, "PIX", start, seq_ser.size - start);

        bnk_table = ser_pack_reserved(&bnk_ser, bnk_table, "H", bnk_ser.size);
        ser_pack(&bnk_ser, "B", sound_sequences[seq->id].num_banks);
        for (int i = sound_sequences[seq->id].num_banks - 1; i >= 0; i--)
            ser_pack(&bnk_ser, "B", sound_sequences[seq->id].banks[i]->index);
    }

    ser_align(&seq_ser, 64);
    ser_align(&bnk_ser, 16);

    *out_seq_buf = seq_ser.buf; *out_seq_len = seq_ser.size;
    *out_bnk_buf = bnk_ser.buf; *out_bnk_len = bnk_ser.size;
}

static void sound_assemble(
    set(NameToSampleMap) extracted_samples,
    uint8_t** out_ctl_buf, size_t* out_ctl_len,
    uint8_t** out_tbl_buf, size_t* out_tbl_len
) {
    Serializer ser_ctl = {}, ser_tbl = {};

    smart list(int) bank_indexes = list_init(int);
    smart list(int) bank_ordered = list_init(int);
    smart set(NameToIndexMap) indexes = set_init(NameToIndexMap, compare_str);
    for (int i = 0; i < num_sound_banks; i++) {
        push(bank_ordered) = i;

        NameToIndexMap* found;
        if ((found = find(indexes, &sound_bank_list[i]->sample_bank))) {
            sound_bank_list[i]->is_shared = true;
            sound_bank_list[found->value]->is_shared = true;
            push(bank_indexes) = found->value;
        }
        else {
            sound_bank_list[i]->is_shared = false;
            push(bank_indexes) = i;
            push(indexes) = (NameToIndexMap){ .key = sound_bank_list[i]->sample_bank, .value = i };
        }
    }

    serialize_seqfile(&ser_tbl, TYPE_TBL, extracted_samples, serialize_tbl, bank_indexes);
    serialize_seqfile(&ser_ctl, TYPE_CTL, extracted_samples, serialize_ctl, bank_ordered);
    
    *out_ctl_buf = ser_ctl.buf; *out_ctl_len = ser_ctl.size;
    *out_tbl_buf = ser_tbl.buf; *out_tbl_len = ser_tbl.size;
}

static void parse_aifc_book(const uint8_t* data, Aifc* out) {
    data = unpack(">hhh", data, NULL, &out->book.order, &out->book.npredictors);
    out->book.table = malloc(16 * out->book.order * out->book.npredictors);
    for (int i = 0; i < 8 * out->book.order * out->book.npredictors; i++)
        data = unpack(">h", data, &out->book.table[i]);
}

static void parse_aifc_loop(const uint8_t* data, Aifc* out) {
    data = unpack(">HHIIi", data, NULL, NULL, &out->loop.start, &out->loop.end, &out->loop.count);
    for (int i = 0; i < 16; i++)
        data = unpack(">h", data, &out->loop.state[i]);
}

static double parse_f80(const uint8_t* data) {
    uint16_t exp_bits;
    uint64_t mantissa_bits;
    unpack(">HQ", data, &exp_bits, &mantissa_bits);

    uint16_t sign_bit = exp_bits & (1 << 15);
    exp_bits ^= sign_bit;

    int8_t sign = sign_bit ? -1 : 1;

    if (exp_bits == 0 && mantissa_bits == 0) return sign * 0.0;

    double mantissa = (double)mantissa_bits / (double)(1UL << 63);
    return sign * mantissa * pow(2, exp_bits - 0x3FFF);
}

static void parse_aifc(const uint8_t* data, size_t size, Aifc* out, const char* name) {
    const uint8_t* base = data;
    data += 12;

    while (data < base + size) {
        char section_magic[4];
        uint32_t section_length;
        data = unpack("I", data, &section_magic);
        data = unpack(">I", data, &section_length);

        const uint8_t* section_base = data;
        if (strncmp(section_magic, "APPL", 4) == 0 && strncmp((const char*)data, "stoc", 4) == 0) {
            data += 4;

            uint8_t subsection_magic_len;
            data = unpack(">B", data, &subsection_magic_len);
            const char* subsection_magic = (const char*)data;
            data += subsection_magic_len;
            if (strncmp(subsection_magic, "VADPCMCODES", subsection_magic_len) == 0)
                parse_aifc_book(data, out);
            else if (strncmp(subsection_magic, "VADPCMLOOPS", subsection_magic_len) == 0)
                parse_aifc_loop(data, out);
        }
        else if (strncmp(section_magic, "SSND", 4) == 0) {
            out->data = data + 8;
            out->data_size = section_length - 8;
        }
        else if (strncmp(section_magic, "COMM", 4) == 0)
            out->tuning = parse_f80(data + 8) / 32000;

        data = section_base + section_length;
    }
}

static void append_custom_samples(set(NameToSampleMap)* extracted_samples) {
    foreach (*sample, gCustomSoundTable) {
        NameToSampleMap* entry = &push(*extracted_samples);
        entry->key = sample->name;
        parse_aifc(sample->data, sample->size, &entry->value, sample->name);
    }

    // sort the map
    find(*extracted_samples, &(char*){""});
}

static void copy_extended(set(NameToSampleMap)* samples) {
    struct {
        const char *src, *dst;
    } copies[] = {
        { "bowser_organ/00_organ_1", "extended/00_organ_1" },
        { "bowser_organ/01_organ_1_lq", "extended/01_organ_1_lq" },
        { "bowser_organ/02_boys_choir", "extended/02_boys_choir" },
        { "course_start/00_la", "extended/00_la" },
        { "piranha_music_box/00_music_box", "extended/00_music_box" },
        { "instruments/00", "extended/00" },
        { "instruments/01_banjo_1", "extended/01_banjo_1" },
        { "instruments/02", "extended/02" },
        { "instruments/03_human_whistle", "extended/03_human_whistle" },
        { "instruments/04_bright_piano", "extended/04_bright_piano" },
        { "instruments/05_acoustic_bass", "extended/05_acoustic_bass" },
        { "instruments/06_kick_drum_1", "extended/06_kick_drum_1" },
        { "instruments/07_rimshot", "extended/07_rimshot" },
        { "instruments/08", "extended/08" },
        { "instruments/09", "extended/09" },
        { "instruments/0A_tambourine", "extended/0A_tambourine" },
        { "instruments/0B", "extended/0B" },
        { "instruments/0C_conga_stick", "extended/0C_conga_stick" },
        { "instruments/0D_clave", "extended/0D_clave" },
        { "instruments/0E_hihat_closed", "extended/0E_hihat_closed" },
        { "instruments/0F_hihat_open", "extended/0F_hihat_open" },
        { "instruments/10_cymbal_bell", "extended/10_cymbal_bell" },
        { "instruments/11_splash_cymbal", "extended/11_splash_cymbal" },
        { "instruments/12_snare_drum_1", "extended/12_snare_drum_1" },
        { "instruments/13_snare_drum_2", "extended/13_snare_drum_2" },
        { "instruments/14_strings_5", "extended/14_strings_5" },
        { "instruments/15_strings_4", "extended/15_strings_4" },
        { "instruments/16_french_horns", "extended/16_french_horns" },
        { "instruments/17_trumpet", "extended/17_trumpet" },
        { "instruments/18_timpani", "extended/18_timpani" },
        { "instruments/19_brass", "extended/19_brass" },
        { "instruments/1A_slap_bass", "extended/1A_slap_bass" },
        { "instruments/1B_organ_2", "extended/1B_organ_2" },
        { "instruments/1C", "extended/1C" },
        { "instruments/1D", "extended/1D" },
        { "instruments/1E_closed_triangle", "extended/1E_closed_triangle" },
        { "instruments/1F_open_triangle", "extended/1F_open_triangle" },
        { "instruments/20_cabasa", "extended/20_cabasa" },
        { "instruments/21_sine_bass", "extended/21_sine_bass" },
        { "instruments/22_boys_choir", "extended/22_boys_choir" },
        { "instruments/23_strings_1", "extended/23_strings_1" },
        { "instruments/24_strings_2", "extended/24_strings_2" },
        { "instruments/25_strings_3", "extended/25_strings_3" },
        { "instruments/26_crystal_rhodes", "extended/26_crystal_rhodes" },
        { "instruments/27_harpsichord", "extended/27_harpsichord" },
        { "instruments/28_sitar_1", "extended/28_sitar_1" },
        { "instruments/29_orchestra_hit", "extended/29_orchestra_hit" },
        { "instruments/2A", "extended/2A" },
        { "instruments/2B", "extended/2B" },
        { "instruments/2C", "extended/2C" },
        { "instruments/2D_trombone", "extended/2D_trombone" },
        { "instruments/2E_accordion", "extended/2E_accordion" },
        { "instruments/2F_sleigh_bells", "extended/2F_sleigh_bells" },
        { "instruments/30_rarefaction-lahna", "extended/30_rarefaction-lahna" },
        { "instruments/31_rarefaction-convolution", "extended/31_rarefaction-convolution" },
        { "instruments/32_metal_rimshot", "extended/32_metal_rimshot" },
        { "instruments/33_kick_drum_2", "extended/33_kick_drum_2" },
        { "instruments/34_alto_flute", "extended/34_alto_flute" },
        { "instruments/35_gospel_organ", "extended/35_gospel_organ" },
        { "instruments/36_sawtooth_synth", "extended/36_sawtooth_synth" },
        { "instruments/37_square_synth", "extended/37_square_synth" },
        { "instruments/38_electric_kick_drum", "extended/38_electric_kick_drum" },
        { "instruments/39_sitar_2", "extended/39_sitar_2" },
        { "instruments/3A_music_box", "extended/3A_music_box" },
        { "instruments/3B_banjo_2", "extended/3B_banjo_2" },
        { "instruments/3C_acoustic_guitar", "extended/3C_acoustic_guitar" },
        { "instruments/3D", "extended/3D" },
        { "instruments/3E_monk_choir", "extended/3E_monk_choir" },
        { "instruments/3F", "extended/3F" },
        { "instruments/40_bell", "extended/40_bell" },
        { "instruments/41_pan_flute", "extended/41_pan_flute" },
        { "instruments/42_vibraphone", "extended/42_vibraphone" },
        { "instruments/43_harmonica", "extended/43_harmonica" },
        { "instruments/44_grand_piano", "extended/44_grand_piano" },
        { "instruments/45_french_horns_lq", "extended/45_french_horns_lq" },
        { "instruments/46_pizzicato_strings_1", "extended/46_pizzicato_strings_1" },
        { "instruments/47_pizzicato_strings_2", "extended/47_pizzicato_strings_2" },
        { "instruments/48_steel_drum", "extended/48_steel_drum" },
    };

    foreach (*copy, copies) {
        Aifc aifc = find(*samples, &copy->src)->value;
        if (aifc.book.table) aifc.book.table = memcpy(
            malloc(16 * aifc.book.npredictors * aifc.book.order), aifc.book.table,
            16 * aifc.book.npredictors * aifc.book.order
        );
        push(*samples) = (NameToSampleMap){ .key = copy->dst, .value = aifc };
    }
}

void sound_reassemble(
    set(Sound_SampleAsset) samples,
    const uint8_t* ctl_buf, size_t ctl_len,
    const uint8_t* tbl_buf, size_t tbl_len,
    uint8_t** out_ctl_buf, size_t* out_ctl_len,
    uint8_t** out_tbl_buf, size_t* out_tbl_len,
    uint8_t** out_seq_buf, size_t* out_seq_len,
    uint8_t** out_bnk_buf, size_t* out_bnk_len
) {
    smart set(NameToSampleMap) extracted_samples = sound_disassemble(samples, ctl_buf, ctl_len, tbl_buf, tbl_len);
    copy_extended(&extracted_samples);
    append_custom_samples(&extracted_samples);
    sound_assemble(extracted_samples, out_ctl_buf, out_ctl_len, out_tbl_buf, out_tbl_len);
    write_sequences(out_seq_buf, out_seq_len, out_bnk_buf, out_bnk_len);
    foreach (*entry, extracted_samples)
        free(entry->value.book.table);
}
