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

#define THRESHOLD 300 / 2
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

std::unordered_map<long long, int> compcar;

inline long long encode(int x, int y) {
    return (long long)x << 32 | y;
}

inline std::pair<int, int> decode(long long x) {
    return std::make_pair(x >> 32, x);
}

void readData(const char *file)
{
    int max_xr = -1;
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

    for (size_t i = 0; i < kTotalLine; i++) {
        if (max_xr < records[i].xr)
            max_xr = records[i].xr;
        inverted_index[records[i].xr][TIME(records[i].ts)].push_front(records[i].car);
    }

    std::cerr << "max_xr = " << max_xr << std::endl;
    munmap(pa, st.st_size);
    close(fd);

    auto end = std::chrono::high_resolution_clock::now();
    std::cerr << "Time cost: " << (end - start).count() / 1e9 << std::endl;
}

void merge(int car, std::unordered_map<int, int> &temp_counter)
{
    for (const auto &e : temp_counter) {
        if (e.second < THRESHOLD)
            continue;
        if (e.first < car)
            compcar[encode(e.first, car)] += e.second;
        else
            compcar[encode(car, e.first)] += e.second;
    }

    temp_counter.clear();
}

void solve()
{
    size_t num_rec = kTotalLine;
    int car, xr, last_car = -1;
    int l, r;
    std::unordered_map<int, int> temp_counter;

    for (size_t i = 0, j; i < num_rec; i = j, last_car = car) {
        car = records[i].car;
        xr = records[i].xr;
        if (car != last_car && last_car >= 0)
            merge(last_car, temp_counter);

        for (j = i + 1; j < num_rec &&
                records[j].car == car &&
                records[j].xr == xr &&
                records[j].ts <= records[j - 1].ts + TIME_SPAN; j++);

        l = TIME(records[i].ts);
        r = std::min(TIME(records[j - 1].ts) + TIME_SPAN, kTotalTime);

        int cnt = 0;
        for (int k = l; k < r; k++) {
            auto &car_list = inverted_index[xr][k];
            for (const auto &e : car_list) {
                if (car == e)
                    continue;
                cnt++;
                temp_counter[e]++;
            }
        }

        if ((car & 0xf) == 0)
            fprintf(stderr, "car = %d\n", car);
    }

    if (last_car >= 0)
        merge(last_car, temp_counter);
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
    solve();
    writeResult();
    return 0;
}
