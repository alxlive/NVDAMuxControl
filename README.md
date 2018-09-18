#  NVDAGPUWakeHandler

This kernel extension disables the NVIDIA GPU after waking up from sleep. It is intended to be used on a 2012 MacBook Pro (retina) with a failed NVIDIA GPU.

It is adapted from [AMDGPUWakeHandler](https://github.com/blackgate/AMDGPUWakeHandler) (for 2011 MacBook Pros with AMD GPUs).

**NOTE**: To use this extension, you will also want to disable the GPU at boot time using the Grub config:

```
set timeout=10
menuentry "macOS" {
  outb 0x7c2 1
  outb 0x7d4 0x28
  outb 0x7c2 2
  outb 0x7d4 0x10
  outb 0x7c2 2
  outb 0x7d4 0x40

  outb 0x7c2 1
  outb 0x7d4 0x50
  outb 0x7c2 0
  outb 0x7d4 0x50
}
```

See [this gist](https://gist.github.com/blackgate/17ac402e35d2f7e0f1c9708db3dc7a44) for more info on how to set up booting from Grub.

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
