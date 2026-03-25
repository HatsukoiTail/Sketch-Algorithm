#ifndef PISKETCH_H
#define PISKETCH_H

#include "BloomFilter.h"

#include <cstdint>
#include <span>
#include <vector>

class PISketch
{
public:
    struct Param
    {
        size_t filter_size;
        size_t filter_hash_num;
        size_t bucket_num;
        size_t cell_num_per_bucket;
        int weight_increment;
    };

public:
    PISketch(const Param &param);
    void insert(std::span<const uint8_t> key, std::span<const uint8_t> element);
    int query(std::span<const uint8_t> key) const;
    void reset_filter();

private:
    struct PISketchCell
    {
        std::vector<uint8_t> key;
        int weight;
        int window_number;
        bool valid = false;
    };

private:
    Param param;
    BloomFilter filter;
    std::vector<std::vector<PISketchCell>> buckets;
};

#endif // PISKETCH_H