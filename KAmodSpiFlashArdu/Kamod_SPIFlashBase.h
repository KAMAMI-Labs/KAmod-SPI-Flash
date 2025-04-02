
#ifndef Kamod_SPIFlashBase_H_
#define Kamod_SPIFlashBase_H_

#include "Arduino.h"
#include "SPI.h"

typedef struct {
  uint32_t total_size;
  uint16_t start_up_time_us;

  // Three response bytes to 0x9f JEDEC ID command.
  uint8_t manufacturer_id;
  uint8_t memory_type;
  uint8_t capacity;

  // Max clock speed for all operations and the fastest read mode.
  uint8_t max_clock_speed_mhz;

  // Bitmask for Quad Enable bit if present. 0x00 otherwise. This is for the
  // highest byte in the status register.
  uint8_t quad_enable_bit_mask;

  bool has_sector_protection : 1;

  // Supports the 0x0b fast read command with 8 dummy cycles.
  bool supports_fast_read : 1;

  // Supports the fast read, quad output command 0x6b with 8 dummy cycles.
  bool supports_qspi : 1;

  // Supports the quad input page program command 0x32. This is known as 1-1-4
  // because it only uses all four lines for data.
  bool supports_qspi_writes : 1;

  // Requires a separate command 0x31 to write to the second byte of the status
  // register. Otherwise two byte are written via 0x01.
  bool write_status_register_split : 1;

  // True when the status register is a single byte. This implies the Quad
  // Enable bit is in the first byte and the Read Status Register 2 command
  // (0x35) is unsupported.
  bool single_status_byte : 1;

  // Fram does not need/support erase and has much simpler WRITE operation
  bool is_fram : 1;

} Kamod_SPIFlash_Device_t;



enum {
  SFLASH_CMD_READ = 0x03,      // Single Read
  SFLASH_CMD_FAST_READ = 0x0B, // Fast Read
  SFLASH_CMD_QUAD_READ = 0x6B, // 1 line address, 4 line data

  SFLASH_CMD_READ_JEDEC_ID = 0x9f,

  SFLASH_CMD_PAGE_PROGRAM = 0x02,
  SFLASH_CMD_QUAD_PAGE_PROGRAM = 0x32, // 1 line address, 4 line data

  SFLASH_CMD_READ_STATUS = 0x05,
  SFLASH_CMD_READ_STATUS2 = 0x35,

  SFLASH_CMD_WRITE_STATUS = 0x01,
  SFLASH_CMD_WRITE_STATUS2 = 0x31,

  SFLASH_CMD_ENABLE_RESET = 0x66,
  SFLASH_CMD_RESET = 0x99,

  SFLASH_CMD_WRITE_ENABLE = 0x06,
  SFLASH_CMD_WRITE_DISABLE = 0x04,

  SFLASH_CMD_ERASE_PAGE = 0x81,
  SFLASH_CMD_ERASE_SECTOR = 0x20,
  SFLASH_CMD_ERASE_BLOCK = 0xD8,
  SFLASH_CMD_ERASE_CHIP = 0xC7,

  SFLASH_CMD_4_BYTE_ADDR = 0xB7,
  SFLASH_CMD_3_BYTE_ADDR = 0xE9,
};

#define KAMOD_SFLASH_RESPONSE_BUFF 6

#define KAMOD_SFLASH80_PAGE_SIZE   256UL
#define KAMOD_SFLASH80_SECTOR_SIZE  (16UL * KAMOD_SFLASH80_PAGE_SIZE)
#define KAMOD_SFLASH80_BLOCK_SIZE   (256UL * KAMOD_SFLASH80_PAGE_SIZE)

//#define SPIFLASH_DEBUG     1

class Kamod_SPIFlashBase {
private:
  SPIClass *_spi;
  uint8_t _ss;
  uint8_t _sclk;
  uint8_t _miso;
  uint8_t _mosi;

  uint32_t _clock_wr;
  uint32_t _clock_rd;

  Kamod_SPIFlash_Device_t *_flash_dev;
  uint8_t _addr_len;

public:
  uint8_t resp_buff[KAMOD_SFLASH_RESPONSE_BUFF];
  uint8_t data_buff[KAMOD_SFLASH80_PAGE_SIZE];
  
public:
  Kamod_SPIFlashBase(SPIClass *spiinterface, uint8_t ss, uint8_t sclk, uint8_t mosi, uint8_t miso);
  void setClockSpeed(uint32_t write_hz, uint32_t read_hz);
  uint8_t* buff(void);
  bool begin(Kamod_SPIFlash_Device_t *flash_dev);
  bool end(void);
  bool readCommand(uint8_t command, uint8_t *response, uint32_t len);
  bool writeCommand(uint8_t command, uint8_t *data, uint32_t len);
  bool writeCommand(uint8_t command);
  bool eraseCommand(uint8_t command, uint32_t addr);
  bool readMemory(uint32_t addr, uint8_t *data, uint32_t len);
  bool writeMemory(uint32_t addr, uint8_t *data, uint32_t len);

  uint32_t readJEDECID(void);
  uint32_t getJEDECID(void);
  void memParams(void);
  uint32_t size(void);
  uint32_t pageSize(void);
  uint32_t pages(void);
  uint32_t sectors(void);
  uint32_t blocks(void);

  uint8_t readStatus(void);
  uint8_t readStatus2(void);
  bool isReady(void);
  bool waitUntilReady(void);
  bool writeEnable(void);
  bool writeDisable(void);
  bool eraseSector(uint32_t sectorNumber);
  bool eraseBlock(uint32_t blockNumber);  
  bool eraseChip(uint32_t blockNumber);

  uint32_t readPageBuff(uint32_t address);
  uint32_t writePageBuff(uint32_t address);

private:
  void debug(uint8_t command, int32_t address, int32_t len);
  void fillAddress(uint8_t *buf, uint32_t addr);
  void beginTransaction(uint32_t clock_hz);
  void endTransaction(void);
};

#endif /* Kamod_SPIFlashBase_H_ */


