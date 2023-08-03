# MarbLED Storage

## Table Definition

### Tables  

| no | name         | description      |
|----|--------------|------------------|
| 1  | controller   | controller       |
| 2  | board        | board            |
| 3  | layout       | layout           |
| 4  | cal_data     | calibration data |
| 5  | board_master | known board data |
  
### Controller  

| type    | name    | description              |
|---------|---------|--------------------------|
| integer | id      | controller id            |
| text    | serial  | serial num from usb desc |
| integer | version | sw ver                   |
| integer | type    | connection type          |

### Board

| type    | name          | description                |
|---------|---------------|----------------------------|
| integer | id            | board id ( not serial no.) |
| integer | controller_id | connected controller id    |
| integer | board_ver     | sensing board version      |
| integer | modes         | sensing modes              |

### Layout
  
| type    | name     | description  |
|---------|----------|--------------|
| integer | id       | layout id    |
| integer | board_id | board id     |
| integer | bx       | board x pos  |
| integer | by       | board y pos  |
| integer | group    | layout group |

### Calibrated Data

| type    | name      | description   |
|---------|-----------|---------------|
| integer | id        | cal_data id   |
| integer | board_id  | board id      |
| text    | data_path | cal_data path |

### Board Master

| type    | name         | description                    |
|---------|--------------|--------------------------------|
| integer | id           | bm id                          |
| integer | board_ver    | board version                  |
| integer | sensors      | sensor quantity                |
| integer | matrix_res_x | led matrix resolution (width)  |
| integer | matrix_res_y | led matrix resolution (height) |
