#include "On-Off Sketch.h"

#include "Hash.h"

#include <algorithm>

OnOffSketch::OnOffSketch(const Param &param)
    : param(param),
      counters(param.array_num, OnOffSketchCounter{.on_off = false, .count = 0}),
      buckets(param.array_num, std::vector<OnOffSketchBucket>(param.bucket_size,
                                                              OnOffSketchBucket{.key = {}, .count = 0, .on_off = false}))
{
}

void OnOffSketch::insert(std::span<const uint8_t> key, std::span<const uint8_t> element)
{
    const size_t index = hash32(key) % this->counters.size();
    auto &counter = this->counters[index];

    auto &buckets = this->buckets[index];
    auto bucket = std::ranges::find_if(buckets, [key](const OnOffSketchBucket &b) { return std::ranges::equal(b.key, key); });
    if (bucket == buckets.end())
    {
        if (!counter.on_off)
        {
            counter.count++;
            counter.on_off = true;
        }
    }
    else
    {
        if (!bucket->on_off)
        {
            bucket->count++;
            bucket->on_off = true;
        }
    }

    auto &min_bucket = *std::min_element(buckets.begin(), buckets.end(), [](const OnOffSketchBucket &a, const OnOffSketchBucket &b)
                                         { return a.count < b.count; });
    if (counter.count >= min_bucket.count)
    {
        min_bucket.key = std::vector<uint8_t>(key.begin(), key.end());
        std::swap(min_bucket.count, counter.count);
        std::swap(min_bucket.on_off, counter.on_off);
    }
}

int OnOffSketch::query(std::span<const uint8_t> key) const
{
    const size_t index = hash32(key) % this->counters.size();
    const auto &counter = this->counters[index];
    const auto &buckets = this->buckets[index];
    auto bucket = std::ranges::find_if(buckets, [key](const OnOffSketchBucket &b) { return std::ranges::equal(b.key, key); });
    if (bucket == buckets.end())
    {
        return counter.count;
    }
    else
    {
        return bucket->count;
    }
}

void OnOffSketch::reset()
{
    for (auto &counter : this->counters)
    {
        counter.on_off = false;
    }
    for (auto &bucket : this->buckets)
    {
        for (auto &cell : bucket)
        {
            cell.on_off = false;
        }
    }
}