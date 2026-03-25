#ifndef VIRTUAL_HYPERLOGLOG_H
#define VIRTUAL_HYPERLOGLOG_H

#include <span>
#include <vector>

class VHLL
{
public:
    /// @brief Constructor
    /// @param counter_num vHLL总长度
    /// @param hll_size vHLL中一个逻辑HLL的计数器数量，逻辑HLL的计数器固定5位大小
    VHLL(size_t counter_num, size_t hll_size);

public:
    void insert(std::span<const uint8_t> key, std::span<const uint8_t> element);
    size_t estimate(std::span<const uint8_t> key) const;
    void reset();

private:
    double estimate_global() const;

private:
    size_t counter_num;
    size_t hll_size;
    const size_t counter_size = 5;

private:
    mutable bool updated;
    mutable int flag;
    size_t counter_bits;
    double hll_correction_factor;
    double global_correction_factor;
    mutable double global_estimate;

private:
    std::vector<uint8_t> counters;
};

#endif // VIRTUAL_HYPERLOGLOG_H