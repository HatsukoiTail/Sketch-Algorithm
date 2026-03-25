#include "SPSketch.h"

#include "Hash.h"

#include <algorithm>
#include <random>
#include <ranges>

SPSketch::SPSketch(const Param &param)
    : param(param), buckets(param.rows, std::vector<Bucket>(param.cols, {.key = {}, .count = 0, .time_slot = 0}))
{
}

void SPSketch::insert(std::span<const uint8_t> key, std::span<const uint8_t> element, const int time_slot)
{
    std::vector<Bucket *> bucket_ptrs(this->buckets.size(), nullptr);
    for (size_t i = 0; i < this->buckets.size(); i++)
    {
        const auto hash_value = hash32(key, i);
        const size_t index = hash_value % this->buckets[i].size();
        bucket_ptrs[i] = &this->buckets[i][index];
    }

    for (auto &bucket_ptr : bucket_ptrs)
    {
        auto &bucket = *bucket_ptr;
        if (std::ranges::equal(bucket.key, key))
        {
            if (bucket.time_slot != time_slot)
            {
                auto delta = time_slot - bucket.time_slot;
                if (delta >= this->param.max_delay)
                {
                    bucket.count = 0;
                }
                else
                {
                    static std::mt19937_64 engine(std::random_device{}());
                    static std::uniform_real_distribution<double> dist(0.0, 1.0);
                    const double pro = dist(engine);
                    if (pro < static_cast<double>(bucket.count) / this->param.max_delay)
                    {
                        bucket.count -= 1;
                    }
                }
                bucket.count += 1;
                bucket.time_slot = time_slot;
            }
            return;
        }
    }

    for (auto bucket_ptr : bucket_ptrs)
    {
        auto &bucket = *bucket_ptr;
        if (bucket.key.empty())
        {
            bucket.key = std::vector<uint8_t>(key.begin(), key.end());
            bucket.count = 1;
            bucket.time_slot = time_slot;
            return;
        }
    }

    auto min_bucket_ptr = *std::ranges::min_element(bucket_ptrs,
                                                    [](const Bucket *a, const Bucket *b)
                                                    {
                                                        return a->count < b->count;
                                                    });
    if (min_bucket_ptr->time_slot != time_slot)
    {
        if (min_bucket_ptr->count > 0)
        {
            min_bucket_ptr->count -= 1;
        }
    }
    if (min_bucket_ptr->count == 0)
    {
        min_bucket_ptr->key = std::vector<uint8_t>(key.begin(), key.end());
        min_bucket_ptr->count = 1;
        min_bucket_ptr->time_slot = time_slot;
    }
}

int SPSketch::query(std::span<const uint8_t> key) const
{
    std::vector<size_t> indexs(this->buckets.size(), 0);
    for (size_t i = 0; i < this->buckets.size(); ++i)
    {
        const auto hash_value = hash32(key, i);
        const size_t index = hash_value % this->buckets[i].size();
        indexs[i] = index;
    }

    for (size_t i = 0; i < this->buckets.size(); ++i)
    {
        const auto &bucket = this->buckets[i][indexs[i]];
        if (std::ranges::equal(bucket.key, key))
        {
            return static_cast<int>(bucket.count);
        }
    }

    return 0;
}