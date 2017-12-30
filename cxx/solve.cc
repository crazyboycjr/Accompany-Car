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
        count_car[records[i].car]++;
    }

    for (size_t i = 0; i < kTotalLine; i++) {
        int car = records[i].car;
        if (count_car[car] >= THRESHOLD) {
            if (new_car_id[car] == 0) {
                old_car_id[new_car_counter] = car;
                new_car_id[car] = ++new_car_counter;
            }
            inverted_index[records[i].xr][TIME(records[i].ts)].push_front(new_car_id[car] - 1);
        }
    }

    std::cerr << "max_xr = " << max_xr << std::endl;
    munmap(pa, st.st_size);
    close(fd);

    auto end = std::chrono::high_resolution_clock::now();
    std::cerr << "Time cost: " << (end - start).count() / 1e9 << std::endl;
}

static int top = 0, stack[80000], temp_counter[80000];

void merge(int car)
{
    while (top > 0) {
        int j = stack[top--], cary = old_car_id[j];
        if (temp_counter[j] >= THRESHOLD / 2) {
            if (car < cary)
                compcar[encode(car, cary)] += temp_counter[j];
            else
                compcar[encode(cary, car)] += temp_counter[j];
        }
        temp_counter[j] = 0;
    }
}

void merge2(int car, std::unordered_map<int, int> &temp_counter)
{
    for (const auto &e : temp_counter) {
        if (e.second < THRESHOLD / 2) // FIXME(cjr)
            continue;
        if (old_car_id[e.first] < car)
            compcar[encode(old_car_id[e.first], car)] += e.second;
        else
            compcar[encode(car, old_car_id[e.first])] += e.second;
    }

    temp_counter.clear();
}

void solve()
{
    size_t num_rec = kTotalLine;
    int car, xr, last_car = -1;
    int l, r;
    //std::unordered_map<int, int> temp_counter;

    for (size_t i = 0, j; i < num_rec; i = j, last_car = car) {
        car = records[i].car;
        xr = records[i].xr;
        if (count_car[car] < THRESHOLD) {
            j = i + 1;
            continue;
        }
        if (car != last_car && last_car >= 0)
            merge(last_car);
            //merge(last_car, temp_counter);

        for (j = i + 1; j < num_rec &&
                records[j].car == car &&
                records[j].xr == xr &&
                records[j].ts <= records[j - 1].ts + TIME_SPAN; j++);

        l = TIME(records[i].ts);
        r = std::min(TIME(records[j - 1].ts) + TIME_SPAN, kTotalTime);

        for (int k = l; k < r; k++) {
            auto &car_list = inverted_index[xr][k];
            for (const auto &e : car_list) {
                if (car == e)
                    continue;
                if (temp_counter[e]++ == 0)
                    stack[++top] = e;
            }
        }

        if ((car & 0xff) == 0)
            fprintf(stderr, "car = %d\n", car);
    }

    if (last_car >= 0)
        merge(last_car);
        //merge(last_car, temp_counter);
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
