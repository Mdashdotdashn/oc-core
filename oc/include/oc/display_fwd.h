#pragma once

// Forward-declare the concrete display type (full definition comes from platform.h).
namespace platform { class Display; }

namespace oc {
using Display = platform::Display;
} // namespace oc
