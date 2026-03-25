#include "VHLL.h"

#include "Hash.h"
#include "HyperLogLog.h"

#include <algorithm>
#include <cmath>

VHLL::VHLL(size_t counter_num, size_t hll_size)
    : counter_num(counter_num), hll_size(hll_size)
{
    const size_t vector_size = std::ceil(counter_num * counter_size / 8.0);
    this->counters.resize(vector_size, 0);
    this->counter_bits = std::ceil(std::log2(hll_size));
    this->hll_correction_factor = calc_hll_correct_factor(hll_size);
    this->global_correction_factor = calc_hll_correct_factor(counter_num);
    this->flag = 0;
    this->updated = false;
}

void VHLL::insert(std::span<const uint8_t> key, std::span<const uint8_t> element)
{
    const auto element_hash = hash64(element);
    const uint32_t mask = (1 << this->counter_bits) - 1;
    const uint32_t index = (element_hash >> (64 - this->counter_bits)) & mask;
    const uint32_t low32 = static_cast<uint32_t>(element_hash);
    const size_t leading_zero = low32 ? std::countr_zero(low32) : 32;

    const auto key_hash = hash32(key, index);
    const size_t counter_index = key_hash % this->counter_num;

    const size_t byte_index = counter_index * this->counter_size / 8;
    const size_t bit_index = counter_index * this->counter_size % 8;

    if (bit_index + this->counter_size <= 8)
    {
        uint8_t &byte = this->counters[byte_index];
        uint8_t current_value = (byte >> bit_index) & ((1 << this->counter_size) - 1);
        if (leading_zero > current_value)
        {
            byte &= ~(((1 << this->counter_size) - 1) << bit_index);
            byte |= (leading_zero << bit_index);
            this->updated = true;
        }
    }
    else
    {
        uint16_t &two_bytes = *reinterpret_cast<uint16_t *>(&this->counters[byte_index]);
        uint16_t current_value = (two_bytes >> bit_index) & ((1 << this->counter_size) - 1);
        if (leading_zero > current_value)
        {
            two_bytes &= ~(((1 << this->counter_size) - 1) << bit_index);
            two_bytes |= (leading_zero << bit_index);
            this->updated = true;
        }
    }
}

size_t VHLL::estimate(std::span<const uint8_t> key) const
{
    estimate_global();

    std::vector<uint32_t> values(this->hll_size, 0);
    for (size_t i = 0; i < this->hll_size; ++i)
    {
        const auto key_hash = hash32(key, i);
        const size_t counter_index = key_hash % this->counter_num;

        const size_t byte_index = counter_index * this->counter_size / 8;
        const size_t bit_index = counter_index * this->counter_size % 8;

        uint8_t current_value;
        if (bit_index + this->counter_size <= 8)
        {
            current_value = (this->counters[byte_index] >> bit_index) & ((1 << this->counter_size) - 1);
        }
        else
        {
            const uint16_t two_bytes = *reinterpret_cast<const uint16_t *>(&this->counters[byte_index]);
            current_value = (two_bytes >> bit_index) & ((1 << this->counter_size) - 1);
        }
        values[i] = current_value;
    }

    size_t zero_count = std::count(values.begin(), values.end(), 0);
    double estimate_value = 0.0;
    for (const auto &value : values)
    {
        if (value != 0)
        {
            estimate_value += 1.0 / (1 << value);
        }
        else
        {
            estimate_value += 1.0;
        }
    }

    estimate_value = this->hll_size * this->hll_size * this->hll_correction_factor / estimate_value;
    estimate_value = correct_hll_result(estimate_value, this->hll_size, zero_count);

    auto final_estimate = static_cast<double>(this->counter_num * this->hll_size) / (this->counter_num - this->hll_size) * ((estimate_value / this->hll_size) - (this->global_estimate / this->counter_num));

    return static_cast<size_t>(std::max(std::ceil(final_estimate), 0.0));
}

void VHLL::reset()
{
    this->flag = 0;
    this->global_estimate = 0.0;
    std::fill(this->counters.begin(), this->counters.end(), 0);
}

double VHLL::estimate_global() const
{
    if (!this->updated)
        return this->global_estimate;
    // 计算全局估计值
    size_t zero_count = 0;
    double estimate_value = 0.0;
    for (size_t i = 0; i < this->counter_num; ++i)
    {
        const size_t byte_index = i * this->counter_size / 8;
        const size_t bit_index = i * this->counter_size % 8;

        uint8_t current_value;
        if (bit_index + this->counter_size <= 8)
        {
            current_value = (this->counters[byte_index] >> bit_index) & ((1 << this->counter_size) - 1);
        }
        else
        {
            const uint16_t two_bytes = *reinterpret_cast<const uint16_t *>(&this->counters[byte_index]);
            current_value = (two_bytes >> bit_index) & ((1 << this->counter_size) - 1);
        }

        if (current_value != 0)
        {
            estimate_value += 1.0 / (1 << current_value);
        }
        else
        {
            estimate_value += 1.0;
            ++zero_count;
        }
    }
    estimate_value = this->counter_num * this->counter_num * this->global_correction_factor / estimate_value;
    estimate_value = correct_hll_result(estimate_value, this->counter_num, zero_count);
    this->global_estimate = estimate_value;

    this->updated = false;

    return estimate_value;
}
