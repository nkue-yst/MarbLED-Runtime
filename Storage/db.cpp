//
// Created by ura on 7/3/23.
//

#include <cstring>
#include <vector>
#include "db.h"


db::db(const char *dbp) {
    strcpy(db_path, dbp);
}

int db::open(){
    int ret = sqlite3_open(db_path, &db_s);
    if(ret != SQLITE_OK){
        std::cerr << "db open error" << std::endl;
        return -1;
    }
    return 0;
}

void db::close(){
    sqlite3_close(db_s);
}


/**
 * initialize the database
 * if database does not exist, generate new db
 * @return
 */
int db::init(){

    open();

    int ret = 0;
    // create controller table
    {
        bool c = is_table_exist("controller");
        if (!c) {
            const char *sql =
                    "CREATE TABLE controller(id INTEGER UNIQUE PRIMARY KEY AUTOINCREMENT, serial TEXT UNIQUE, version INTEGER, type INTEGER);";
            ret = exec_sql(sql);
            if (ret != 0) return -1;
        }
    }

    // create board table
    {
        bool c = is_table_exist("board");
        if (!c) {
            const char *sql =
                    "CREATE TABLE board(id INTEGER UNIQUE PRIMARY KEY AUTOINCREMENT, parent INTEGER, chain_num INTEGER, sensors INTEGER, modes INTEGER);";
            ret = exec_sql(sql);
            if (ret != 0) return -1;
        }
    }

    // create cal_data table
    {
        bool c = is_table_exist("cal_data");
        if (!c) {
            const char *sql =
                    "CREATE TABLE cal_data(id INTEGER, serial TEXT, version INTEGER, chain INTEGER, type INTEGER);";
            ret = exec_sql(sql);
            if (ret != 0) return -1;
        }
    }

    // create layout table
    {
        bool c = is_table_exist("layout");
        if (!c) {
            const char *sql =
                    "CREATE TABLE layout(id INTEGER, serial TEXT, version INTEGER, chain INTEGER, type INTEGER);";
            ret = exec_sql(sql);
            if (ret != 0) return -1;
        }
    }

    close();

    return 0;
}

/**
 * A callback function called by another function
 * @param param
 * @param col_cnt
 * @param row_txt
 * @param col_name
 * @return
 */
int db::callback_count(void* param, int col_cnt, char** row_txt, char** col_name){
    *(int*)param = atoi(row_txt[0]);
    return 0;
}


/**
 * execute sql
 * @param db    db instance
 * @param sql   sql
 * @return
 */
int db::exec_sql(const char* sql){
    char *err = nullptr;

    int ret = sqlite3_exec(db_s, sql, nullptr, nullptr, &err);
    if(ret != SQLITE_OK) {
        std::cerr << "sql exec error : " << err << std::endl;
        sqlite3_close(db_s);
        sqlite3_free(err);
        return -1;
    }

    return 0;
}

/**
 * check if the table exists
 * @param db
 * @param table
 * @return
 */
int db::is_table_exist(const char* table){

    char sql[256];
    char* err;
    int count;

    snprintf(sql, 256, "select count(*) from sqlite_master  where type='table' and name='%s';", table);
    int ret = sqlite3_exec(db_s, sql, callback_count, (void*)(&count), &err);
    if(ret != SQLITE_OK) {
        std::cerr << "sql exec error : " << err << std::endl;
        sqlite3_close(db_s);
        sqlite3_free(err);
        return false;
    }

    return count;
}

int db::get_controller_id(const char *serial){
    int id;

    char sql[256];
    char* err;
    snprintf(sql, 256, "SELECT id FROM controller  WHERE serial=\"%s\";", serial);
    int ret = sqlite3_exec(db_s, sql, callback_count, (void*)(&id), &err);
    if(ret != SQLITE_OK){
        std::cerr << "sql error occur : " << err << std::endl;
        sqlite3_close(db_s);
        sqlite3_free(err);
        return -1;
    }
    return id;
}


/**
 * inserts a connected board into the database if it is not registered.
 * @param brd board meta-data
 * @return result process
 */
int db::add_board(board *brd){

    open();

    int count;
    // check exist controller record
    {
        char sql[256];
        char* err;
        snprintf(sql, 256, "SELECT count(*) FROM controller  WHERE serial=\"%s\";", brd->serial);
        int ret = sqlite3_exec(db_s, sql, callback_count, (void*)(&count), &err);
        if(ret != SQLITE_OK){
            std::cerr << "sql error occur : " << err << std::endl;
            close();
            sqlite3_free(err);
            return -1;
        }
    }

    if(count == 0){
        // if controller does not register
        char sql[256];
        snprintf(sql, 256, "INSERT INTO controller(serial, version, type) VALUES(\"%s\", %d, %d);", brd->serial, brd->version, 1);
        int ret = exec_sql(sql);
        if(ret != 0) return -1;
    }

    int id = get_controller_id(brd->serial);
    if(id == -1)return -1;

    for(int i = 0; i < brd->chain; i++){
        // if controller does not register
        char sql[256];
        snprintf(sql, 256, "INSERT INTO board(parent, sensors, modes) VALUES(%d, %d, %d);", id, brd->sensors, brd->modes);
        int ret = exec_sql(sql);
        if(ret != 0) return -1;
    }

    close();
    return count;
}

int db::callback_board(void* param, int col_cnt, char** row_txt, char** col_name){
    *(std::vector<board>*)param;
    char *i = row_txt[0];
    return 0;
}

int db::get_controller(const char *serial, std::vector<board> *brds) {
    {
        char sql[256];
        char* err;
        snprintf(sql, 256, "SELECT * FROM controller  WHERE serial=\"%s\";", serial);
        int ret = sqlite3_exec(db_s, sql, callback_count, (void*)(brds), &err);
        if(ret != SQLITE_OK){
            std::cerr << "sql error occur : " << err << std::endl;
            close();
            sqlite3_free(err);
            return -1;
        }
    }
    return 0;
}
