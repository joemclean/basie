#include "midi_output.hpp"
#include "daisy_core.h"
#include <algorithm>

namespace {
// DMA needs accessible SRAM and a stable buffer until its completion callback.
std::uint8_t DMA_BUFFER_MEM_SECTION transmitBuffer[32];
}

bool MidiOutput::init() {
    daisy::UartHandler::Config config;
    config.periph = daisy::UartHandler::Config::Peripheral::USART_1;
    config.pin_config.tx = daisy::Pin(daisy::PORTB, 6);
    config.pin_config.rx = daisy::Pin(daisy::PORTB, 7);
    config.mode = daisy::UartHandler::Config::Mode::TX;
    config.baudrate = 31250;
    initialized_ = uart_.Init(config) == daisy::UartHandler::Result::OK;
    failed_ = !initialized_;
    return initialized_;
}

void MidiOutput::completed(void* context, daisy::UartHandler::Result result) {
    auto& self = *static_cast<MidiOutput*>(context);
    self.transfer_.store(result == daisy::UartHandler::Result::OK ? 2 : 3, std::memory_order_release);
}

void MidiOutput::service(const MIDI::Voicing& desired, std::uint32_t revision) {
    if (!initialized_) return;
    const int status = transfer_.load(std::memory_order_acquire);
    if (status == 1) return;
    if (status == 3) {
        // A partial transfer may have sounded notes absent from our bookkeeping.
        // Channel 1 is the application's sole output channel. Reset it before retrying.
        tracker_.reset();
        panic_ = true;
        failed_ = true;
        hasSent_ = false;
    } else if (status == 2) {
        failed_ = false;
    }
    if (hasSent_ && sentRevision_ == revision && !panic_) return;
    auto packet = tracker_.transition(desired);
    std::size_t offset = 0;
    if (panic_) { // MIDI All Sound Off on channel 1, including partially sent voices.
        transmitBuffer[0] = 0xb0; transmitBuffer[1] = 120; transmitBuffer[2] = 0;
        offset = 3;
    }
    std::copy_n(packet.bytes.begin(), packet.size, transmitBuffer + offset);
    if (packet.size + offset == 0) { sentRevision_ = revision; hasSent_ = true; return; }
    transfer_.store(1, std::memory_order_release);
    if (uart_.DmaTransmit(transmitBuffer, packet.size + offset, nullptr, completed, this)
        != daisy::UartHandler::Result::OK) {
        transfer_.store(3, std::memory_order_release);
        failed_ = true;
        return;
    }
    tracker_.commit(desired);
    sentRevision_ = revision;
    hasSent_ = true;
    panic_ = false;
}
