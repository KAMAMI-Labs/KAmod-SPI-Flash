#include <SPI.h>
#include "Kamod_SPIFlashBase.h"

//LED
#define LED_PIN       2 
#define MY_DELAY      2000

//SPI
#define SPI_MOSI      13
#define SPI_MISO      12
#define SPI_SCK       14
#define SPI_CS        15
#define SPI_CLOCK     4000000

//INIT
SPIClass spiFlashBus(HSPI);
Kamod_SPIFlashBase spiFlashBase(&spiFlashBus, SPI_CS, SPI_SCK, SPI_MOSI, SPI_MISO);
Kamod_SPIFlash_Device_t MY_W25Q80DV;
uint8_t *data_buff;

int ledi;
int loops;
bool retstat;
bool verify;
int result;

//-------------------------------------------
void setup() {
  Serial.begin(115200);
  //while (!Serial) {
    delay(200); // wait for native usb
  //}
  Serial.println("\r\r\rHello. KAmod SPI Flash test start");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  ledi = 0;

  retstat = false;
  while(retstat == false){
    spiFlashBus.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_CS);
    retstat = spiFlashBase.begin(&MY_W25Q80DV); 
    if (!retstat){
      Serial.println("Init fail ");
      delay(1000);
    } else {
      Serial.print("JEDEC: ");
      Serial.println(spiFlashBase.getJEDECID(), HEX);
    }
  }

  Serial.println("--- First, erase sector ---");
  spiFlashBase.eraseSector(0);
  delay(1000);
  loops = 0;
  result = 0;
  Serial.println("Flash init...OK");
  Serial.println();
}

//-------------------------------------------
void loop() {  
  Serial.print("### START at page: ");
  Serial.print(loops);
  Serial.println(" ###");

  Serial.println("--- Clear buff ---");
  data_buff = spiFlashBase.buff();
  for(int n = 0; n<KAMOD_SFLASH80_PAGE_SIZE; n++){
    *data_buff = 0;
    data_buff++;
  }

  Serial.println("--- Read mem ---");
  spiFlashBase.readPageBuff(loops * KAMOD_SFLASH80_PAGE_SIZE);
  data_buff = spiFlashBase.buff();
  for(int n = 0; n<20; n++){
    Serial.print(*data_buff);
    Serial.print(",");
    data_buff++;
  }
  Serial.println("");

  Serial.println("--- Prepare new buff ---");
  data_buff = spiFlashBase.buff();
  for(int n = 0; n<20; n++){
    *data_buff = (uint8_t)(n + (loops*10));
    Serial.print(*data_buff);
    Serial.print(",");
    data_buff++;
  }
  Serial.println("");

  Serial.println("--- Write mem ---");
  //spiFlashBase.eraseSector(0);
  spiFlashBase.writePageBuff(loops * KAMOD_SFLASH80_PAGE_SIZE);
  
  Serial.println("--- Clear buff ---");
  data_buff = spiFlashBase.buff();
  for(int n = 0; n<KAMOD_SFLASH80_PAGE_SIZE; n++){
    *data_buff = 0;
    data_buff++;
  }

  Serial.println("--- Read mem for check content ---");
  spiFlashBase.readPageBuff(loops * KAMOD_SFLASH80_PAGE_SIZE);
  data_buff = spiFlashBase.buff();
  for(int n = 0; n<20; n++){
    Serial.print(*data_buff);
    Serial.print(",");
    data_buff++;
  }
  Serial.println("");

  Serial.print("### Verify ");
  verify = true;
  data_buff = spiFlashBase.buff();
  for(int n = 0; n<20; n++){
    if (*data_buff != (uint8_t)(n + (loops*10))) verify = false;
    data_buff++;
  }
  if (verify) {
    result++;
    Serial.println("OK ###");
  }
  else Serial.println("FAIL !!! ###");

  ledi++;
  digitalWrite(LED_PIN, (ledi&1)); 
  Serial.println();
  Serial.println();
  Serial.println();

  delay(3000);
  loops++;
  if (loops >= 3) {
    if (result == 3) Serial.println("### Verify OK 3 times ###");
    else Serial.println("!!! Some things are wrong, some verifications fail !!!!");
    Serial.println("### THATS ALL ###");
    while(1){};
  }
}
