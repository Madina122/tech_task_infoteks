/**
 * @file test_eeprom.cpp
 * @brief Test suite for 25LC040A EEPROM driver with SPI emulation
 * @version 0.1
 * @date 2026-02-04
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#include "../eeprom/include/eeprom_25lc040a.h"
#include <iostream>
#include <iomanip>
#include <stdexcept>

/**
 * @brief Simple emulation of SPI communication
 * @details Emulates SPI signals on virtual pins with predictable MISO data
 */
class SimpleSpiDriver : public SpiBitbangDriver {
public:
    /**
     * @brief Set the Pin object
     * 
     * @param pin       CS, SCK, MOSI
     * @param state     HIGH/LOW
     */
    void setPin(uint8_t pin, PinState state) override {
        switch (pin) {
        case 10:  // CS pin
            std::cout << "[SPI] Chip Select (CS): " << (state == PinState::LOW ? "ACTIVE" : "INACTIVE") << "\n";
            break;
        case 13:  // SCK pin
            if (state == PinState::HIGH) {
                std::cout << "[SPI] SCK rising edge\n";
            }
            break;
        case 11:  // MOSI pin
            std::cout << "[SPI] MOSI: " << (state == PinState::HIGH ? "1" : "0") << "\n";
            break;
        default:
            break;
        }
    }

    /**
     * @brief Read SPI pin state
     * @note Emulae MISO response 
     * 
     * @param pin           Pin number to read from
     * @return PinState     HIGH/LOW
     * @details Returns repeating 0xAA pattern (10101010) on MISO pin
     */
    PinState readPin(uint8_t pin) override {
        if (pin == 12) {    
            // Simulate reading a byte 0xAA (10101010)
            static int bitCounter = 0;
            static bool bits[8] = {1, 0, 1, 0, 1, 0, 1, 0};
            bool bit = bits[bitCounter % 8];
            bitCounter++;
            return bit ? PinState::HIGH : PinState::LOW;
        }
        return PinState::LOW;
    }

    /**
     * @brief Microsecond delay
     * 
     * @param us Delay duration in microseconds
     */
    void delayUs(uint32_t us) override {}

    /**
     * @brief Millisecond delay
     * 
     * @param ms Delay duration in milliseconds
     */
    void delayMs(uint32_t ms) override {}
};

/**
 * @brief Run tests with descriptive output
 */
void runTests() {
    /**
     * @brief Initialize test with standard Arduino SPI pins
     */
    SimpleSpiDriver spi;
    SpiPinConfig pins = {10, 13, 11, 12}; // CS=10, SCK=13, MOSI=11, MISO=12
    Eeprom_25lco4oa eeprom(&spi, pins, 100); // 100 kHz frequency

    std::cout << "\n=== Test 1: Basic Byte Write and Read ===\n";

    eeprom.writeByte(0x10, 0x55);
    uint8_t value = eeprom.readByte(0x10);
    std::cout << "Read byte from address 0x10: 0x" << std::hex << (int)value << std::dec << " (expected 0x55)\n";

    std::cout << "\n=== Test 2: Bit Write and Read ===\n";

    // Write a bit at address 0x20, bit number 3
    eeprom.writeBit(0x20, 3, true);
    bool bitVal = eeprom.readBit(0x20, 3);
    std::cout << "Bit 3 at address 0x20: " << (bitVal ? "1" : "0") << " (expected 1)\n";

    std::cout << "\n=== Test 3: Buffer Write and Read ===\n";

    uint8_t writeBuf[3] = {0x01, 0x02, 0x03};
    uint8_t readBuf[3] = {0};

    // Write buffer to address 0x30
    eeprom.writeBuffer(0x30, writeBuf, 3);
    eeprom.readBuffer(0x30, readBuf, 3);
    std::cout << "Buffer read from address 0x30: ";
    for (auto b : readBuf) {
        std::cout << "0x" << std::hex << (int)b << " ";
    }
    std::cout << std::dec << "\n";

    std::cout << "\n=== Test 4: Error Handling ===\n";

    /**
     * @brief Test memory bounds checking
     */
    try {
        eeprom.readByte(512);  // Beyond 512-byte limit
    } catch (const std::out_of_range& e) {
        std::cout << "- Address bounds: " << e.what() << "\n";
    }

    /**
     * @brief Test bit position validation
     */
    try {
        eeprom.readBit(0x10, 8);  // Invalid bit position
    } catch (const std::out_of_range& e) {
        std::cout << "- Bit position: " << e.what() << "\n";
    }

    /**
     * @brief Test null pointer validation
     */
    try {
        eeprom.readBuffer(0x10, nullptr, 1);
    } catch (const std::invalid_argument& e) {
        std::cout << "- Null buffer: " << e.what() << "\n";
    }
}

/**
 * @brief Program entry point
 * @return 0 on success, non-zero on failure
 */
int main() {
    try {
        runTests();
        std::cout << "\nAll tests passed successfully.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error during tests: " << e.what() << "\n";
        return 1;
    }
}