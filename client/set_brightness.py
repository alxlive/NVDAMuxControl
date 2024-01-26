# Simple Python script that connects to the kernel extension, sets the
# brightness level, and disconnects.

# Usage:
#   $ python3 -m venv venv
#   $ . venv/bin/activate
#   $ python3 -m pip install -r requirements.txt
#   $ python3 set_brightness.py [100-900]
#
# Brightness 0 is not allowed because then you'd be stuck with a black screen.

import sys

import build.KernelSocketLib as KernelSocketLib


def Main():
  try:
    brightness = int(sys.argv[1])
  except:
    brightness = None
  if not brightness or brightness < 100 or brightness > 900:
    print('Usage: python3 set_brightness.py [value between 100-900]')
    exit(0)
  socket = KernelSocketLib.KernelSocket()
  if not socket.connect():
    print('Connection to kernel extension failed.')
    # Exit right away, as the connection isn't open.
    return 0
  if not socket.setBrightness(brightness):
    print('Failed to set brightness.')
    # Don't exit -- fall through and let it close the connection.
  socket.close()
  return 0


if __name__ == '__main__':
  Main()
