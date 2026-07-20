#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MIN_MERGE 32
#define MAX_STACK 85

#define elem(base, i) ((void*)((char*)(base) + (i) * size))

typedef int(*Compare)(const void*, const void*);

static void swap(void* a, void* b, size_t size) {
    char t[size];
    memcpy(t, a, size);
    memcpy(a, b, size);
    memcpy(b, t, size);
}

static size_t get_minrun(size_t n) {
    size_t r = 0;
    while (n >= MIN_MERGE) {
        r |= n & 1;
        n >>= 1;
    }
    return n + r;
}

static void binary_insertion_sort(void* base, size_t start, size_t end, size_t size, Compare compare) {
    char pivot[size];
    for (size_t i = start + 1; i < end; i++) {
        memcpy(pivot, elem(base, i), size);

        size_t left = start, right = i;
        while (left < right) {
            size_t mid = left + (right - left) / 2;
            if (compare(elem(base, mid), pivot) <= 0) left = mid + 1;
            else right = mid;
        }

        memmove(elem(base, left + 1),  elem(base, left), (i - left) * size);
        memcpy(elem(base, left), pivot, size);
    }
}

static void merge_sort(void *base, size_t start, size_t mid, size_t end, size_t size, Compare compare) {
    size_t left_len = mid - start;
    size_t right_len = end - mid;
    char buf[(left_len <= right_len ? left_len : right_len) * size];

    if (left_len <= right_len) {
        memcpy(buf, elem(base, start), left_len * size);

        size_t i = 0, j = mid, k = start;
        while (i < left_len && j < end) {
            if (compare(elem(buf, i), elem(base, j)) <= 0)
                memcpy(elem(base, k++), elem(buf, i++), size);
            else
                memcpy(elem(base, k++), elem(base, j++), size);
        }
        while (i < left_len)
            memcpy(elem(base, k++), buf + i++ * size, size);
    }
    else {
        memcpy(buf, elem(base, mid), right_len * size);

        while (left_len > 0 && right_len > 0) {
            if (compare(elem(base, start + left_len - 1), elem(buf, right_len - 1)) > 0)
                memcpy(elem(base, --end), elem(base, start + --left_len), size);
            else memcpy(elem(base, --end), elem(buf, --right_len), size);
        }

        while (right_len > 0) memcpy(elem(base, --end), elem(buf, --right_len), size);
    }
}

typedef struct {
    size_t start;
    size_t length;
} Run;

typedef struct {
    Run runs[MAX_STACK];
    int num_runs;
    void* base;
    size_t size;
    Compare compare;
} RunStack;

static void rs_init(RunStack* rs, void* base, size_t size, Compare compare) {
    rs->num_runs = 0;
    rs->base = base;
    rs->size = size;
    rs->compare = compare;
}

static void rs_push(RunStack* rs, size_t start, size_t length) {
    assert(rs->num_runs < MAX_STACK);
    rs->runs[rs->num_runs].start  = start;
    rs->runs[rs->num_runs].length = length;
    rs->num_runs++;
}

static void rs_merge(RunStack* rs, int i) {
    size_t lo  = rs->runs[i].start;
    size_t mid = lo + rs->runs[i].length;
    size_t hi  = mid + rs->runs[i + 1].length;

    merge_sort(rs->base, lo, mid, hi, rs->size, rs->compare);

    rs->runs[i].length += rs->runs[i + 1].length;
    for (int k = i + 1; k < rs->num_runs - 1; k++)
        rs->runs[k] = rs->runs[k + 1];
    rs->num_runs--;
}

static void rs_merge_collapse(RunStack* rs) {
    while (rs->num_runs > 1) {
        int n = rs->num_runs - 1;
        if (n >= 2 && rs->runs[n - 2].length <= rs->runs[n - 1].length + rs->runs[n].length) {
            if (rs->runs[n - 2].length < rs->runs[n].length) n--;
            rs_merge(rs, n - 1);
        }
        else if (rs->runs[n - 1].length <= rs->runs[n].length)
            rs_merge(rs, n - 1);
        else break;
    }
}

static void rs_merge_force_collapse(RunStack* rs) {
    while (rs->num_runs > 1) {
        int n = rs->num_runs - 1;
        if (n >= 2 && rs->runs[n - 2].length < rs->runs[n].length)
            n--;
        rs_merge(rs, n - 1);
    }
}

static size_t count_run(void* base, size_t start, size_t end, size_t size, Compare compare) {
    if (start + 1 >= end) return end - start;

    size_t run_end = start + 1;

    if (compare(elem(base, start), elem(base, run_end)) > 0) {
        while (run_end < end && compare(elem(base, run_end - 1), elem(base, run_end)) > 0) run_end++;
        for (size_t l = start, r = run_end - 1; l < r; l++, r--)
            swap(elem(base, l), elem(base, r), size);
    }
    else while (run_end < end && compare(elem(base, run_end - 1), elem(base, run_end)) <= 0)
        run_end++;
    return run_end - start;
}

void timsort(void* base, size_t count, size_t size, Compare compare) {
    if (count < 2) return;

    size_t minrun = get_minrun(count);
    RunStack rs;
    rs_init(&rs, base, size, compare);

    size_t start = 0;
    while (start < count) {
        size_t run_len = count_run(base, start, count, size, compare);

        if (run_len < minrun) {
            size_t force = count - start < minrun ? count - start : minrun;
            binary_insertion_sort(base, start, start + force, size, compare);
            run_len = force;
        }

        rs_push(&rs, start, run_len);
        rs_merge_collapse(&rs);

        start += run_len;
    }

    rs_merge_force_collapse(&rs);

    return;
}
