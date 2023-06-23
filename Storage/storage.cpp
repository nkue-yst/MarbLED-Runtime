//
// Created by ura on 6/21/23.
//
#include <string>
#include <unistd.h>
#include <iostream>

#include <zmq.hpp>
#include <zmq_addon.hpp>

#include <sqlite3.h>
#include <future>


char* db_path = nullptr;

struct board{
    char *serial;
    uint8_t version;
    uint8_t chain;
    uint8_t sensors;
    uint8_t modes;
};

std::string db_tables[] = {
        "controller",
        "board",
        "cal_data",
        "layout"
};


/**
 * A callback function called by another function
 * @param param
 * @param col_cnt
 * @param row_txt
 * @param col_name
 * @return
 */
int callback_count(void* param, int col_cnt, char** row_txt, char** col_name){
    *(int*)param = atoi(row_txt[0]);
    return 0;
}

/**
 * check if the table exists
 * @param db
 * @param table
 * @return
 */
int is_table_exist(sqlite3 *db, const char* table){

    char sql[256];
    char* err;
    int count;

    snprintf(sql, 256, "select count(*) from sqlite_master  where type='table' and name='%s';", table);
    int ret = sqlite3_exec(db, sql, callback_count, (void*)(&count), &err);
    if(ret != SQLITE_OK) {
        std::cerr << "sql exec error : " << err << std::endl;
        sqlite3_close(db);
        sqlite3_free(err);
        return false;
    }

    return count;
}

/**
 * execute sql
 * @param db    db instance
 * @param sql   sql
 * @return
 */
int exec_sql(sqlite3 *db, const char* sql){
    char *err = nullptr;

    int ret = sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if(ret != SQLITE_OK) {
        std::cerr << "sql exec error : " << err << std::endl;
        sqlite3_close(db);
        sqlite3_free(err);
        return -1;
    }

    return 0;
}

/**
 * initialize the database
 * if database does not exist, generate new db
 * @return
 */
int init_db(){

    sqlite3 *db = nullptr;

    int ret = sqlite3_open(db_path, &db);
    if(ret != SQLITE_OK){
        std::cerr << "db open error" << std::endl;
        return -1;
    }

    // create controller table
    {
        bool c = is_table_exist(db, "controller");
        if (!c) {
            const char *sql =
                    "CREATE TABLE controller(id INTEGER PRIMARY KEY AUTOINCREMENT, serial TEXT, version INTEGER, type INTEGER);";
            ret = exec_sql(db, sql);
            if (ret != 0) return -1;
        }
    }

    // create board table
    {
        bool c = is_table_exist(db, "board");
        if (!c) {
            const char *sql =
                    "CREATE TABLE board(id INTEGER, serial TEXT, version INTEGER, chain INTEGER, type INTEGER);";
            ret = exec_sql(db, sql);
            if (ret != 0) return -1;
        }
    }

    // create cal_data table
    {
        bool c = is_table_exist(db, "cal_data");
        if (!c) {
            const char *sql =
                    "CREATE TABLE cal_data(id INTEGER, serial TEXT, version INTEGER, chain INTEGER, type INTEGER);";
            ret = exec_sql(db, sql);
            if (ret != 0) return -1;
        }
    }

    // create layout table
    {
        bool c = is_table_exist(db, "layout");
        if (!c) {
            const char *sql =
                    "CREATE TABLE layout(id INTEGER, serial TEXT, version INTEGER, chain INTEGER, type INTEGER);";
            ret = exec_sql(db, sql);
            if (ret != 0) return -1;
        }
    }

    sqlite3_close(db);
    return 0;
}

/**
 * inserts a connected board into the database if it is not registered.
 * @param brd board meta-data
 * @return result process
 */
int add_board(board *brd){

    sqlite3 *db = nullptr;
    int ret = sqlite3_open(db_path, &db);
    if(ret != SQLITE_OK){
        std::cerr << "db open error" << std::endl;
        return -1;
    }

    int count;
    {
        char sql[256];
        char* err;
        snprintf(sql, 256, "SELECT count(*) FROM controller  WHERE serial=%s;", brd->serial);
        int ret = sqlite3_exec(db, sql, callback_count, (void*)(&count), &err);
        if(ret != SQLITE_OK){
            std::cerr << "sql error occur" << err << std::endl;
            sqlite3_close(db);
            sqlite3_free(err);
            return -1;
        }
    }

    if(count == 0){
        // if controller does not register

        char sql[256];
        snprintf(sql, 256, "INSERT INTO controller VALUES(%s, %d, %d);", brd->serial, brd->version, 1);
        int ret = exec_sql(db, sql);
        if(ret != 0) return -1;
    }

    sqlite3_close(db);
    return count;
}

void receive_meta(const char* addr){

    std::vector<std::string> serial_cache;

    zmq::context_t ctx(1);
    zmq::socket_t subscriber(ctx, zmq::socket_type::sub);
    subscriber.bind(addr);

    subscriber.set(zmq::sockopt::subscribe, "BRD_INFO");
    std::vector<zmq::message_t> recv_msgs;


    while(1){
        recv_msgs.clear();

        board brd = {};
        int mode;

        zmq::recv_multipart(subscriber, std::back_inserter(recv_msgs));
        if(recv_msgs.empty()) {
            std::cout << "timeout" << std::endl;
            continue;
        }

        sscanf(recv_msgs.at(0).data<char>(),
                "BRD_DATA %s %d ",
                brd.serial,
                &brd.version);

        //
        //std::cout << brd_serial << " : [" << mode << "] [" << recv_msgs.at(1).data<uint16_t>()[30] << "] "<< std::endl;

    }
}


void run(const char *addr){
    int ret = init_db();
    if(ret < 0)return;

    std::future<void> recv_meta = std::async(std::launch::async, receive_meta, addr);
    recv_meta.wait();
}

int main(int argc, char* argv[]){

    int c;
    const char* optstring = "d:b:";

    // enable error log from getopt
    opterr = 0;

    // bind address
    char* bind_addr = nullptr;

    // get options
    while((c = getopt(argc, argv, optstring)) != -1){
        if(c == 'd') {
            db_path = optarg;
        }else if(c == 'b'){
            bind_addr = optarg;
        }else{
            // parse error
            printf("unknown argument error\n");
            return -1;
        }
    }

    // check for serial port
    if(db_path == nullptr){
        printf("require data-base path");
        return -1;
    }
    // check for bind address
    if(bind_addr == nullptr){
        printf("require bind address option");
        return -2;
    }

    run(bind_addr);

    return 0;
}
