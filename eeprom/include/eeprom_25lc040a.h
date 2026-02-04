/**
 * @file eeprom_25lco4oa.h
 * @brief Driver for EEPROM 25LC040H over bit-banding SPI
 * @details Implements bit/byte/array read/write operations
 * @note 25LC040A Specification
 *          - Max. Clock 10 MHz
 *          - 512 x 8-Bit Organization
 *          - Write Page mode (up to 16 bytes)
 *          - Self-timed Erase and Write Cycles (5 ms max.)
 * 
 * @version 0.1
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

 #pragma once

 #include "spi_bitbang_driver.h"
 #include <cstdint>
 #include <cstddef>
 #include <stdexcept>

 /**
  * @brief SPI pin config
  * 
  */
 struct SpiPinConfig
 {
    uint8_t csPin;      ///< Chip Select
    uint8_t sckPin;     ///< Serial Clock
    uint8_t mosiPin;    ///< MOSI
    uint8_t misoPin;    ///< MISO

    /**
     * @brief Construct a new Spi Pin Config object
     * 
     */
    SpiPinConfig(): csPin(10), sckPin(13), mosiPin(11), misoPin(12) {}

    /**
     * @brief Constructor with custom pin configuration
     * @param cs    Chip Select
     * @param sck   Serial Clock pin
     * @param mosi  MOSI pin
     * @param miso  MISO pin
     */
    SpiPinConfig(uint8_t cs, uint8_t sck, uint8_t mosi, uint8_t miso)
        : csPin(cs), sckPin(sck), mosiPin(mosi), misoPin(miso) {}
 };

 /**
  * @brief Instruction
  * 
  */
enum class EepromCommand:uint8_t{
    READ    = 0x03,    ///< Read data from memory array beginning at selected address
    WRITE   = 0x02,    ///< Write data to memory array beginning at selected address
    WRDI    = 0x04,    ///< Reset the write enable latch
    WREN    = 0x06,    ///< Set the write enable latch
    RDSR    = 0x05,    ///< Read status register
    WRSR    = 0x01     ///< Write Status Register
};

/**
 * @brief EEPROM 25LC040H status register
 * @details Status Register from the documentation:
 *          - Bit 0: WIP  (Write In Progress) - Read only
 *          - Bit 1: WEL  (Write Enable Latch) - Read only
 *          - Bit 2: BP0  (Block Protect 0) - Writable/Readable
 *          - Bit 3: BP1  (Block Protect 1) - Writable/Readable
 *          - Bit 4-7: Unused (always read as 0)
 */
union StatusRegister
{
    uint8_t value;

    struct 
    {
        uint8_t wip     : 1; ///< Write In Progress (bit 0) - Read only
        uint8_t well    : 1; ///< Write Enable Latch (bit 1) - Read only
        uint8_t bp0     : 1; ///< Block Protect 0 (bit 2) - Writable
        uint8_t bp1     : 1; ///< Block Protect 1 (bit 3) - Writable
        uint8_t         : 4; ///< Unused bits 4-7 (always 0)
    } bits;

    /**
     * @brief Check if the recording in progress or not
     * 
     * @return true 
     * @return false 
     */
    bool isWriteInProgress() const {return bits.wip == 1; }

    /**
     * @brief Check if recording is allowed
     * 
     * @return true 
     * @return false 
     */
    bool isWriteInabled() const {return bits.well == 1; }
};

/**
 * @brief Class working with EEPROM 25lco4oa
 * 
 */
class Eeprom_25lco4oa
{
private:
    SpiBitbangDriver* m_spi;          ///< Driver
    SpiPinConfig      m_pins;         ///< Pins Configuration
    uint32_t          m_clockDelayUs; ///< Clock delay in mcr

    static constexpr uint32_t WRITE_TIME_MS = 5;     ///< Recording timeout in ms

    static constexpr size_t PAGE_SIZE = 16;          ///< Page size in bytes
    static constexpr size_t MEMORY_SIZE = 512;       ///< Memory size in bytes
    static constexpr size_t MAX_CLOCK = 10000;       ///< Max SPI frequency(2 MHz)

    /**
     * @brief Trasfer bit
     * 
     * @param bit 
     * @return true 
     * @return false 
     */
    bool spiBitTransfer (bool bit);

    /**
     * @brief Transfer Byte
     * 
     * @param byte 
     * @return uint8_t (byte)
     */
    uint8_t spiByteTransfer (uint8_t byte);

    /**
     * @brief Activate the chip (CS low)
     */
    void chipSelect();

    /**
     * @brief Disactivate the chip (CS high) 
     */
    void chipDiselect();

    /**
     * @brief Allow recording 
     * @note Conmmand WREN 
     */
    void writeEnable();

    /**
     * @brief Deny recordng
     * @note Command WRDI
     */
    void writeDisable();

    /**
     * @brief Waiting for the write operation to complete
     * 
     * @return true 
     * @return false
     * 
     * @note Waiting for the WIP bit to be reset in the status register
     */
    bool waitForWriteComplete();

    /**
     * @brief Check is Valid address
     * 
     * @param address 
     * @return true 
     * @return false 
     */
    bool isValidAddress(uint16_t address) const;

    /**
     * @brief Check is Valid bit position
     * 
     * @param bitPosition 
     * @return true 
     * @return false 
     */
    bool isValidBitPosition(uint8_t bitPosition) const;

    /**
     * @brief Check is Valid address range
     * 
     * @param startAddress 
     * @param size 
     * @return true 
     * @return false 
     */
    bool isValidAddressRange(uint16_t startAddress, size_t size) const;

public:
    /**
     * @brief Construct a new Eeprom_25lco4oa object
     * 
     * @param spiDriver 
     * @param pinConfig 
     * @param clockFrequency (defoult 100 kHz)
     * 
     * @throw std::invalid_argument if spiDriver == nullptr
     * @throw std::invalid_argument if frequency more than 10 MHz
     */
    Eeprom_25lco4oa(SpiBitbangDriver* spiDriver, 
                    const SpiPinConfig& pinConfig = SpiPinConfig(),
                    uint32_t clockFrequency = 100);

    /**
     * @brief Destructor
     * 
     */
    ~Eeprom_25lco4oa() = default;

    /**
     * @brief Read bit
     * 
     * @param address 
     * @param bitPosition 
     * @return true     (if everything is good)
     * @return false    (else)
     * @throw std::out_of_range if adress or position are not valid
     */
    bool readBit(uint16_t address, uint8_t bitPosition);

    /**
     * @brief Write bit
     * 
     * @param address 
     * @param bitPosition 
     * @param value   (byte)
     * @return true 
     * @return false 
     * @throw std::out_of_range if adress or position are not valid
     */
    bool writeBit(uint16_t address, uint8_t bitPosition, bool value);

    /**
     * @brief Read byte
     * 
     * @param address 
     * @return uint8_t (byte)
     * @throw std::out_of_range if address is not valid
     */
    uint8_t readByte(uint16_t address);

    /**
     * @brief Write byte
     * 
     * @param address 
     * @param data    (byte)
     * @return true   (if everything is good)
     * @return false  (else)
     * @throw std::out_of_range if address is not valid
     */
    bool writeByte(uint16_t address, uint8_t data);

    /**
     * @brief Read bytes array
     * 
     * @param startAddress 
     * @param buffer    (data reception (allocated))
     * @param size      (bytes count for reading)
     * @throw std::invalid_argument if buffer == nullptr
     * @throw std::out_of_range if address of size are not valid
     */
    void readBuffer(uint16_t startAddress, uint8_t* buffer, size_t size);

    /**
     * @brief Write bytes array
     * 
     * @param startAddress 
     * @param data 
     * @param size 
     * @return true 
     * @return false 
     * @throw std::invalid_argument if data == nullptr
     * @throw std::out_of_range if address of size are not valid
     * @note automatically splits the record into 16-byte pages
     */
    bool writeBuffer(uint16_t startAddress, const uint8_t* data, size_t size);

    
    /**
     * @brief Read the status register
     * 
     * @return StatusRegister 
     */
    StatusRegister readStatus();

    /**
     * @brief Get the Memory Size object
     * 
     * @return constexpr size_t (512)
     */
    static constexpr size_t getMemorySize() {return MEMORY_SIZE;}

    /**
     * @brief Get the Page Size object
     * 
     * @return constexpr size_t  (16)
     */
    static constexpr size_t getPageSize() {return PAGE_SIZE;}

    /**
     * @brief Get the Max Write Time object
     * 
     * @return constexpr uint32_t (5)
     */
    static constexpr uint32_t getMaxWriteTime() {return WRITE_TIME_MS;}

    /**
     * @brief Get the Max Clock Frequency object
     * 
     * @return constexpr uint32_t  (10000)
     */
    static constexpr uint32_t getMaxClockFrequency(){return MAX_CLOCK; }

    /**
     * @brief Checking the EEPROM ready
     * 
     * @return true (if WIP == 0)
     * @return false 
     */
    bool isReady();
};