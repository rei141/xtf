#pragma once
#include <xtf.h>
    

    typedef struct ClassCode {
    uint8_t base, sub, interface;}  ClassCode;
typedef struct Device {
    uint8_t bus, device, function, header_type;
    ClassCode class_code;
  } Device;
    extern int num_device;
    extern Device devices[32];
uint32_t IoIn32(uint16_t addr);
void IoOut32(uint16_t addr, uint32_t data);
//     inline uint16_t ReadVendorId(const Device& dev) {
//     return ReadVendorId(dev.bus, dev.device, dev.function);
//   }
   void WriteAddress(uint32_t address);
   void WriteData(uint32_t value);
    uint32_t ReadData(void);
  uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function);
  
  uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function);

  uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function);
    uint32_t ReadConfReg(Device dev, uint8_t reg_addr);

  void WriteConfReg(Device dev, uint8_t reg_addr, uint32_t value);
    uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function);
    int IsSingleFunctionDevice(uint8_t header_type);
int ScanAllBus(void);
  uint32_t ReadBar(uint8_t bus, uint8_t device, uint8_t function,uint8_t bar);
