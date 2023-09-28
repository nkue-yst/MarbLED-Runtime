//
// Created by ura on 7/3/23.
//

#ifndef TOUCHMATRIX_STORAGE_DB_H
#define TOUCHMATRIX_STORAGE_DB_H

#include <string>
#include <unistd.h>
#include <iostream>

#include <sqlite3.h>


struct board{
    char serial[256];
    uint16_t id;
    uint16_t controller_id;
    uint8_t version;
    uint8_t chain_num;
    uint8_t modes;
    int32_t layout_x;
    int32_t layout_y;
};


class db {
private:
    sqlite3 *db_s = nullptr;
    char db_path[256]{};

    int exec_sql(const char* sql);
    int is_table_exist(const char* table);
    int open();
    void close();

    static int callback_count(void* param, int col_cnt, char** row_txt, char** col_name);
    static int callback_board(void* param, int col_cnt, char** row_txt, char** col_name);

public:
    db(const char *dbp);
    int init();

    int is_controller_exist(const char *serial);
    int is_board_exist(unsigned int cid, unsigned int chain_num);

    int add_controller(const char *serial, uint16_t sw_ver, uint16_t type);
    int add_board(board *brd);

    unsigned int get_controller_id(const char *serial);
    unsigned int get_board_id(unsigned int cid, unsigned int chain_num);

    int get_board(unsigned int bid, board *brd);
    int update_layout(unsigned int bid, int x, int y);

};


#endif //TOUCHMATRIX_STORAGE_DB_H
