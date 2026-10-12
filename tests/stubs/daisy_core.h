#pragma once
#include <cstdint>
#define DMA_BUFFER_MEM_SECTION
namespace daisy {
enum Port { PORTB };
struct Pin { Pin(Port, int) {} };
}
