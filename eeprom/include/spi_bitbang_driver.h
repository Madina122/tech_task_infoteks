
/**
 * @file spi_bitbang_driver.h
 * @brief Abstract interface for bit-banding SPI driver
 * @version 0.1
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once

#include <cstdint>
#include <cstddef>

/**
 * @brief Pin state 
 */

enum class PinState : bool{
    LOW = false, ///< low level
    HIGH = true ///<  high level
};

/**
 * @brief Abstract driver interface
 */
class SpiBitbangDriver
{
public:
    virtual ~SpiBitbangDriver() = default;

    /**
     * @brief Set the Pin object
     * 
     * @param pin   Pin number
     * @param state (HIGH/LOW)
     */
    virtual void setPin(uint8_t pin, PinState state) = 0;

    /**
     * @brief Read the pin object
     * 
     * @param pin       Pin number
     * @return PinState (HIGH/LOW)
     */
    virtual PinState readPin(uint8_t pin) = 0;

    /**
     * @brief Microseconds delay
     * 
     * @param microseconds 
     */
    virtual void delayUs(uint32_t microseconds) = 0;

    /**
     * @brief Milliseconds delay
     * 
     * @param milliseconds 
     */
    virtual void delayMs(uint32_t milliseconds) = 0;
};

