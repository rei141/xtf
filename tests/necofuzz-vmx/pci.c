#include "pci.h"
    uint16_t kConfigAddress = 0x0cf8;
    uint16_t kConfigData = 0x0cfc;
    int num_device;
    
Device devices[32];
void IoOut32(uint16_t addr, uint32_t data){
    asm volatile ("mov %0, %%dx" ::"r" (addr));
    asm volatile ("mov %0, %%eax" ::"r" (data));
    asm volatile ("out %eax, %dx");
}
uint32_t IoIn32(uint16_t addr){
    uint32_t ret;
    asm volatile("mov %0, %%dx" ::"r"(addr));
    asm volatile("in %dx, %eax");
    asm volatile("mov %%eax, %0" :"=r" (ret));
    return ret;
}
#define shl(a,b) (a << b)
  uint32_t MakeAddress(uint8_t bus, uint8_t device,
                       uint8_t function, uint8_t reg_addr) {

    return shl(1, 31)  // enable bit
        | shl(bus, 16)
        | shl(device, 11)
        | shl(function, 8)
        | (reg_addr & 0xfcu);
  }
  int AddDevice(Device device) {
    // ClassCode dam = {0,0,0};
    // Device dammy = {0,0,0,0, dam};bus, device, function, header_type
    
    if (devices[31].bus != 0 && devices[31].device != 0 && devices[31].function != 0){
        return 1;
    }
    devices[num_device] = device;
    ++num_device;
    return 0;
  }
int ScanBus(uint8_t bus);

  ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t function) {
    WriteAddress(MakeAddress(bus, device, function, 0x08));
    uint32_t reg = ReadData();
    ClassCode cc;
    cc.base       = (reg >> 24) & 0xffu;
    cc.sub        = (reg >> 16) & 0xffu;
    cc.interface  = (reg >> 8)  & 0xffu;
    return cc;
  }
  int ScanFunction(uint8_t bus, uint8_t device, uint8_t function) {
    ClassCode class_code = ReadClassCode(bus, device, function);
    uint8_t header_type = ReadHeaderType(bus, device, function);
    Device dev = {bus, device, function, header_type, class_code};
    int err =  AddDevice(dev);
    if ( err) {
      return err;
    }

    if (class_code.base == 0x6u && class_code.sub == 0x04u) {
      // standard PCI-PCI bridge
      uint32_t bus_numbers = ReadBusNumbers(bus, device, function);
      uint8_t secondary_bus = (bus_numbers >> 8) & 0xffu;
      return ScanBus(secondary_bus);
    }

    return 0;
  }
    int ScanDevice(uint8_t bus, uint8_t device) {
    int err= ScanFunction(bus, device, 0);
    if (err ) {
      return err;
    }
    if (IsSingleFunctionDevice(ReadHeaderType(bus, device, 0))) {
      return 0;
    }

    for (uint8_t function = 1; function < 8; ++function) {
      if (ReadVendorId(bus, device, function) == 0xffffu) {
        continue;
      }
      err = ScanFunction(bus, device, function);
      if (err) {
        return err;
      }
    }
    return 0;
  }

    int ScanBus(uint8_t bus) {
    for (uint8_t device = 0; device < 32; ++device) {
      if (ReadVendorId(bus, device, 0) == 0xffffu) {
        continue;
      }
    int err= ScanDevice(bus, device);

      if ( err ) {
        return err;
      }
    }
    return 0;
  }


   void WriteAddress(uint32_t address) {
    IoOut32(kConfigAddress, address);
  }
   void WriteData(uint32_t value){
    IoOut32(kConfigData, value);
  }
    uint32_t ReadData(){
    return IoIn32(kConfigData);
  }
  uint32_t ReadBar(uint8_t bus, uint8_t device, uint8_t function,uint8_t bar) {
    WriteAddress(MakeAddress(bus, device, function, 0x10+0x4*bar));
    return ReadData();
  }
  inline uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function) {
    WriteAddress(MakeAddress(bus, device, function, 0x00));
    return ReadData() & 0xffffu;
  }
  
  uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function) {
    WriteAddress(MakeAddress(bus, device, function, 0x00));
    return ReadData() >> 16;
  }
//     inline uint16_t ReadVendorId(const Device& dev) {
//     return ReadVendorId(dev.bus, dev.device, dev.function);
//   }
  uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function) {
    WriteAddress(MakeAddress(bus, device, function, 0x0c));
    return (ReadData() >> 16) & 0xffu;
  }
    uint32_t ReadConfReg(Device dev, uint8_t reg_addr) {
    WriteAddress(MakeAddress(dev.bus, dev.device, dev.function, reg_addr));
    return ReadData();
  }

  void WriteConfReg(Device dev, uint8_t reg_addr, uint32_t value) {
    WriteAddress(MakeAddress(dev.bus, dev.device, dev.function, reg_addr));
    WriteData(value);
  }

    uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function) {
    WriteAddress(MakeAddress(bus, device, function, 0x18));
    return ReadData();
  }
    int IsSingleFunctionDevice(uint8_t header_type) {
    return (header_type & 0x80u) == 0;
  }

int ScanAllBus() {
    num_device = 0;

    uint8_t header_type = ReadHeaderType(0, 0, 0);
    if (IsSingleFunctionDevice(header_type)) {
        // wprintf(L"single function\n");
        // return 100;
      return ScanBus(0);
    }

    for (uint8_t function = 0; function < 8; ++function) {
      if (ReadVendorId(0, 0, function) == 0xffffu) {
        continue;
      }
    int err= ScanBus(function);

      if ( err ) {
        return err;
      }
    }
    return 0;
  }