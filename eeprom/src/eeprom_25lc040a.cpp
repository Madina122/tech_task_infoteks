/**
 * @file eeprom_25lc040a.cpp
 * @brief EEPROM_25LC040A driver realization
 * @version 0.1
 * @date 2026-02-04
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "..\include\eeprom_25lc040a.h"
 
#include <stdexcept>
#include <cstring>

/**
 * @brief Construct a new Eeprom_25lco4oa::Eeprom_25lco4oa object
 * 
 * @param spiDriver         Pointer to SPI bit-banging driver instance
 * @param pinConfig         (CS, SCK, MOSI, MISO)
 * @param clockFrequency    (max 2000 kHz)
 *  @throw std::invalid_argument if spiDriver is null, clockFrequency invalid
 */
Eeprom_25lco4oa::Eeprom_25lco4oa(SpiBitbangDriver* spiDriver,
                                const SpiPinConfig& pinConfig,
                                uint32_t clockFrequency)
    : m_spi(spiDriver), m_pins(pinConfig){

    /// Validate GPIO driver
    if(!m_spi){throw std::invalid_argument("SPI driver cannot be null");}
    
    /// Validate clock frequency (max 2MHz for safe bit-banging)
    if(clockFrequency > MAX_CLOCK){throw std::invalid_argument("Clock frequency exceeds maximum 10 MHz");}
    if(clockFrequency == 0){throw std::invalid_argument("Clock frequency cannot be zero");}

    /// Calculate clock delay
    m_clockDelayUs = 500/clockFrequency;
    
    /// Initialize SPI pins
    m_spi->setPin(m_pins.csPin, PinState::HIGH);    // CS inactive
    m_spi->setPin(m_pins.sckPin, PinState::LOW);    // Sck Low
    m_spi->setPin(m_pins.mosiPin, PinState::LOW);   // MOSI low
}

/**
 * @brief Transfer single bit over SPI (full duplex)
 * 
 * @param bit Bit to transmit (true=HIGH, false=LOW)
 * @return Received bit from MISO pin
 */
bool Eeprom_25lco4oa::spiBitTransfer (bool bit){
    /// Setup MOSI pin with data
    m_spi->setPin(m_pins.mosiPin, bit ? PinState::HIGH : PinState::LOW);

    if(m_clockDelayUs > 0){ m_spi->delayUs(m_clockDelayUs);}

    /// Clock high
    m_spi->setPin(m_pins.sckPin, PinState::HIGH);

    /// Sample MISO pin state
    bool received = (m_spi->readPin(m_pins.misoPin) == PinState::HIGH);

    if(m_clockDelayUs > 0){ m_spi->delayUs(m_clockDelayUs); }

    /// Clock low
    m_spi->setPin(m_pins.sckPin, PinState::LOW);

    return received;
}

/**
 * @brief Transfer single byte over SPI (MSB first)
 * 
 * @param byte data to transmit
 * @return uint8_t Received byte 
 */
uint8_t Eeprom_25lco4oa::spiByteTransfer (uint8_t byte){
    uint8_t received = 0;

    /// MSB first
    for(int8_t i = 7; i >= 0; i--){
        bool bitToSend = (byte >> i) & 0x01;
        bool receivedBit = spiBitTransfer(bitToSend);

        if(receivedBit){ received |= (1 << i); }
    }
    return received;
}

/**
 * @brief Activate EEPROM chip 
 * 
 */
void Eeprom_25lco4oa::chipSelect(){
    m_spi->setPin(m_pins.csPin, PinState::LOW);

    m_spi->delayUs(1); /// CS setup time
}

/**
 * @brief Deactivate EEPROM chip 
 * 
 */
void Eeprom_25lco4oa::chipDiselect(){
    m_spi->delayUs(1); /// CS hold time
    m_spi->setPin(m_pins.csPin, PinState::HIGH);

    m_spi->delayUs(1); /// next command delay
}

/**
 * @brief Enable write operations 
 * 
 */
void Eeprom_25lco4oa::writeEnable(){
    chipSelect();
    spiByteTransfer(static_cast<uint8_t>(EepromCommand::WREN));
    chipDiselect();

    m_spi->delayUs(10); /// WREN delay
}

/**
 * @brief Disable write operations
 * 
 */
void Eeprom_25lco4oa::writeDisable(){
    chipSelect();
    spiByteTransfer(static_cast<uint8_t>(EepromCommand::WRDI));
    chipDiselect();

    m_spi->delayUs(10); /// WRDI delay
}

/**
 * @brief Wait for internal write cycle completion
 * 
 * @return true    (if write completed within timeout)
 * @return false   (else)
 */
bool Eeprom_25lco4oa::waitForWriteComplete(){
    /// Timeout = 2x maximum write cycle
    const uint32_t timeoutMs = WRITE_TIME_MS * 2;

    for(uint32_t i = 0; i < timeoutMs; ++i){
        StatusRegister status = readStatus();

        if(!status.bits.wip){ return true; }

        m_spi->delayMs(1);
    }
    return false; /// Timeout
}

/**
 * @brief Validate single address
 * 
 * @param address Address to validate (0-511)
 * @return true   (if address within memory bounds)
 * @return false  (else)
 */
bool Eeprom_25lco4oa::isValidAddress(uint16_t address) const{ 
    return address < MEMORY_SIZE; 
}

/**
 * @brief Validate bit position within byte
 * 
 * @param bitPosition  Bit position (0-7)
 * @return true        (if position valid)
 * @return false       (else)
 */
bool Eeprom_25lco4oa::isValidBitPosition(uint8_t bitPosition) const{
    return bitPosition < 8;
}

/**
 * @brief Validate address range for buffer operations
 * 
 * @param startAddress 
 * @param size          number of bytes
 * @return true         (if entire range fits in memory)
 * @return false        (else)
 */
bool Eeprom_25lco4oa::isValidAddressRange(uint16_t startAddress, size_t size) const{
    if(size == 0) { return false; }

    if(startAddress > MEMORY_SIZE - size){ return false; }

    return true;
}

/**
 * @brief Read single bit using Read-Modify-Write pattern
 * 
 * @param address           Byte address (0-511)
 * @param bitPosition       Bit position (0=LSB, 7=MSB)
 * @return Bit value (true=1, false=0)
 * @throw std::out_of_range if address or bit position invalid
 */
bool Eeprom_25lco4oa::readBit(uint16_t address, uint8_t bitPosition){
    if(!isValidAddress(address)){ throw std::out_of_range("EEPROM address out of range (0-511)");}

    if(!isValidBitPosition(bitPosition)){ throw std::out_of_range("Bit position must be 0-7"); }

    uint8_t byteValue = readByte(address);
    return(byteValue >> bitPosition) & 0x01;
}

/**
 * @brief Write single bit using Read-Modify-Write cycle
 * 
 * @param address           Byte address (0-511)
 * @param bitPosition       Bit position (0-7)
 * @param value             Bit value to write
 * @return true if write successful
 * @throw std::out_of_range if address or bit position invalid
 */
bool Eeprom_25lco4oa::writeBit(uint16_t address, uint8_t bitPosition, bool value){
    if(!isValidAddress(address)){ throw std::out_of_range("EEPROM address out of range (0-511)");}

    if(!isValidBitPosition(bitPosition)){ throw std::out_of_range("Bit position must be 0-7"); }

    uint8_t currentByte = readByte(address);

    if(value){ currentByte |= (1 << bitPosition); 
    } else { currentByte &= ~(1 << bitPosition); }

    return writeByte(address, currentByte);
}

/**
 * @brief Read single byte from EEPROM
 * 
 * @param address   Address (0-511)
 * @return uint8_t  Byte value
 * @throw std::out_of_range if address invalid
 */
uint8_t Eeprom_25lco4oa::readByte(uint16_t address){
    if(!isValidAddress(address)){ throw std::out_of_range("EEPROM address out of range (0-511)");}

    chipSelect();

    spiByteTransfer(static_cast<uint8_t>(EepromCommand::READ));

    spiByteTransfer(address >> 8);          /// Address MSB
    spiByteTransfer(address & 0xFF);        /// Address LSB

    uint8_t data = spiByteTransfer(0x00);   /// Dummy byte, receive data
    chipDiselect();

    return data;
}

/**
 * @brief Write single byte to EEPROM
 * 
 * @param address   Address (0-511)
 * @param data      Byte to write
 * @return true if write successful
 * @throw std::out_of_range if address invalid
 */
bool Eeprom_25lco4oa::writeByte(uint16_t address, uint8_t data){
    if(!isValidAddress(address)){ throw std::out_of_range("EEPROM address out of range (0-511)");}

    writeEnable();
    chipSelect();

    spiByteTransfer(static_cast<uint8_t>(EepromCommand::WRITE));

    spiByteTransfer(address >> 8);      /// Address MSB
    spiByteTransfer(address & 0xFF);    /// Address LSB
    spiByteTransfer(data);              /// Data byte
    chipDiselect();

    bool success = waitForWriteComplete();

    writeDisable();

    return success;
}

/**
 * @brief Read multiple bytes into buffer
 * 
 * @param startAddress  Start address (0-511)
 * @param buffer        Output buffer
 * @param size          Number of bytes to read
 * @throw std::out_of_range if address range exceeds memory
 * @throw std::invalid_argument if buffer is null
 */
void Eeprom_25lco4oa::readBuffer(uint16_t startAddress, uint8_t* buffer, size_t size){
    if(!buffer){ throw std::invalid_argument("Buffer pointer cannot be null");}
    if(size == 0) {return;}
    if(!isValidAddressRange(startAddress, size)){
        throw std::out_of_range("Address range out of bounds (0-511)");
    }
    chipSelect();

    spiByteTransfer(static_cast<uint8_t>(EepromCommand::READ));

    spiByteTransfer(startAddress >> 8);     /// Address MSB
    spiByteTransfer(startAddress & 0xFF);   /// Address LSB

    /// Read sequential bytes
    for(size_t i = 0; i < size; ++i){
        buffer[i] = spiByteTransfer(0x00);
    }
    chipDiselect();
}

/**
 * @brief Write multiple bytes respecting page boundaries
 * 
 * @param startAddress          Start address (0-511)
 * @param data                  Input data buffer
 * @param size                  Number of bytes to write
 * @return true if all bytes written successfully
 * @throw std::out_of_range if address range exceeds memory
 * @throw std::invalid_argument if data is null
 */
bool Eeprom_25lco4oa::writeBuffer(uint16_t startAddress, const uint8_t* data, size_t size){
    if(!data){
        throw std::invalid_argument("Data pointer cannot be null");
    }

    if(size == 0) {return true;}

    if(!isValidAddressRange(startAddress, size)){
        throw std::out_of_range("Address range out of bounds (0-511)");
    }

    size_t bytesWritten = 0;
    uint16_t currentAddress = startAddress;

    /// Page by page 
    while(bytesWritten < size){
        /// Page boundaries
        uint16_t pageStart = currentAddress & ~(PAGE_SIZE-1);
        uint16_t offsetPage = currentAddress - pageStart;
        size_t bytesInthisPage = PAGE_SIZE - offsetPage;
        
        /// Determine bytes to write in this page
        size_t bytesToWrite = (size - bytesWritten < bytesInthisPage) ? (size - bytesWritten) : bytesInthisPage;

        /// Single page write
        writeEnable();
        chipSelect();

        spiByteTransfer(static_cast<uint8_t>(EepromCommand::WRITE));

        spiByteTransfer(currentAddress >> 8);
        spiByteTransfer(currentAddress & 0xFF);

        for(size_t i = 0; i < bytesToWrite; ++i){
            spiByteTransfer(data[bytesWritten + i]);
        }

        chipDiselect();

        /// Wait for page write completion
        if(!waitForWriteComplete()){
            writeDisable();
            return false;
        }
        writeDisable();

        /// Advance to next page
        currentAddress += bytesToWrite;
        bytesWritten += bytesToWrite;
    }
    return true;
}

/**
 * @brief Read EEPROM status register
 * 
 * @return StatusRegister 
 */
StatusRegister Eeprom_25lco4oa::readStatus(){
    chipSelect();

    spiByteTransfer(static_cast<uint8_t>(EepromCommand::RDSR));

    StatusRegister status;
    status.value = spiByteTransfer(0x00);   /// Dummy byte receive status

    chipDiselect();
    return status;
}

/**
 * @brief Check if EEPROM is ready for new operations
 * 
 * @return true  (if WIP bit is clear)
 * @return false (else)
 */
bool Eeprom_25lco4oa::isReady() {
    StatusRegister status = readStatus();
    return !status.bits.wip;
}