// Based on the linux implementationn of apple-gmux
// https://github.com/torvalds/linux/blob/master/drivers/platform/x86/apple-gmux.c

// Forked from
// https://github.com/timpalpant/NVDAGPUWakeHandler
// itself forked from
// https://github.com/blackgate/AMDGPUWakeHandler

// Unload kext from /Library/Extensions directory:
//    sudo kextunload /Library/Extensions/NVDAMuxControl.kext
// Build and reload kext:
//    cd <KEXT_DIR>
//    sudo chown -R root:wheel NVDAMuxControl.kext
//    sudo kextload NVDAMuxControl.kext
// Check it’s loaded:
//    kextstat | grep -i NVDAMuxControl
// Check the logs of your kernel extension over last 24h:
//    log show --last 24h --predicate 'senderImagePath contains "NVDAMuxControl"'
// Unload:
//    sudo kextunload /path/to/NVDAMuxControl.kext
// Copy to /Library/Extensions:
//    sudo cp -vR NVDAMuxControl.kext /Library/Extensions
//    sudo chown -R root:wheel /Library/Extensions/NVDAMuxControl.kext
//    sudo touch /Library/Extensions


#include <IOKit/IOLib.h>
#include "NVDAMuxControl.hpp"

#include <sys/systm.h>
#include <mach/mach_types.h>
#include <mach/kern_return.h>
#include <sys/kern_control.h>

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
// Although the Linux kernel uses a much higher value for the
// MAX (0xffff), the brightness of the screen doesn't actually
// increase past 900.
#define GMUX_MAX_BRIGHTNESS         900
// Don't allow brightness to drop to 0, because in the absence
// of hardware keys to bring it back up, one might get stuck
// with a black screen with the Mac turned on.
#define MIN_BRIGHTNESS              100

#define GMUX_SWITCH_DDC_IGD         0x1
#define GMUX_SWITCH_DISPLAY_IGD     0x2
#define GMUX_SWITCH_EXTERNAL_IGD    0x2

#define GMUX_DISCRETE_POWER_ON      0x1
#define GMUX_DISCRETE_POWER_OFF     0x0

#define BRIGHTNESS_UP               -100
#define BRIGHTNESS_DOWN             -200

// Brightness will be set to this value when kext first loaded.
static uint32_t brightness = 500;

// This required macro defines the class's constructors, destructors,
// and several other methods I/O Kit requires.
OSDefineMetaClassAndStructors(NVDAMuxControl, IOService)

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

int setBrightness(int value = brightness)
{
    if (value < MIN_BRIGHTNESS || value > GMUX_MAX_BRIGHTNESS) {
        IOLog("Invalid value for brightness: %d\n", value);
        return 1;
    }
    IOLog("Setting brightness:%d (command)\n", value);
    brightness = value;
    gmux_write32(GMUX_PORT_BRIGHTNESS, value);
    return 0;
}

int increaseBrightness()
{
    return setBrightness(brightness + 100);
}

int decreaseBrightness()
{
    return setBrightness(brightness - 100);
}

/* A simple setsockopt handler. */
static errno_t EPHandleSet( kern_ctl_ref ctlref, unsigned int unit, void *userdata, int opt, void *data, size_t len )
{
    IOLog("EPHandleSet opt is %d\n", opt);
    if (opt == BRIGHTNESS_UP) {
        return increaseBrightness();
    } else if (opt == BRIGHTNESS_DOWN) {
        return decreaseBrightness();
    }
    return setBrightness(opt);
}

/* A simple getsockopt handler. */
static errno_t EPHandleGet(kern_ctl_ref ctlref, unsigned int unit, void *userdata, int opt, void *data, size_t *len)
{
    errno_t error = EINVAL;
    IOLog("EPHandleGet opt is %d *****************\n", opt);
    return error;
}

/* A minimalist connect handler. */
static errno_t
EPHandleConnect(kern_ctl_ref ctlref, struct sockaddr_ctl *sac, void **unitinfo)
{
    IOLog("EPHandleConnect called\n");
    return (0);
}

/* A minimalist disconnect handler. */
static errno_t
EPHandleDisconnect(kern_ctl_ref ctlref, unsigned int unit, void *unitinfo)
{
    IOLog("EPHandleDisconnect called\n");
    return (0);
}

/* A minimalist write handler. */
static errno_t EPHandleWrite(kern_ctl_ref ctlref, unsigned int unit, void *userdata, mbuf_t m, int flags)
{
    IOLog("EPHandleWrite called\n");
    return (0);
}

// Define the driver's superclass.
#define super IOService

bool NVDAMuxControl::init(OSDictionary *dict)
{
    bool result = super::init(dict);
    RegisterController();
    IOLog("Initializing\n");
    return result;
}

void NVDAMuxControl::free(void)
{
    IOLog("Freeing\n");
    UnregisterController();
    super::free();
}

uint8_t get_discrete_state() {
    return gmux_read8(GMUX_PORT_DISCRETE_POWER);
}

IOService *NVDAMuxControl::probe(IOService *provider, SInt32 *score)
{
    IOService *result = super::probe(provider, score);
    IOLog("Probing\n");
    return result;
}

bool NVDAMuxControl::start(IOService *provider)
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

    bzero(&ep_ctl, sizeof(ep_ctl));  // sets ctl_unit to 0
    ep_ctl.ctl_id = 0; /* OLD STYLE: ep_ctl.ctl_id = kEPCommID; */
    ep_ctl.ctl_unit = 0;
    strcpy(ep_ctl.ctl_name, "fr.alxlive.NVDAMuxControl");
    ep_ctl.ctl_flags = CTL_FLAG_PRIVILEGED & CTL_FLAG_REG_ID_UNIT;
    ep_ctl.ctl_send = EPHandleWrite;
    ep_ctl.ctl_getopt = EPHandleGet;
    ep_ctl.ctl_setopt = EPHandleSet;
    ep_ctl.ctl_connect = EPHandleConnect;
    ep_ctl.ctl_disconnect = EPHandleDisconnect;
    errno_t error = ctl_register(&ep_ctl, &kctlref);
    if (!error) {
        IOLog("ctl_register success, ctl_id is: %d\n", ep_ctl.ctl_id);
    } else {
        IOLog("ctl_register error: %d\n", error);
    }

    return result;
}

void NVDAMuxControl::stop(IOService *provider)
{
    IOLog("Stopping\n");
    errno_t error = ctl_deregister(kctlref);
    if (!error) {
        IOLog("ctl_deregister success\n");
    } else {
        IOLog("ctl_deregister returned: %d\n", error);
    }
    PMstop();
    super::stop(provider);
}

void NVDAMuxControl::disableGPU()
{
    IOLog("Disabling GPU\n");
    gmux_write8(GMUX_PORT_INTERRUPT_ENABLE, GMUX_INTERRUPT_ENABLE);
    gmux_write8(GMUX_PORT_SWITCH_DDC, GMUX_SWITCH_DDC_IGD);
    gmux_write8(GMUX_PORT_SWITCH_DISPLAY, GMUX_SWITCH_DISPLAY_IGD);
    gmux_write8(GMUX_PORT_SWITCH_EXTERNAL, GMUX_SWITCH_EXTERNAL_IGD);
    gmux_write8(GMUX_PORT_DISCRETE_POWER, GMUX_DISCRETE_POWER_ON);
    gmux_write8(GMUX_PORT_DISCRETE_POWER, GMUX_DISCRETE_POWER_OFF);
}

IOReturn NVDAMuxControl::setPowerState ( unsigned long whichState, IOService * whatDevice )
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
