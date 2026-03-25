#ifndef SP_SKETCH_H
#define SP_SKETCH_H

#include <cstdint>
#include <span>
#include <vector>

class SPSketch
{
public:
    struct Param
    {
        size_t rows;
        size_t cols;
        size_t max_delay;
    };

public:
    SPSketch(const Param& param);
    SPSketch(const SPSketch& other) = delete;
    SPSketch& operator=(const SPSketch& other) = delete;

public:
    void insert(std::span<const uint8_t> key, std::span<const uint8_t> element, const int time_slot);
    int query(std::span<const uint8_t> key) const;

private:
    struct Bucket
    {
        std::vector<uint8_t> key;
        int count;
        int time_slot;
    };

private:
    Param param;
    std::vector<std::vector<Bucket>> buckets;
};

#endif // SP_SKETCH_H