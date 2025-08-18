#pragma once
#include <stdint.h>
#include <stddef.h>

class IFpmStream {
public:
    virtual ~IFpmStream() = default;

    // Virtuali "core"
    virtual int    available() = 0;
    virtual size_t read(uint8_t* buf, size_t len, uint32_t timeout_ms) = 0;
    virtual size_t write(const uint8_t* data, size_t len) = 0;
    virtual void   flush() = 0;

    // Convenienze per compatibilità con codice stile Arduino
    // NON virtuali: delegano alle virtuali core
    size_t write(uint8_t b) {
        return write(&b, 1);
    }

    // Arduino::Stream::readBytes ha un timeout interno; qui di default usiamo 0 (non bloccante)
    size_t readBytes(uint8_t* buf, size_t len, uint32_t timeout_ms = 0) {
        return read(buf, len, timeout_ms);
    }
    
    // Legge un byte e lo restituisce; se non disponibile, blocca fino a timeout
    int read() {
        if (available() <= 0) return -1;
        uint8_t b = 0;
        size_t r = read(&b, 1, 0);
        return (r == 1) ? static_cast<int>(b) : -1;
    }
    
};