/* add your code here */
#include <IOKit/IOService.h>
#include <sys/kern_control.h>

class NVDAMuxControl : public IOService
{
    OSDeclareDefaultStructors(NVDAMuxControl)
public:
    virtual bool init(OSDictionary *dictionary = 0);
    virtual void free(void);
    virtual IOService *probe(IOService *provider, SInt32 *score);
    virtual bool start(IOService *provider);
    virtual void stop(IOService *provider);
    virtual IOReturn setPowerState(unsigned long whichState, IOService * whatDevice);
private:
    virtual void disableGPU();

    struct kern_ctl_reg ep_ctl;
    kern_ctl_ref kctlref;
};
