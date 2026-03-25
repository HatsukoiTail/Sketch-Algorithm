#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

#include <cstdint>
#include <span>
#include <vector>

class BloomFilter
{
public:
    BloomFilter(size_t size, size_t hash_num);
    bool update(std::span<const uint8_t> data);
    void set(size_t index, bool value = true);
    // return false if data has not been seen before
    bool query(std::span<const uint8_t> data) const;
    void reset();
    size_t bit_size() const;

private:
    size_t size;
    size_t hash_num;
    std::vector<bool> bit_vector;
};

#endif // BLOOMFILTER_H