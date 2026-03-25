#include "PISketch.h"

#include "Hash.h"

#include <algorithm>

PISketch::PISketch(const Param &param)
    : param(param),
      filter(param.filter_size, param.filter_hash_num),
      buckets(param.bucket_num, std::vector<PISketchCell>(param.cell_num_per_bucket,
                                                          PISketchCell{.key = {}, .weight = 0, .window_number = 0, .valid = false}))
{
}

void PISketch::insert(std::span<const uint8_t> key, std::span<const uint8_t> element)
{
    const bool exist = this->filter.update(key);
    const int weight = exist ? -1 : this->param.weight_increment;
    const size_t index = hash32(key) % buckets.size();
    auto &bucket = buckets[index];

    int min_index = -1;
    for (size_t i = 0; i < bucket.size(); ++i)
    {
        auto &cell = bucket[i];
        if (!cell.valid)
            continue;
        if (std::ranges::equal(cell.key, key))
        {
            cell.weight += weight;
            if (weight > 0)
                cell.window_number++;
            return;
        }
        if (min_index == -1 || cell.weight < bucket[min_index].weight)
            min_index = i;
    }
    for (size_t i = 0; i < bucket.size(); ++i)
    {
        auto &cell = bucket[i];
        if (cell.valid)
            continue;
        cell.key = std::vector<uint8_t>(key.begin(), key.end());
        cell.weight = weight;
        cell.window_number = 1;
        cell.valid = true;
        return;
    }
    bucket[min_index].weight -= 1;
    if (bucket[min_index].weight < 0)
    {
        bucket[min_index].key = std::vector<uint8_t>(key.begin(), key.end());
        bucket[min_index].weight = weight + 1;
        bucket[min_index].valid = true;
    }
}

int PISketch::query(std::span<const uint8_t> key) const
{
    const size_t index = hash32(key) % buckets.size();
    const auto &bucket = buckets[index];
    for (const auto &cell : bucket)
    {
        if (!cell.valid)
            continue;
        if (std::ranges::equal(cell.key, key))
        {
            return cell.weight;
        }
    }
    return -1;
}

void PISketch::reset_filter()
{
    filter.reset();
}