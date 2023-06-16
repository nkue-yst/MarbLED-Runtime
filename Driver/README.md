# MarbLED-Board Driver

## note
works on linux only.

## dependencies

- [libudev](https://www.freedesktop.org/software/systemd/man/libudev.html)
- [libzmq](https://github.com/zeromq/libzmq)
- [cppzmq](https://github.com/zeromq/cppzmq)

## options
  
| option | description                        | example        |
|--------|------------------------------------|----------------|
| -t     | number of operating modes of board | 5              |
| -p     | device name                        | /dev/ttyACM0   |
| -b     | bind address                       | 127.0.0.1:6001 |