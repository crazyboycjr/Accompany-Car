#include <ctime>
#include <cstdio>
#include <cassert>

#include <list>
#include <vector>
#include <algorithm>
#include <utility>
#include <thread>
#include <unordered_map>

#define MAX_PROCS 8

#define THRESHOLD 300 / 2
#define TIME_SPAN 180 //3min

#ifdef DEBUG
const int kTotalTime = 86400 * 1;
#else
const int kTotalTime = 86400 * 31;
#endif

const int kMaxCar = 23182818;
const int kStartTime = 1419969600;
#define TIME(x) ((x) - kStartTime)

struct Record {
    int car, xr, ts;
};

std::vector<Record> records;

/* 
 * vector has an extra overhead, so we use list
 * TODO try implement list by hand
 */
std::list<int> inverted_index[1000][kTotalTime];

std::unordered_map<long long, int> compcar;

inline long long encode(int x, int y) {
    return (long long)x << 32 | y;
}

inline std::pair<int, int> decode(long long x) {
    return std::make_pair(x >> 32, x);
}

void readData(const char *file)
{
    int car, xr, ts, max_xr = -1;
    FILE *fp = fopen(file, "r");
    assert(fp);

    /* read can be speed up further */
    for (size_t i = 0; ~fscanf(fp, "%d,%d,%d", &car, &xr, &ts); i++) {
        if (max_xr < xr)
            max_xr = xr;
        if ((i & 0xffff) == 0)
            fprintf(stderr, "i = %ld\n", i);

        inverted_index[xr][TIME(ts)].push_back(car);
        records.push_back((Record){car, xr, ts});
    }

    fclose(fp);
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
    size_t num_rec = records.size();
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
        r = TIME(records[j - 1].ts) + TIME_SPAN;

        for (int k = l; k < r; k++) {
            auto &car_list = inverted_index[xr][k];
            for (const auto &e : car_list) {
                if (car == e)
                    continue;
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
