#ifndef SKETCH_ALGORITHM_HASH_H
#define SKETCH_ALGORITHM_HASH_H

#include "Murmur.h"

#include <array>
#include <cstdint>
#include <span>

inline uint32_t hash32(std::span<const uint8_t> key, uint32_t seed = 0)
{
    uint32_t out;
    MurmurHash3_x86_32(key.data(), key.size(), seed, &out);
    return out;
}

inline uint64_t hash64(std::span<const uint8_t> key, uint32_t seed = 0)
{
    uint64_t out[2];
    MurmurHash3_x64_128(key.data(), key.size(), seed, out);
    return out[0];
}

inline std::array<uint8_t, 16> hash128(std::span<const uint8_t> key, uint32_t seed = 0)
{
    std::array<uint8_t, 16> out;
    MurmurHash3_x86_128(key.data(), key.size(), seed, out.data());
    return out;
}

inline uint32_t hash32(const void* key, int len, uint32_t seed = 0)
{
    uint32_t out;
    MurmurHash3_x86_32(key, len, seed, &out);
    return out;
}

inline uint64_t hash64(const void* key, int len, uint32_t seed = 0)
{
    uint64_t out[2];
    MurmurHash3_x64_128(key, len, seed, out);
    return out[0];
}

inline std::array<uint8_t, 16> hash128(const void* key, int len, uint32_t seed = 0)
{
    std::array<uint8_t, 16> out;
    MurmurHash3_x86_128(key, len, seed, out.data());
    return out;
}

#endif // SKETCH_ALGORITHM_HASH_H