#ifndef HYPERLOGLOG_H
#define HYPERLOGLOG_H

#include <cstdint>
#include <span>
#include <vector>
#include <cmath>

constexpr double calc_hll_correct_factor(size_t bucket_num)
{
    double factor = 0.0;
    if (bucket_num < 32)
        factor = 0.673;
    else if (bucket_num < 64)
        factor = 0.697;
    else if (bucket_num < 128)
        factor = 0.709;
    else
        factor = 0.7213 / (1.0 + 1.079 / (bucket_num * 1.0));
    return factor;
}

constexpr double correct_hll_result(double estimate_value, size_t bucket_num, size_t empty_bucket_num)
{
    if (estimate_value <= 2.5 * bucket_num)
    {
        if (empty_bucket_num != 0)
            estimate_value = bucket_num * std::log(static_cast<double>(bucket_num) / static_cast<double>(empty_bucket_num));
    }
    else if (estimate_value <= (1ull << 31) / 15.0)
    {
        estimate_value = estimate_value;
    }
    else
    {
        estimate_value = -std::pow(2, 32) * std::log(1 - estimate_value / std::pow(2, 32));
    }
    return estimate_value;
}

class HyperLogLog
{
public:
    // assert counter_size <= 8
    HyperLogLog(size_t counter_size, size_t counter_num);

public:
    void insert(std::span<const uint8_t> element);
    void insert_hashed(uint64_t hash_value);
    size_t estimate() const;

private:
    std::vector<uint8_t> counters;
    size_t counter_size;
    size_t counter_num;
    double alpha;
};

#endif // HYPERLOGLOG_H