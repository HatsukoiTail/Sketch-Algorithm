#include "FunnelSketch.h"

// #include "hash.h"

#include <algorithm>

FunnelSketch::FunnelSketch(const Param &param)
{
    this->init(param);
}

void FunnelSketch::init(const Param &param)
{
    this->lightCU.resize(param.light_layer, std::vector<uint8_t>(param.length_4bit / 2, 0));
    this->buckets.resize(param.middle_layer,
                         std::vector<Bucket>(param.length_bucket, Bucket{
                                                                      .array_8bit = std::vector<uint8_t>(param.length_8bit_per_bucket, 0),
                                                                      .array_16bit = std::vector<uint16_t>(param.length_16bit_per_bucket, 0),
                                                                      .key = {},
                                                                      .value = 0}));
    this->heavyArray.resize(param.length_32bit, 0);
}

uint32_t FunnelSketch::hash(std::span<const uint8_t> key, size_t seed)
{
    // replace hash function here
    uint32_t out;
    // MurmurHash3_x86_32(&key, sizeof(key), static_cast<uint32_t>(seed), &out);
    for (const auto &k : key)
    {
        out = k ^ out;
    }
    return out;
}

uint32_t FunnelSketch::insert(std::span<const uint8_t> flowkey, uint32_t count)
{
    // caclulate hash values
    const size_t max_layer_num = std::max(this->lightCU.size(), this->buckets.size());
    std::vector<uint64_t> hash_values(max_layer_num, 0);
    for (auto layer = 0; layer < max_layer_num; ++layer)
    {
        hash_values[layer] = this->hash(flowkey, layer);
    }

    std::vector<std::pair<bool, std::reference_wrapper<uint8_t>>> lightCU_refs;
    for (size_t layer = 0; layer < this->lightCU.size(); ++layer)
    {
        size_t index = (hash_values[layer] % (this->lightCU[layer].size() * 2));
        const bool is_low = !(index & 0x01);
        lightCU_refs.emplace_back(is_low, std::ref(this->lightCU[layer][index / 2]));
    }

    const auto &min_light_counter = *std::min_element(lightCU_refs.begin(), lightCU_refs.end(),
                                                      [](const auto &a, const auto &b)
                                                      {
                                                          uint8_t a_value = a.first ? (a.second.get() & 0x0F) : (a.second.get() >> 4);
                                                          uint8_t b_value = b.first ? (b.second.get() & 0x0F) : (b.second.get() >> 4);
                                                          return a_value < b_value;
                                                      });

    const uint8_t min_light_value = min_light_counter.first ? (min_light_counter.second.get() & 0x0F) : (min_light_counter.second.get() >> 4);

    const uint32_t min_light_result = min_light_value + count;

    if (min_light_result < 15)
    {
        for (auto &lightCU_ref : lightCU_refs)
        {
            if (lightCU_ref.first)
            {
                const uint8_t value = lightCU_ref.second.get() & 0x0F;
                const uint8_t counter_result = (std::min<uint8_t>(value + count, min_light_result) & 0x0F) | (lightCU_ref.second.get() & 0xF0);
                lightCU_ref.second.get() = counter_result;
            }
            else
            {
                const uint8_t value = lightCU_ref.second.get() >> 4;
                const uint8_t counter_result = (std::min<uint8_t>(value + count, min_light_result) << 4) | (lightCU_ref.second.get() & 0x0F);
                lightCU_ref.second.get() = counter_result;
            }
        }
        return min_light_result;
    }
    else
    {
        for (auto &lightCU_ref : lightCU_refs)
        {
            if (lightCU_ref.first)
            {
                lightCU_ref.second.get() |= 0x0F;
            }
            else
            {
                lightCU_ref.second.get() |= 0xF0;
            }
        }
    }

    // insert remaining count to 8bit array
    uint32_t remaining_count = count - (14 - min_light_value);
    std::vector<std::reference_wrapper<uint8_t>> counter_8bit_refs;
    for (size_t layer = 0; layer < this->buckets.size(); ++layer)
    {
        size_t index = (hash_values[layer] % this->buckets[layer].size());
        auto &array_8bit = this->buckets[layer][index].array_8bit;
        size_t index_8bit = index % array_8bit.size();
        counter_8bit_refs.push_back(std::ref(array_8bit[index_8bit]));
    }

    const uint8_t min_8bit = *std::min_element(counter_8bit_refs.begin(), counter_8bit_refs.end(),
                                               [](const auto &a, const auto &b)
                                               {
                                                   return a.get() < b.get();
                                               });

    const uint32_t min_8bit_result = min_8bit + remaining_count;

    if (min_8bit_result < 255)
    {
        for (auto &counter_ref : counter_8bit_refs)
        {
            const uint8_t value = counter_ref.get();
            counter_ref.get() = std::min<uint8_t>(min_8bit_result, value + remaining_count);
        }
        return 14 + min_light_result;
    }
    else
    {
        for (auto &counter_ref : counter_8bit_refs)
        {
            counter_ref.get() = 255;
        }
        remaining_count -= (254 - min_8bit);
    }

    // insert remaining count to kv
    std::vector<std::reference_wrapper<Bucket>> bucket_refs;
    for (size_t layer = 0; layer < this->buckets.size(); ++layer)
    {
        const size_t index = (hash_values[layer] % this->buckets[layer].size());
        bucket_refs.push_back(std::ref(this->buckets[layer][index]));
    }
    auto match_kv_bucket = std::find_if(bucket_refs.begin(), bucket_refs.end(),
                                        [flowkey](const auto &bucket_ref)
                                        {
                                            return (bucket_ref.get().key.size() == flowkey.size() &&
                                                    std::equal(bucket_ref.get().key.begin(), bucket_ref.get().key.end(), flowkey.begin())) ||
                                                   (bucket_ref.get().key.empty());
                                        });
    if (match_kv_bucket != bucket_refs.end())
    {
        auto &bucket = match_kv_bucket->get();
        bucket.key = std::vector<uint8_t>(flowkey.begin(), flowkey.end());
        if (bucket.value + remaining_count < 65535)
        {
            bucket.value += remaining_count;
            return 14 + 254 + bucket.value;
        }
        else
        {
            remaining_count -= (65534 - bucket.value);
            bucket.value = 65535;
        }
        const size_t index_32bit = hash_values[0] % this->heavyArray.size();
        this->heavyArray[index_32bit] += remaining_count;
        return 14 + 254 + 65534 + this->heavyArray[index_32bit];
    }

    // insert remaining count to 16bit array
    std::vector<std::reference_wrapper<uint16_t>> counter_16bit_refs;
    for (size_t layer = 0; layer < this->buckets.size(); ++layer)
    {
        size_t index = (hash_values[layer] % this->buckets[layer].size());
        auto &array_16bit = bucket_refs[layer].get().array_16bit;
        size_t index_16bit = index % array_16bit.size();
        counter_16bit_refs.push_back(std::ref(array_16bit[index_16bit]));
    }

    const uint16_t min_16bit = *std::min_element(counter_16bit_refs.begin(), counter_16bit_refs.end(),
                                                 [](const auto &a, const auto &b)
                                                 {
                                                     return a.get() < b.get();
                                                 });

    const uint32_t min_16bit_result = min_16bit + remaining_count;

    uint32_t return_value = 0;

    if (min_16bit_result < 65535)
    {
        for (auto &counter_ref : counter_16bit_refs)
        {
            const uint16_t value = counter_ref.get();
            counter_ref.get() = std::min<uint16_t>(min_16bit_result, value + remaining_count);
        }
        return_value = 14 + 254 + min_16bit_result;
    }
    else
    {
        for (auto &counter_ref : counter_16bit_refs)
        {
            counter_ref.get() = 65535;
        }
        remaining_count -= (65534 - min_16bit);
        const size_t index_32bit = hash_values[0] % this->heavyArray.size();
        this->heavyArray[index_32bit] += remaining_count;
        return_value = 14 + 254 + 65534 + this->heavyArray[index_32bit];
    }

    auto &min_kv = *std::min_element(bucket_refs.begin(), bucket_refs.end(),
                                     [](const auto &a, const auto &b)
                                     {
                                         return a.get().value < b.get().value;
                                     });

    if (min_16bit >= min_kv.get().value * this->threshold_ratio)
    {
        auto &bucket = min_kv.get();
        bucket.key = std::vector<uint8_t>(flowkey.begin(), flowkey.end());
        bucket.value = min_16bit;

        for (auto &counter_ref : counter_16bit_refs)
        {
            const uint16_t value = counter_ref.get();
            counter_ref.get() = std::min<uint16_t>(65535, value + min_kv.get().value);
        }
    }
    return return_value;
}

uint32_t FunnelSketch::query(std::span<const uint8_t> key) const
{
    auto max_layer_num = std::max(this->lightCU.size(), this->buckets.size());
    std::vector<uint64_t> hash_values(max_layer_num, 0);
    for (auto layer = 0; layer < max_layer_num; ++layer)
    {
        hash_values[layer] = hash(key, layer);
    }

    uint32_t result = 0;

    // query lightCU
    uint8_t min_light_value = 255;
    for (size_t layer = 0; layer < this->lightCU.size(); ++layer)
    {
        size_t index = (hash_values[layer] % (this->lightCU[layer].size() * 2));
        const bool is_low = !(index & 0x01);
        const uint8_t counter_value = this->lightCU[layer][index / 2];
        uint8_t value = is_low ? (counter_value & 0x0F) : (counter_value >> 4);
        min_light_value = std::min<uint8_t>(min_light_value, value);
    }
    if (min_light_value < 15)
    {
        return min_light_value;
    }
    result += 14;

    // query 8bit array
    uint8_t min_8bit = 255;
    for (size_t layer = 0; layer < this->buckets.size(); ++layer)
    {
        size_t index = (hash_values[layer] % this->buckets[layer].size());
        const auto &array_8bit = this->buckets[layer][index].array_8bit;
        size_t index_8bit = index % array_8bit.size();
        uint8_t value = array_8bit[index_8bit];
        min_8bit = std::min<uint8_t>(min_8bit, value);
    }
    if (min_8bit < 255)
    {
        result += min_8bit;
        return result;
    }
    result += 254;

    // query kv
    for (size_t layer = 0; layer < this->buckets.size(); ++layer)
    {
        size_t index = (hash_values[layer] % this->buckets[layer].size());
        const auto &bucket = this->buckets[layer][index];
        if (bucket.key.size() == key.size() && std::equal(bucket.key.begin(), bucket.key.end(), key.begin()))
        {
            if (bucket.value < 65535)
            {
                result += bucket.value;
                return result;
            }
            else
            {
                size_t index_32bit = hash_values[0] % this->heavyArray.size();
                result += this->heavyArray[index_32bit];
                return result;
            }
        }
    }

    // query 16bit array
    uint16_t min_16bit = 65535;
    for (size_t layer = 0; layer < this->buckets.size(); ++layer)
    {
        size_t index = (hash_values[layer] % this->buckets[layer].size());
        const auto &array_16bit = this->buckets[layer][index].array_16bit;
        size_t index_16bit = index % array_16bit.size();
        uint16_t value = array_16bit[index_16bit];
        min_16bit = std::min<uint16_t>(min_16bit, value);
    }
    if (min_16bit < 65535)
    {
        result += min_16bit;
        return result;
    }
    result += 65534;

    // query heavyArray
    size_t index_32bit = hash_values[0] % this->heavyArray.size();
    result += this->heavyArray[index_32bit];

    return result;
}
