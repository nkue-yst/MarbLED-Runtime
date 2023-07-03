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
    uint8_t version;
    uint8_t chain;
    uint8_t chain_num;
    uint8_t sensors;
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
    int get_controller_id(const char *serial);
    int open();
    void close();

    static int callback_count(void* param, int col_cnt, char** row_txt, char** col_name);
    static int callback_board(void* param, int col_cnt, char** row_txt, char** col_name);

public:
    db(const char *dbp);
    int init();
    int add_board(board *brd);
    int get_board(const char *serial, std::vector<board> *brds);

};


#endif //TOUCHMATRIX_STORAGE_DB_H
