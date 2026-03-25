#include "LinearCounting.h"

#include "Hash.h"

#include <cmath>

LinearCounting::LinearCounting(size_t bit_vector_size)
{
    this->bit_vector_size = bit_vector_size;
    this->bit_vector.resize((bit_vector_size + 7) / 8, 0);
}

void LinearCounting::insert(std::span<const uint8_t> element)
{
    uint32_t hash_value = hash32(element);
    const size_t index = hash_value % this->bit_vector_size;
    const size_t byte_index = index / 8;
    const size_t bit_index = index % 8;
    bit_vector[byte_index] |= (1 << bit_index);
}

double LinearCounting::estimate() const
{
    size_t zero_bit_num = 0;
    for (const auto byte : this->bit_vector)
    {
        zero_bit_num += std::popcount(byte);
    }
    double estimate = -static_cast<double>(this->bit_vector_size) * std::log(static_cast<double>(this->bit_vector_size - zero_bit_num) / this->bit_vector_size);
    return estimate;
}

void LinearCounting::reset()
{
    this->bit_vector.assign(this->bit_vector.size(), 0);
}
