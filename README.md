# NVDAMuxControl

This is a fork of [NVDAGPUWakeHandler](https://github.com/timpalpant/NVDAGPUWakeHandler).

Some people report successfully restoring brightness control with
[NativeDisplayBrightness](https://github.com/TankTheFrank/NativeDisplayBrightness)
after installing [NVDAGPUWakeHandler](https://github.com/timpalpant/NVDAGPUWakeHandler). For me this didn't work. This is why I created this fork of NVDAGPUWakeHandler, which allows communication with the kernel extension to set the brightness directly in the gMux. This is also what the Linux kernel does.

**NOTE**: To use this extension, you must disable the GPU at boot time using the Grub config. As far as I know, you'll get a kernel panic otherwise.

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
  exit
}
```

See [this gist](https://gist.github.com/blackgate/17ac402e35d2f7e0f1c9708db3dc7a44) for more info on how to set up booting from Grub.

## Build with Xcode

To build the kext with Xcode you must disable SIP. Running recovery mode may be difficult with the broken NVIDIA GPU, you could try to create a macOS usb stick and run it to disable SIP.

To build the kext make sure the OSBundle versions in the Info.plist file match the kernel version of your system.

To check your kernel version run from terminal:

```
system_profiler SPSoftwareDataType
```


## Installation

Build with Xcode:

* Open project in Xcode
* Select Product > Archive
* In the window that opens up, select the latest build and click Distribute
  Content.
* Select "Built products" and click Next, choose a destination, and you're done.

After building using Xcode, copy the kext to `/Library/Extensions`, update its permissions, then touch /Library/Extensions to force the kext cache to be rebuilt:

```
sudo cp -vR <EXPORT DIR>/Products/System/Library/Extensions/NVDAMuxControl.kext /Library/Extensions/
sudo chown -R root:wheel /Library/Extensions/NVDAMuxControl.kext
sudo touch /Library/Extensions
```

Next, compile the client:

```
cd NVDAMuxControl/client
python3 -m venv venv
. venv/bin/activate
python3 -m pip install -r requirements.txt
make
cp -vR dist/brightness.app /Applications/
```

Now add the client app to start automatically at login:

* Settings > Users & Groups > (your account) > Login Items tab
* Drag and drop the app there from your Applications directory

Finally, give the app permissions to listen for F1/F2 keypresses:

* Settings > Security & Privacy > Input Monitoring
* Drag and drop the app there from your Applications directory, and make sure
  its checkbox is checked.

Reboot.

Check that the kext is running:

```
kextstat | grep NVDAMuxControl
```

Press Fn+F1/F2 to test if the brightness goes up and down.

## Loading the kext manually

If you prefer to load the kext manually, then after building the kext, copy it to a location of your choice and run the following command to change its permissions:

```
sudo chown -R root:wheel /path/to/NVDAMuxControl.kext
```

Then when you want to load the kext you can simply run the command:

```
sudo kextload /path/to/NVDAMuxControl.kext
```

To unload:

```
sudo kextunload /path/to/NVDAMuxControl.kext
```

## View logs

To view the logs for the last 24 hours run the following on the terminal:
```
log show --last 24h --predicate 'senderImagePath contains "NVDAMuxControl"'
```
