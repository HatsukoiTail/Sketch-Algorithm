#include "HyperLogLog.h"

#include "Hash.h"

#include <cassert>

HyperLogLog::HyperLogLog(size_t counter_size, size_t counter_num)
    : counter_size(counter_size), counter_num(counter_num)
{
    assert(counter_size <= 8);
    const size_t total_size = (counter_size * counter_num + 15) / 8;
    counters.resize(total_size, 0);
    this->alpha = calc_hll_correct_factor(counter_num);
}

void HyperLogLog::insert(std::span<const uint8_t> element)
{
    const uint64_t hash_value = hash64(element);
    insert_hashed(hash_value);
}

void HyperLogLog::insert_hashed(uint64_t hash_value)
{
    const size_t bucket_index = hash_value % counter_num;
    const size_t leading_zeros = std::countr_zero(hash_value >> 32);

    const size_t pos = bucket_index * counter_size; 
    size_t byte_index = pos / 8;
    size_t bit_index = pos % 8;
    uint16_t &counter = *reinterpret_cast<uint16_t *>(counters.data() + byte_index);

    const uint8_t mask = (1 << counter_size) - 1;
    const uint8_t count = (counter >> bit_index) & mask;
    const uint16_t new_count = std::max(count, static_cast<uint8_t>(leading_zeros + 1));

    counter &= ~(static_cast<uint16_t>(mask) << bit_index);
    counter |= (new_count << bit_index);
}

size_t HyperLogLog::estimate() const
{
    double estimate_value = 0.0;
    size_t empty_bucket_num = 0;
    for (size_t i = 0; i < counter_num; ++i)
    {
        const size_t pos = i * counter_size;
        size_t byte_index = pos / 8;
        size_t bit_index = pos % 8;
        const uint16_t counter = *reinterpret_cast<const uint16_t *>(counters.data() + byte_index);
        const uint8_t count = (counter >> bit_index) & ((1 << counter_size) - 1);
        if (count == 0)
        {
            ++empty_bucket_num;
        }
        else
        {
            estimate_value += 1.0 / (1 << count);
        }
    }
    estimate_value = this->counter_num * this->counter_num * this->alpha / estimate_value;
    estimate_value = correct_hll_result(estimate_value, this->counter_num, empty_bucket_num);
    return static_cast<size_t>(std::round(estimate_value));
}
