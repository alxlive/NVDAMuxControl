#  NVDAGPUWakeHandler

This kernel extension disables the NVIDIA GPU after waking up from sleep. It is intended to be used on a 2012 MacBook Pro (retina) with a failed NVIDIA GPU.

It is adapted from [AMDGPUWakeHandler](https://github.com/blackgate/AMDGPUWakeHandler) (for 2011 MacBook Pros with AMD GPUs).

## Installation

After building using Xcode, copy the kext to `/Library/Extensions` and run the following commands from terminal:

```
sudo chown -R root:wheel /Library/Extensions/NVDAGPUWakeHandler.kext
sudo touch /Library/Extensions
```

Reboot.

## Manual loading

If you prefer to load the kext manually, then after building the kext, copy it to a location of your choice and run the following command to change its permissions:

```
sudo chown -R root:wheel /path/to/NVDAGPUWakeHandler.kext
```

Then when you want to load the kext you can simply run the command:

```
sudo kextload /path/to/NVDAGPUWakeHandler.kext
```

To unload:

```
sudo kextunload /path/to/NVDAGPUWakeHandler.kext
```

## View logs

To view the logs for the last 24 hours run the following on the terminal:
```
log show --last 24h --predicate 'senderImagePath contains "NVDAGPUWakeHandler"'
```
