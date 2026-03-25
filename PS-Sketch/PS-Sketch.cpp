#include "PS-Sketch.h"

#include "Hash.h"

#include <algorithm>
#include <ranges>

PSPSketch::PSPSketch(const Param &param)
    : param(param),
      pSketch(param.pSketch_rows, std::vector<PBucket>(param.pSketch_cols, PBucket{.time = -1, .value = 0.0})),
      sSketch(param.sSketch_rows, std::vector<SBucket>(param.sSketch_cols, SBucket(param.hll_size)))
{
}

void PSPSketch::insert(std::span<const uint8_t> key, std::span<const uint8_t> element, const int time_window)
{
    bool flag = false;
    double min_value = std::numeric_limits<double>::max();
    for (size_t i = 0; i < pSketch.size(); ++i)
    {
        const uint32_t hash_value = hash32(key, i);
        const size_t bucket_index = hash_value % pSketch[i].size();
        PBucket &bucket = pSketch[i][bucket_index];
        if (bucket.time != time_window)
        {
            const auto exp = -(time_window - bucket.time) / this->param.decay_factor;
            bucket.value = 1 + bucket.value * std::exp(exp);
            bucket.time = time_window;
            flag = true;
        }
        if (bucket.value < min_value)
        {
            min_value = bucket.value;
        }
    }

    if (!flag)
        return;

    const uint64_t hash_value = hash64(element);
    const uint64_t shifted_hash_value = hash_value >> static_cast<int>(std::ceil((std::log2(min_value))));
    for (size_t i = 0; i < sSketch.size(); ++i)
    {
        size_t index = hash32(key, i) % sSketch[i].size();
        SBucket &bucket = sSketch[i][index];
        bucket.hll.insert_hashed(hash_value);
        const auto level = std::countl_zero(hash_value) - 1;
        if (level > bucket.level)
        {
            bucket.level = level;
            bucket.key = std::vector<uint8_t>(key.begin(), key.end());
        }
    }
}

double PSPSketch::query(std::span<const uint8_t> key) const
{
    double min_value = std::numeric_limits<double>::max();
    for (size_t i = 0; i < sSketch.size(); ++i)
    {
        const uint32_t index = hash32(key, i) % sSketch[i].size();
        const SBucket &bucket = sSketch[i][index];
        if (std::ranges::equal(bucket.key, key))
        {
            const auto cardinality = bucket.hll.estimate();
            if (cardinality < min_value)
            {
                min_value = cardinality;
            }
        }
    }
    return min_value;
}