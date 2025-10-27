#ifndef FUNNELSKETCH_H
#define FUNNELSKETCH_H

#include <cstdint>
#include <span>
#include <vector>

class FunnelSketch
{
private:
    struct Bucket
    {
        std::vector<uint8_t> array_8bit;
        std::vector<uint16_t> array_16bit;
        std::vector<uint8_t> key;
        uint16_t value = 0;
    };

public:
    struct Param
    {
        size_t light_layer = 0;
        size_t middle_layer = 0;
        size_t length_4bit = 0;
        size_t length_bucket = 0;
        size_t length_32bit = 0;
        size_t length_8bit_per_bucket = 0;
        size_t length_16bit_per_bucket = 0;
        double threshold = 2.0;
    };

public:
    explicit FunnelSketch(const Param& param);
    uint32_t insert(std::span<const uint8_t> key, uint32_t count = 1);
    uint32_t query(std::span<const uint8_t> key) const;

private:
    void init(const Param& param);
    static uint32_t hash(std::span<const uint8_t> key, size_t seed);

private:
    double threshold_ratio = 0.0;

    std::vector<std::vector<uint8_t>> lightCU;
    std::vector<std::vector<Bucket>> buckets;
    std::vector<uint32_t> heavyArray;
};

#endif // FUNNELSKETCH_H
