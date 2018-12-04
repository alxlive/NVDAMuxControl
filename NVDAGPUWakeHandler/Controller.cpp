//
//  Controller.cpp
//  NVDAGPUWakeHandler
//
//

#include <IOKit/IOLib.h>
#include <string.h>
#include <sys/errno.h>
#include <libkern/libkern.h>
#include <sys/kern_control.h>
#include <sys/malloc.h>

static const int kCommandSetBrightness = 1;
static const char* kCtlName = "github.com.TankTheFrank.nvdagpuhandler";

int setBrightness(int);

/* A simple setsockopt handler */
errno_t EPHandleSet(kern_ctl_ref ctlref, unsigned int unit, void *userdata, int opt, void *data, size_t len)
{
    int error = EINVAL;
    IOLog("EPHandleSet opt is %d data len:%ld\n", opt, len);
    
    if (opt == kCommandSetBrightness && len == sizeof(uint32_t))
        error = setBrightness(*(uint32_t*)data);

    return error;
}

/* A simple A simple getsockopt handler */
errno_t EPHandleGet(kern_ctl_ref ctlref, unsigned int unit, void *userdata, int opt, void *data, size_t *len)
{
    int    error = EINVAL;
    IOLog("EPHandleGet opt is %d\n", opt);
    return error;
}

/* A minimalist connect handler */
errno_t
EPHandleConnect(kern_ctl_ref ctlref, struct sockaddr_ctl *sac, void **unitinfo)
{
    IOLog("EPHandleConnect called\n");
    return (0);
}

/* A minimalist disconnect handler */
errno_t
EPHandleDisconnect(kern_ctl_ref ctlref, unsigned int unit, void *unitinfo)
{
    IOLog("EPHandleDisconnect called\n");
    return 0;
}

/* A minimalist write handler */
errno_t EPHandleWrite(kern_ctl_ref ctlref, unsigned int unit, void *userdata, mbuf_t m, int flags)
{
    IOLog("EPHandleWrite called:\n");
    return (0);
}

kern_ctl_ref s_kctlref;
void RegisterController() {
    errno_t error;
    kern_ctl_reg     ep_ctl; // Initialize control
    bzero(&ep_ctl, sizeof(ep_ctl));  // sets ctl_unit to 0
    ep_ctl.ctl_id = 0; /* OLD STYLE: ep_ctl.ctl_id = kEPCommID; */
    ep_ctl.ctl_unit = 0;
    strncpy(ep_ctl.ctl_name, kCtlName, MAX_KCTL_NAME);
    ep_ctl.ctl_flags = CTL_FLAG_PRIVILEGED & CTL_FLAG_REG_ID_UNIT;
    ep_ctl.ctl_send = EPHandleWrite;
    ep_ctl.ctl_getopt = EPHandleGet;
    ep_ctl.ctl_setopt = EPHandleSet;
    ep_ctl.ctl_connect = EPHandleConnect;
    ep_ctl.ctl_disconnect = EPHandleDisconnect;
    error = ctl_register(&ep_ctl, &s_kctlref);
    if(error)
        IOLog("Error registering the controller: %d\n", error);
    else
        IOLog("Controller registered\n");
}

void UnregisterController() {
    ctl_deregister(&s_kctlref);
    IOLog("Controller deregistered\n");
}
