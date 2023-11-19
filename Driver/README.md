# MarbLED-Board Driver

## note
works on linux only.

## dependencies

- [libzmq](https://github.com/zeromq/libzmq)
- [cppzmq](https://github.com/zeromq/cppzmq)

## options
  
| option | description                    | example              |
|--------|--------------------------------|----------------------|
| -p     | controller ip address          | 192.168.0.100        |
| -b     | sensor-data publish address    | tcp://127.0.0.1:6001 |
| -i     | storage node address           | tcp://127.0.0.1:7000 |
| -c     | color-data subscribing address | tcp://127.0.0.1:4000 |
| -h     | board chain count              | 2                    |
| -v     | board version                  | 4                    |            

## zmq message format

### Sensor Data
```angular2html
BRD_DATA [Data Num] [Sensor-Data...]
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