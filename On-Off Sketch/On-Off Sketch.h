#ifndef ON_OFF_SKETCH_H
#define ON_OFF_SKETCH_H

#include <cstdint>
#include <span>
#include <vector>

class OnOffSketch
{
public:
    struct Param
    {
        size_t array_num;
        size_t bucket_size;
    };

public:
    OnOffSketch(const Param &param);

public:
    void insert(std::span<const uint8_t> key, std::span<const uint8_t> element);
    int query(std::span<const uint8_t> key) const;
    void reset();

private:
    struct OnOffSketchCounter;
    struct OnOffSketchBucket;

private:
    Param param;
    std::vector<OnOffSketchCounter> counters;
    std::vector<std::vector<OnOffSketchBucket>> buckets;
};

struct OnOffSketch::OnOffSketchCounter
{
    bool on_off;
    int count;
};

struct OnOffSketch::OnOffSketchBucket
{
    std::vector<uint8_t> key;
    int count;
    bool on_off;
};

#endif // ON_OFF_SKETCH_H