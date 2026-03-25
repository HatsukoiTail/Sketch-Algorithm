#include "BloomFilter.h"

#include "Hash.h"

#include <cassert>

BloomFilter::BloomFilter(size_t size, size_t hash_num)
    : size(size), hash_num(hash_num), bit_vector(size, false)
{
}

bool BloomFilter::update(std::span<const uint8_t> data)
{
    bool updated = false;
    for (size_t i = 0; i < hash_num; ++i)
    {
        size_t hash_value = hash32(data, i);
        size_t index = hash_value % size;
        if (bit_vector[index])
            continue;
        bit_vector[index] = true;
        updated = true;
    }
    return !updated;
}

void BloomFilter::set(size_t index, bool value)
{
    assert(index < size);
    this->bit_vector[index] = value;
}

bool BloomFilter::query(std::span<const uint8_t> data) const
{
    for (size_t i = 0; i < hash_num; ++i)
    {
        size_t hash_value = hash32(data, i);
        size_t index = hash_value % size;
        if (!bit_vector[index])
            return false;
    }
    return true;
}

void BloomFilter::reset()
{
    bit_vector.assign(this->size, false);
}

size_t BloomFilter::bit_size() const
{
    return this->size;
}
