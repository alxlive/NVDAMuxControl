import signal
from functools import partial

from pynput import keyboard
import Quartz

import build.KernelSocketLib as KernelSocketLib


SIGNUM_TO_SIGNAME = {signal_type.value: signal_type.name
                     for signal_type in signal.Signals}
# Full list of codes from pynput codebase:
#    https://github.com/moses-palmer/pynput/blob/master/lib/pynput/keyboard/_darwin.py#L174C5-L175C31
F1_KEY = 0x7A
F2_KEY = 0x78


def darwin_intercept(event_type, event):
    vk = Quartz.CGEventGetIntegerValueField(
            event, Quartz.kCGKeyboardEventKeycode)
    if vk == F1_KEY or vk == F2_KEY:
        # Suppress keys F1 and F2, don't propagate them to the rest of the
        # system.
        return None
    # Let all other keys propagate normally.
    return event


class BrightnessControl:

  def __init__(self):
    self.socket = KernelSocketLib.KernelSocket()
    # Stop cleanly on CTRL+C.
    signal.signal(signal.SIGINT, partial(self.stop_listener))
    # Stop cleanly on KILL.
    # This throws an error, I don't know why.
    # signal.signal(signal.SIGKILL, partial(self.stop_listener))
    # In any case, SIGKILL, SIGTERM, SIGABRT should all be synonyms.
    signal.signal(signal.SIGTERM, partial(self.stop_listener))
    signal.signal(signal.SIGABRT, partial(self.stop_listener))

  def start(self):
    self.socket.connect()
    # Collect events until released.
    self.listener = keyboard.Listener(
            on_press=self.on_press,
            on_release=self.on_release,
            # We cannot use suppress=True, because that suppresses all
            # keyboard events system-wide. Can't type anything anywhere!
            suppress=False,
            # Instead, we use a platform-specific filtering function to stop
            # propagation of F1/F2 only.
            # Pynput docs:
            #    https://pynput.readthedocs.io/en/latest/faq.html#macos
            darwin_intercept=darwin_intercept)
    # Start listening in a non-blocking fashion.
    self.listener.start()
    # Block until the listener has stopped.
    self.listener.join()
    # We've stopped listening for events, so close the socket.
    self.socket.close()

  def stop_listener(self, signum, unused_frame):
    signal_name = SIGNUM_TO_SIGNAME[signum]
    print(f'Received {signal_name}, stopping the listener.')
    self.listener.stop()

  def on_press(self, key):
    if key == keyboard.Key.f1:
      print('Decreasing brightness')
      self.socket.decreaseBrightness()
    elif key == keyboard.Key.f2:
      print('Increasing brightness')
      self.socket.increaseBrightness()

  def on_release(self, key):
    if key == keyboard.Key.f1 or key == keyboard.Key.f2:
      print(f'{key} released')


def Main():
  brightness_control = BrightnessControl()
  brightness_control.start()
  return 0


if __name__ == '__main__':
  Main()
