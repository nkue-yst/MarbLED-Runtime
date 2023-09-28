# MarbLED-Board Driver

## note
works on linux only.

## dependencies

- [libudev](https://www.freedesktop.org/software/systemd/man/libudev.html)
- [libzmq](https://github.com/zeromq/libzmq)
- [cppzmq](https://github.com/zeromq/cppzmq)

## options
  
| option | description                 | example        |
|--------|-----------------------------|----------------|
| -t     | number of operating modes   | 5              |
| -p     | device name                 | /dev/ttyACM0   |
| -b     | sensor-data publish address | 127.0.0.1:6001 |
| -i     | meta-data publish address   | 127.0.0.1:7000 |        

## zmq message format

### Meta Data
```angular2html
BRD_INFO [Serial-Number] [version] [chain] [sensors] [modes]
```

### Sensor Data
```angular2html
BRD_DATA [Serial-Number] [chain] [chain-num] [Mode] [Sensor-Data...]
```  
  
Sensor-Data section sent as multipart message

### Usage  

| data          | description                                |
|---------------|--------------------------------------------|
| Serial Number | Serial Number obtained from usb descriptor |
| chain         | number of concatenated boards              |
| sensors       | number of sensors mounted on board         |
| modes         | number of operating modes                  |
| chain-num     | board number (smaller than chain)          |
| mode          | operating mode                             |
| sensor data   | array of sensor data (often 16bit integer) |