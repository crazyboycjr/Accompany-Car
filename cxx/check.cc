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
    bool operator < (const Record &rec) const {
        return xr < rec.xr || (xr == rec.xr && ts < rec.ts);
    }
} __attribute__((packed));

static Record records[kTotalLine];

std::vector<Record> appear[80000];

static int count_car[kMaxCar];
static int new_car_counter, new_car_id[kMaxCar], old_car_id[80000];

inline bool meet(const Record &r1, const Record &r2)
{
    return r1.xr == r2.xr && abs(r1.ts - r2.ts) < TIME_SPAN;
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
    Record *r;
    for (r = records; r < records + kTotalLine; r++) {
        if (max_xr < r->xr)
            max_xr = r->xr;
        count_car[r->car]++;
    }

    for (r = records; r < records + kTotalLine; r++) {
        int car = r->car;
        if (count_car[car] >= THRESHOLD && count_car[car] < 10000) {
            if (new_car_id[car] == 0) {
                old_car_id[new_car_counter] = car;
                new_car_id[car] = ++new_car_counter;
            }
            appear[new_car_id[car] - 1].push_back({car, r->xr, r->ts});
        }
    }

    std::cerr << "max_xr = " << max_xr << std::endl;
}

void solve(int carx, int cary)
{
    auto &arr1 = appear[new_car_id[carx] - 1];
    auto &arr2 = appear[new_car_id[cary] - 1];

    int count = 0;
    size_t j = 0;
    for (size_t i = 0; i < arr1.size(); i++) {
        for (; j < arr2.size() && arr2[j] < arr1[i]; j++);
        if (meet(arr2[j], arr1[i])) {
            count++;
            printf("xr: %d, %d %d\n", arr1[i].xr, arr1[i].ts, arr2[j].ts);
        }
    }

    printf("accompany times = %d\n", count);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [input]\n", argv[0]);
        return 1;
    }

    readData(argv[1]);
    init();

    std::cout << "Ready" << std::endl;

    int carx, cary;
    while (~scanf("%d%d", &carx, &cary)) {
        printf("%d %d\n", count_car[carx], count_car[cary]);
        if (count_car[carx] < THRESHOLD || count_car[carx] > 10000 ||
            count_car[cary] < THRESHOLD || count_car[cary] > 10000) {
                printf("invalid input\n");
                continue;
        }
        solve(carx, cary);
    }
    return 0;
}
