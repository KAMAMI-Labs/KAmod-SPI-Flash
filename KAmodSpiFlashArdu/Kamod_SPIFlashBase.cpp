
#include "Kamod_SPIFlashBase.h"

Kamod_SPIFlashBase::Kamod_SPIFlashBase(SPIClass *spiinterface, uint8_t ss, uint8_t sclk, uint8_t mosi, uint8_t miso) {
  _spi = spiinterface;
  _ss = ss;
  _sclk = sclk;
  _mosi = mosi;
  _miso = miso;
  _clock_wr = _clock_rd = 4000000;
  _flash_dev = NULL;
  _addr_len = 3;
}

//----------------------------------------------
void Kamod_SPIFlashBase::setClockSpeed(uint32_t write_hz, uint32_t read_hz) {
  _clock_wr = write_hz;
  _clock_rd = read_hz;
}

//----------------------------------------------
bool Kamod_SPIFlashBase::begin(Kamod_SPIFlash_Device_t *flash_dev) {
  _flash_dev = flash_dev;
  if (_flash_dev == NULL) return false;

  _flash_dev->total_size = 1UL * 1024 * 1024;  //1MB default
  _flash_dev->start_up_time_us = 5000;
  _flash_dev->manufacturer_id = 0xef;
  _flash_dev->memory_type = 0x40;
  _flash_dev->capacity = 0x14;
  _flash_dev->max_clock_speed_mhz = 10;
  _flash_dev->quad_enable_bit_mask = 0x00;
  _flash_dev->has_sector_protection = false;
  _flash_dev->supports_fast_read = false;
  _flash_dev->supports_qspi = false;
  _flash_dev->supports_qspi_writes = false;
  _flash_dev->write_status_register_split = false;
  _flash_dev->single_status_byte = false;
  _flash_dev->is_fram = false;

  _addr_len = 3;
  //_clock_wr = _clock_rd = (_flash_dev->max_clock_speed_mhz * 1000000UL);

  if (_spi == NULL) return false;

  pinMode(_ss, OUTPUT);
  digitalWrite(_ss, HIGH);
  _spi->begin(_sclk, _miso, _mosi, _ss);

  readJEDECID();
  memParams();
  return true;
}

//----------------------------------------------
bool Kamod_SPIFlashBase::end(void) {
  if (_spi == NULL) {
    return false;
  }
  _spi->end();
  pinMode(_ss, INPUT);
  return true;
}

//----------------------------------------------
void Kamod_SPIFlashBase::debug(uint8_t command, int32_t address, int32_t len) {
  #if SPIFLASH_DEBUG
    Serial.print("SFLASH COMMAND: ");
    Serial.print(command, HEX);
    if (address >= 0) {                                           
      Serial.print(", ADDR: ");                                              
      Serial.print(address);                                               
    }
    if (len >= 0) {                                                              
      Serial.print(", LEN:");                                               
      Serial.print(len);                                                    
    }                                                                          
    Serial.println();  
  #endif
}

//----------------------------------------------
void Kamod_SPIFlashBase::fillAddress(uint8_t *buf, uint32_t addr) {
  switch (_addr_len) {
    case 4:
      *buf++ = (addr >> 24) & 0xFF;
    case 3:
      *buf++ = (addr >> 16) & 0xFF;
    case 2:
    default:
      *buf++ = (addr >> 8) & 0xFF;
      *buf++ = addr & 0xFF;
  }
}

//----------------------------------------------
uint8_t* Kamod_SPIFlashBase::buff(void) {
  return data_buff;
}

//----------------------------------------------
void Kamod_SPIFlashBase::beginTransaction(uint32_t clock_hz) {
  _spi->beginTransaction(SPISettings(clock_hz, MSBFIRST, SPI_MODE0));
  digitalWrite(_ss, LOW);
}

//----------------------------------------------
void Kamod_SPIFlashBase::endTransaction(void) {
  digitalWrite(_ss, HIGH);
  _spi->endTransaction();
} 

//----------------------------------------------
bool Kamod_SPIFlashBase::readCommand(uint8_t command, uint8_t *response, uint32_t len) {
  debug(command, -1, len);
  beginTransaction(_clock_rd);
  _spi->transfer(command);
  while (len--) {
    *response = _spi->transfer(0xFF);
    response++;
  }
  endTransaction();
  return true;
}

//----------------------------------------------
bool Kamod_SPIFlashBase::writeCommand(uint8_t command, uint8_t *data, uint32_t len) {
  debug(command, -1, len);
  beginTransaction(_clock_wr);
  _spi->transfer(command);
  while (len--) {
    (void)_spi->transfer(*data);
    data++;
  }
  endTransaction();
  return true;
}

//----------------------------------------------
bool Kamod_SPIFlashBase::writeCommand(uint8_t command) {
  debug(command, -1, -1);
  beginTransaction(_clock_wr);
  _spi->transfer(command);
  endTransaction();
  return true;
}

//----------------------------------------------
bool Kamod_SPIFlashBase::eraseCommand(uint8_t command, uint32_t addr) {
  debug(command, addr, -1);
  uint8_t cmd_with_addr[5] = {command};
  beginTransaction(_clock_wr);
  fillAddress(cmd_with_addr + 1, addr);
  _spi->transfer(cmd_with_addr, _addr_len + 1);
  endTransaction();
  return true;
}

//----------------------------------------------
bool Kamod_SPIFlashBase::readMemory(uint32_t addr, uint8_t *data, uint32_t len) {
  uint8_t cmd_with_addr[6];
  beginTransaction(_clock_rd);
  fillAddress(cmd_with_addr + 1, addr);

  if (_flash_dev->supports_fast_read){
    cmd_with_addr[0] = SFLASH_CMD_FAST_READ;
    _spi->transfer(cmd_with_addr, _addr_len + 2);
    _spi->transfer(data, len);
    endTransaction();
    debug(SFLASH_CMD_FAST_READ, addr, len);
  } else {
    cmd_with_addr[0] = SFLASH_CMD_READ;
    _spi->transfer(cmd_with_addr, _addr_len + 1);
    _spi->transfer(data, len);
    endTransaction();
    debug(SFLASH_CMD_READ, addr, len);
  }
  return true;
}

//----------------------------------------------
bool Kamod_SPIFlashBase::writeMemory(uint32_t addr, uint8_t *data, uint32_t len) {
  debug(SFLASH_CMD_PAGE_PROGRAM, addr, len);
  uint8_t cmd_with_addr[5] = {SFLASH_CMD_PAGE_PROGRAM};
  beginTransaction(_clock_wr);
  fillAddress(cmd_with_addr + 1, addr);
  _spi->transfer(cmd_with_addr, _addr_len + 1);
  while (len--) {
    _spi->transfer(*data);
    data++;
  }
  endTransaction();
  return true;
}

//----------------------------------------------
uint32_t Kamod_SPIFlashBase::getJEDECID(void) {
  return (((uint32_t)_flash_dev->manufacturer_id) << 16) |
           (_flash_dev->memory_type << 8) | _flash_dev->capacity;
}

//----------------------------------------------
uint32_t Kamod_SPIFlashBase::readJEDECID(void) {
  readCommand(SFLASH_CMD_READ_JEDEC_ID, resp_buff, 3);
  _flash_dev->manufacturer_id = resp_buff[0];
  _flash_dev->memory_type = resp_buff[1];
  _flash_dev->capacity = resp_buff[2];
  debug(_flash_dev->manufacturer_id, _flash_dev->memory_type, _flash_dev->capacity);
  return getJEDECID();
}

//----------------------------------------------
void Kamod_SPIFlashBase::memParams(void) {
  if ((_flash_dev->memory_type == 0x40) and (_flash_dev->capacity == 0x14))
    _flash_dev->total_size = 1UL * 1024 * 1024;
  else if ((_flash_dev->memory_type == 0x40) and (_flash_dev->capacity == 0x18))
    _flash_dev->total_size = 16UL * 1024 * 1024;
  else if ((_flash_dev->memory_type == 0x70) and (_flash_dev->capacity == 0x18))
    _flash_dev->total_size = 16UL * 1024 * 1024;
  else 
    _flash_dev->total_size = 0;

  if (_flash_dev->total_size > 16UL * 1024 * 1024) {
    _addr_len = 4;
    // Enable 4-Byte address mode (This has to be done after the reset above)
    //_trans->runCommand(SFLASH_CMD_4_BYTE_ADDR);
  } else if (_flash_dev->total_size > 64UL * 1024) {
    _addr_len = 3;
  } else {
    _addr_len = 2;
  }
}

//----------------------------------------------
uint32_t Kamod_SPIFlashBase::size(void) {
  return ((uint32_t)_flash_dev->total_size);
}
//----------------------------------------------
uint32_t Kamod_SPIFlashBase::pageSize(void) {
  return ((uint32_t)KAMOD_SFLASH80_PAGE_SIZE);
}
//----------------------------------------------
uint32_t Kamod_SPIFlashBase::pages(void) {
  return ((uint32_t)(_flash_dev->total_size / KAMOD_SFLASH80_PAGE_SIZE));
}
uint32_t Kamod_SPIFlashBase::sectors(void) {
  return ((uint32_t)(_flash_dev->total_size / KAMOD_SFLASH80_SECTOR_SIZE));
}
uint32_t Kamod_SPIFlashBase::blocks(void) {
  return ((uint32_t)(_flash_dev->total_size / KAMOD_SFLASH80_BLOCK_SIZE));
}

//----------------------------------------------
uint8_t Kamod_SPIFlashBase::readStatus(void) {
  uint8_t status;
  readCommand(SFLASH_CMD_READ_STATUS, &status, 1);
  return status;
}

//----------------------------------------------
uint8_t Kamod_SPIFlashBase::readStatus2(void) {
  uint8_t status;
  readCommand(SFLASH_CMD_READ_STATUS2, &status, 1);
  return status;
}

//----------------------------------------------
bool Kamod_SPIFlashBase::isReady(void) {
  if ((readStatus() & 0x03) == 0) return true;
  return false;
}

//----------------------------------------------
bool Kamod_SPIFlashBase::waitUntilReady(void) {
  volatile uint32_t i = 0;
  uint8_t status = 0xFF;

  while (status & 0x03){
    readCommand(SFLASH_CMD_READ_STATUS, &status, 1);
    i++;
    if (i > 1000000) return false;
  }
  return true;
}

//----------------------------------------------
bool Kamod_SPIFlashBase::writeEnable(void) {
  return writeCommand(SFLASH_CMD_WRITE_ENABLE);
}

//----------------------------------------------
bool Kamod_SPIFlashBase::writeDisable(void) {
  return writeCommand(SFLASH_CMD_WRITE_DISABLE);
}

//----------------------------------------------
bool Kamod_SPIFlashBase::eraseSector(uint32_t sectorNumber) {
  if (!_flash_dev) return false;

  if (waitUntilReady()) {
    writeEnable();
    eraseCommand(SFLASH_CMD_ERASE_SECTOR, sectorNumber * KAMOD_SFLASH80_SECTOR_SIZE);
    return true;
  }
  return false;
}

//----------------------------------------------
bool Kamod_SPIFlashBase::eraseBlock(uint32_t blockNumber) {
  if (!_flash_dev) return false;

  if (waitUntilReady()) {
    writeEnable();
    eraseCommand(SFLASH_CMD_ERASE_BLOCK, blockNumber * KAMOD_SFLASH80_BLOCK_SIZE);
    return true;
  }
  return false;
}

//----------------------------------------------
bool Kamod_SPIFlashBase::eraseChip(uint32_t blockNumber) {
  if (!_flash_dev) return false;

  if (waitUntilReady()) {
    writeEnable();
    writeCommand(SFLASH_CMD_ERASE_CHIP);
    return true;
  }
  return false;
}

//----------------------------------------------
//return num of page
uint32_t Kamod_SPIFlashBase::readPageBuff(uint32_t address) {
  address &= ~(KAMOD_SFLASH80_PAGE_SIZE-1);
  if (waitUntilReady()) {
    readMemory(address, data_buff, KAMOD_SFLASH80_PAGE_SIZE);
  }
  return address;
}

//----------------------------------------------
uint32_t Kamod_SPIFlashBase::writePageBuff(uint32_t address) {
  if (waitUntilReady()) {
    writeEnable();
    address &= ~(KAMOD_SFLASH80_PAGE_SIZE-1);
    writeMemory(address, data_buff, KAMOD_SFLASH80_PAGE_SIZE);
    writeDisable();
  }
  return address;
}