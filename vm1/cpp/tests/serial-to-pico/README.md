

# Test for sending a MessagePack message

...which will be received by [this Arduino script](https://github.com/zwodev/vm1-video-mixer/tree/master/vm1-controller/arduino/tests/20250507-msgpck-receive-and-neopixels-test).

## Dependencies

```
$ sudo apt install libmsgpack-dev
```
## Compile and Run
```
$ mkdir builddir
$ meson setup builddir

$ meson compile -C builddir
$ ./builddir/serial-test
```