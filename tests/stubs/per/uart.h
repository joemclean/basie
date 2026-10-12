#pragma once
#include "daisy_core.h"
#include <cstddef>
#include <cstdint>
#include <vector>
namespace daisy {
class UartHandler {
  public:
    enum class Result { OK, ERR };
    struct Config {
        enum class Peripheral { USART_1 };
        enum class Mode { TX };
        Peripheral periph{};
        Mode mode{};
        struct { Pin tx{PORTB, 6}; Pin rx{PORTB, 7}; } pin_config;
        unsigned baudrate = 31250;
    };
    using StartCallbackFunctionPtr = void (*)(void*);
    using EndCallbackFunctionPtr = void (*)(void*, Result);
    inline static std::vector<std::vector<std::uint8_t>> transmissions;
    inline static EndCallbackFunctionPtr completion = nullptr;
    inline static void* context = nullptr;
    inline static bool reject = false;
    inline static bool initFailure = false;
    Result Init(const Config&) { return initFailure ? Result::ERR : Result::OK; }
    Result DmaTransmit(std::uint8_t* bytes, std::size_t size, StartCallbackFunctionPtr, EndCallbackFunctionPtr done, void* owner) {
        if (reject) return Result::ERR;
        transmissions.emplace_back(bytes, bytes + size);
        completion = done; context = owner;
        return Result::OK;
    }
    static void finish(Result result = Result::OK) {
        auto callback = completion;
        completion = nullptr;
        if (callback) callback(context, result);
    }
};
}
