#pragma once
#include <cstdint>

/// Ornament & Crime Hardware Abstraction Layer
/// GPIO (Digital Gate Inputs) Interface
///
/// Tracks the 4 gate/trigger inputs. Edge detection is performed in scan()
/// (called once per audio callback) accumulating a mask of rising edges
/// seen since the last scan. The mask is cleared on each scan() call.

namespace oc::hal {

class GPIOInterface {
public:
    virtual ~GPIOInterface() = default;

    /// Snapshot all digital inputs and detect rising edges.
    /// Called once per audio callback before the user's audio_callback().
    virtual void scan() = 0;

    /// Current logic level of a gate input (true = high).
    virtual bool read_input(uint8_t channel) const = 0;

    /// Bitmask of channels that had a rising edge since the last scan().
    /// Bit N is set if channel N triggered. Cleared on the next scan() call.
    virtual uint32_t get_edge_mask() const = 0;

    static constexpr int kChannels = 4;
};

} // namespace oc::hal
