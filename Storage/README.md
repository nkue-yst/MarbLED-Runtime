# MarbLED Storage

## API

### Request Board-Ids  
  
Request (multipart message)
```
STORAGE REQ_BRDIDS
[SERIAL] [CHAIN_NUM] [BOARD_VER] [MODES]
```
Reply
```
STORAGE REPLY
[BOARD_ID]
```


### Request Layouts
  
Request
```
STORAGE REQ_LAYOUT
```
Reply (multipart message)
```
STORAGE REPLY [CANBAS AREA x] [CANBAS AREA y]
[BOARD ID] [BOARD VER] [MATRIX RESOLUTION x] [MATRIX RESOLUTION y] [x] [y] [IS_ENABLE]
...
```  
  
### Store Layouts  
  
Request (multipart message)
```
STORAGE STR_LAYOUT
[BOARD ID] [x] [y] [IS_ENABLE]
...
```
Reply
```
STORAGE REPLY [EXEC_CODE]
```


## Table Definition

### Tables  

| no  | name         | description      |
|-----|--------------|------------------|
| 1   | controller   | controller       |
| 2   | board        | board            |
| 3   | layout       | layout           |
| 4   | board_master | known board data |
  
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
| integer | chain_num     | chain_num                  |
| integer | version       | sensing board version      |
| integer | modes         | sensing modes              |

### Layout
  
| type    | name     | description  |
|---------|----------|--------------|
| integer | id       | layout id    |
| integer | board_id | board id     |
| integer | bx       | board x pos  |
| integer | by       | board y pos  |
| integer | group    | layout group |

### Board Master

| type    | name         | description                    |
|---------|--------------|--------------------------------|
| integer | id           | bm id                          |
| integer | board_ver    | board version                  |
| integer | sensors      | sensor quantity                |
| integer | matrix_res_x | led matrix resolution (width)  |
| integer | matrix_res_y | led matrix resolution (height) |
