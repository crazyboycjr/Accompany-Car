#include <ctime>
#include <cstdio>
#include <cassert>

#include <fcntl.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <chrono>
#include <vector>
#include <iostream>
#include <algorithm>
#include <utility>
#include <thread>
#include <unordered_map>
#include <forward_list>

#define MAX_PROCS 8

#define THRESHOLD 300
#define TIME_SPAN 180 //3min

#ifdef DEBUG
const int kTotalTime = 86400 * 1;
#else
const int kTotalTime = 86400 * 31;
#endif

const int kTotalLine = 275893209;
const int kMaxCar = 23182818;
const int kStartTime = 1419969600;
#define TIME(x) ((x) - kStartTime)

struct Record {
    int car, xr, ts;
} __attribute__((packed));

static Record records[kTotalLine];

std::forward_list<int> inverted_index[1000][kTotalTime];

static int count_car[kMaxCar];
static int new_car_counter, new_car_id[kMaxCar], old_car_id[80000];

std::unordered_map<long long, int> compcar;

inline long long encode(int x, int y) {
    return (long long)x << 32 | y;
}

inline std::pair<int, int> decode(long long x) {
    return std::make_pair(x >> 32, x);
}

void readData(const char *file)
{
    std::cerr << "Start" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    char *pa;
    struct stat st;
    int fd = open(file, O_RDONLY);
    assert(fstat(fd, &st) == 0);

    pa = (char *)mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    int *data = (int *)records;
    for (char *t = pa; t < pa + st.st_size; t++) {
        if ('0' <= *t && *t <= '9')
            *data = *data * 10 + *t - '0';
        else
            data++;
    }

    munmap(pa, st.st_size);
    close(fd);

    auto end = std::chrono::high_resolution_clock::now();
    std::cerr << "Time cost: " << (end - start).count() / 1e9 << std::endl;
}

void init()
{
    int max_xr = -1;
    Record *i, *j;

    for (i = records; i < records + kTotalLine; i++) {
        if (max_xr < i->xr)
            max_xr = i->xr;
        count_car[i->car]++;
    }

    int car, xr;
    for (i = records; i  < records + kTotalLine; i = j) {
        car = i->car;
        xr = i->xr;
        if (count_car[car] < THRESHOLD || count_car[car] > 10000) {
            j = i + 1;
            continue;
        }
        for (j = i + 1; j < records + kTotalLine &&
                j->car == car &&
                j->xr == xr &&
                j->ts <= (j - 1)->ts + TIME_SPAN; j++);

        if (new_car_id[car] == 0) {
            old_car_id[new_car_counter] = car;
            new_car_id[car] = ++new_car_counter;
        }

        inverted_index[i->xr][TIME(i->ts)].push_front(new_car_id[car] - 1);
        if (j != i + 1) {
            inverted_index[i->xr][TIME((j - 1)->ts)]
                .push_front(new_car_id[car] - 1);
            for (int ts = TIME(i->ts) + TIME_SPAN;
                     ts < TIME((j - 1)->ts);
                     ts += TIME_SPAN)
                inverted_index[i->xr][ts].push_front(new_car_id[car] - 1);
        }
    }

    std::cerr << "max_xr = " << max_xr << std::endl;
}

static int top = 0, stack[80000], temp_counter[80000];

void merge(int car)
{
    while (top) {
        int j = stack[top--], cary = old_car_id[j];
        if (temp_counter[j] >= THRESHOLD) {
            if (car < cary)
                compcar[encode(car, cary)] += temp_counter[j];
            else
                compcar[encode(cary, car)] += temp_counter[j];
        }
        temp_counter[j] = 0;
    }
}

void solve()
{
    size_t num_rec = kTotalLine;
    int car, xr, last_car = -1;
    int l, r;

    for (Record *i = records, *j; i < records + num_rec; i = j) {
        car = i->car;
        xr = i->xr;
        if (count_car[car] < THRESHOLD || count_car[car] > 10000) {
            j = i + 1;
            continue;
        }
        if (car != last_car && last_car >= 0)
            merge(last_car);

        for (j = i + 1; j < records + num_rec &&
                        j->car == car &&
                        j->xr == xr &&
                        j->ts <= (j - 1)->ts + TIME_SPAN; j++);

        l = std::max(TIME(i->ts) - TIME_SPAN, 0);
        r = std::min(TIME((j - 1)->ts) + TIME_SPAN, kTotalTime);

        for (int k = l; k < r; k++) {
            auto &car_list = inverted_index[xr][k];
            for (const auto &e : car_list) {
                if (new_car_id[car] - 1 >= e)
                    continue;
                if (temp_counter[e]++ == 0)
                    stack[++top] = e;
            }
        }

        last_car = car;

        if ((car & 0xff) == 0)
            fprintf(stderr, "car = %d\n", car);
    }

    if (last_car >= 0)
        merge(last_car);
}

void writeResult()
{
    int x, y;
    for (const auto &e : compcar) {
        x = e.first >> 32;
        y = (int)e.first;
        printf("%d %d %d\n", x, y, e.second);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [input]\n", argv[0]);
        return 1;
    }

    readData(argv[1]);
    init();
    solve();
    writeResult();
    return 0;
}
