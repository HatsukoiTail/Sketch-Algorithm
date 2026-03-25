#ifndef PS_PSKETCH_H
#define PS_PSKETCH_H

#include "HyperLogLog.h"

#include <vector>

class PSPSketch
{
public:
    struct Param
    {
        double decay_factor;
        size_t pSketch_rows;
        size_t pSketch_cols;
        size_t sSketch_rows;
        size_t sSketch_cols;
        size_t hll_size;
    };

public:
    PSPSketch(const Param& param);
    void insert(std::span<const uint8_t> key, std::span<const uint8_t> element, const int time_window);
    double query(std::span<const uint8_t> key) const;

private:
    struct PBucket;
    struct SBucket;

private:
    Param param;
    std::vector<std::vector<PBucket>> pSketch;
    std::vector<std::vector<SBucket>> sSketch;
};

struct PSPSketch::PBucket
{
    int time;
    double value;
};

struct PSPSketch::SBucket
{
    SBucket(size_t hll_size) : hll(hll_size, 5), level(0) {}
    HyperLogLog hll;
    size_t level;
    std::vector<uint8_t> key;
};

#endif /* PS_PSKETCH_H */