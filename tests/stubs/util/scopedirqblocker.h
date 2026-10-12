#pragma once
namespace daisy {
struct ScopedIrqBlocker { ScopedIrqBlocker() {} ~ScopedIrqBlocker() {} };
}
