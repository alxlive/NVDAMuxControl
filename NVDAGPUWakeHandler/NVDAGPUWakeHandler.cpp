// Based on the linux implementationn of apple-gmux
// https://github.com/torvalds/linux/blob/master/drivers/platform/x86/apple-gmux.c
// and adapted for Macbook Pro 10,1 (retina mid-2012) from
// https://github.com/blackgate/AMDGPUWakeHandler

#include <IOKit/IOLib.h>
#include "NVDAGPUWakeHandler.hpp"

#define kMyNumberOfStates 2
#define kIOPMPowerOff 0

#define GMUX_IOSTART                0x700
#define GMUX_INTERRUPT_ENABLE       0xff
#define GMUX_INTERRUPT_DISABLE      0x00

#define GMUX_PORT_SWITCH_DISPLAY    0x10
#define GMUX_PORT_SWITCH_DDC        0x28
#define GMUX_PORT_SWITCH_EXTERNAL   0x40
#define GMUX_PORT_DISCRETE_POWER    0x50
#define GMUX_PORT_INTERRUPT_ENABLE  0x14
#define GMUX_PORT_VALUE             0xc2
#define GMUX_PORT_READ              0xd0
#define GMUX_PORT_WRITE             0xd4

#define GMUX_PORT_MAX_BRIGHTNESS    0x70
#define GMUX_PORT_BRIGHTNESS        0x74
#define GMUX_BRIGHTNESS_MASK        0x00ffffff
#define GMUX_MAX_BRIGHTNESS         900

#define GMUX_SWITCH_DDC_IGD         0x1
#define GMUX_SWITCH_DISPLAY_IGD     0x2
#define GMUX_SWITCH_EXTERNAL_IGD    0x2

#define GMUX_DISCRETE_POWER_ON      0x1
#define GMUX_DISCRETE_POWER_OFF     0x0

static uint32_t brightess = GMUX_MAX_BRIGHTNESS;

// This required macro defines the class's constructors, destructors,
// and several other methods I/O Kit requires.
OSDefineMetaClassAndStructors(NVDAGPUWakeHandler, IOService)

static inline void outb(uint16_t port, uint8_t value)
{
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint32_t inl(uint16_t port)
{
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

int gmux_index_wait_ready()
{
    int i = 200;
    uint8_t gwr = inb(GMUX_IOSTART + GMUX_PORT_WRITE);
    
    while (i && (gwr & 0x01)) {
        inb(GMUX_IOSTART + GMUX_PORT_READ);
        gwr = inb(GMUX_IOSTART + GMUX_PORT_WRITE);
        IODelay(100);
        i--;
    }
    
    return !!i;
}

int gmux_index_wait_complete()
{
    int i = 200;
    uint8_t gwr = inb(GMUX_IOSTART + GMUX_PORT_WRITE);
    
    while (i && !(gwr & 0x01)) {
        gwr = inb(GMUX_IOSTART + GMUX_PORT_WRITE);
        IODelay(100);
        i--;
    }
    
    if (gwr & 0x01)
        inb(GMUX_IOSTART + GMUX_PORT_READ);
    
    return !!i;
}

uint8_t gmux_read8(uint16_t port)
{
    uint8_t val;
    gmux_index_wait_ready();
    outb(GMUX_IOSTART + GMUX_PORT_READ, port & 0xff);
    gmux_index_wait_complete();
    val = inb(GMUX_IOSTART + GMUX_PORT_VALUE);
    
    return val;
}

void gmux_write8(uint16_t port, uint8_t val)
{
    outb(GMUX_IOSTART + GMUX_PORT_VALUE, val);
    gmux_index_wait_ready();
    outb(GMUX_IOSTART + GMUX_PORT_WRITE, port & 0xff);
    gmux_index_wait_complete();
}

static void gmux_write32(uint16_t port, uint32_t val)
{
    int i;
    uint8_t tmpval;
    
    for (i = 0; i < 4; i++) {
        tmpval = (val >> (i * 8)) & 0xff;
        outb(GMUX_IOSTART + GMUX_PORT_VALUE + i, tmpval);
    }
    
    gmux_index_wait_ready();
    outb(GMUX_IOSTART + GMUX_PORT_WRITE, port & 0xff);
    gmux_index_wait_complete();
}

//
void RegisterController();
void UnregisterController();

int setBrightness(int value = brightess)
{
    IOLog("Setting brightness:%d (command)\n", value);
    brightess = value;
    gmux_write32(GMUX_PORT_BRIGHTNESS, value);
    return 0;
}

// Define the driver's superclass.
#define super IOService

bool NVDAGPUWakeHandler::init(OSDictionary *dict)
{
    bool result = super::init(dict);
    RegisterController();
    IOLog("Initializing\n");
    return result;
}

void NVDAGPUWakeHandler::free(void)
{
    IOLog("Freeing\n");
    UnregisterController();
    super::free();
}

uint8_t get_discrete_state() {
    return gmux_read8(GMUX_PORT_DISCRETE_POWER);
}

IOService *NVDAGPUWakeHandler::probe(IOService *provider, SInt32 *score)
{
    IOService *result = super::probe(provider, score);
    IOLog("Probing\n");
    return result;
}

bool NVDAGPUWakeHandler::start(IOService *provider)
{
    bool result = super::start(provider);
    IOLog("Starting\n");
    PMinit();
    provider->joinPMtree(this);
    static IOPMPowerState myPowerStates[kMyNumberOfStates] = {
        {1, kIOPMPowerOff, kIOPMPowerOff, kIOPMPowerOff, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, kIOPMPowerOn, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
    };

    registerPowerDriver (this, myPowerStates, kMyNumberOfStates);

    return result;
}

void NVDAGPUWakeHandler::stop(IOService *provider)
{
    IOLog("Stopping\n");
    PMstop();
    super::stop(provider);
}

void NVDAGPUWakeHandler::disableGPU()
{
    IOLog("Disabling GPU\n");
    gmux_write8(GMUX_PORT_INTERRUPT_ENABLE, GMUX_INTERRUPT_ENABLE);
    gmux_write8(GMUX_PORT_SWITCH_DDC, GMUX_SWITCH_DDC_IGD);
    gmux_write8(GMUX_PORT_SWITCH_DISPLAY, GMUX_SWITCH_DISPLAY_IGD);
    gmux_write8(GMUX_PORT_SWITCH_EXTERNAL, GMUX_SWITCH_EXTERNAL_IGD);
    gmux_write8(GMUX_PORT_DISCRETE_POWER, GMUX_DISCRETE_POWER_ON);
    gmux_write8(GMUX_PORT_DISCRETE_POWER, GMUX_DISCRETE_POWER_OFF);
}

IOReturn NVDAGPUWakeHandler::setPowerState ( unsigned long whichState, IOService * whatDevice )
{
    if ( whichState != kIOPMPowerOff ) {
        IOLog("Waking up\n");
        this->disableGPU();
        // Major hack: there must be some ordering dependency that is not
        // being respected--doing it once will not take effect.
        // So we retry a few times after wake to make sure it sticks.
        //
        // Thanks to @andywarduk in https://github.com/blackgate/AMDGPUWakeHandler/pull/1
        // for the idea.
        for(int i = 0; i < 20; i++) {
            setBrightness();
            IOSleep(500);
        }
    } else {
        IOLog("Sleeping\n");
        gmux_write8(GMUX_PORT_INTERRUPT_ENABLE, GMUX_INTERRUPT_DISABLE);
    }
    
    return kIOPMAckImplied;
}
