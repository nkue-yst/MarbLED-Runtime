//
// Created by chihiro on 23/09/28.
//

#ifndef MARBLED_RUNTIME_UTILITY_H
#define MARBLED_RUNTIME_UTILITY_H

#include "zmq.hpp"
#include "zmq_addon.hpp"

#define MESSAGE_ERROR -2


// Storageノードと通信する際に使用する（すべての項目に値がセットされるとは限らない）
typedef struct {
    char serial[256];       // 基板のシリアル番号
    uint16_t id;            // Board ID
    uint16_t controller_id; // Controller ID
    uint8_t version;        // 基板のバージョン
    uint8_t chain_num;      // 基板の連結番号
    uint8_t modes;          // センシングモード数
    int32_t layout_x;       // 基板レイアウト（X）
    int32_t layout_y;       // 基板レイアウト（Y）
} Container;


/**
 *  センシング基板のシリアル番号とChain番号からBoardIDを取得（または新規生成）
 * @param addr      Storageノードのアドレス (e.g. tcp://127.0.0.1:8001)
 * @param serial    センシング基板のシリアル番号
 * @param chain_num Chain番号
 * @return          取得したBoardID
 */
inline int get_board_id(const char *addr, const char *serial, unsigned int chain_num) {

    // prepare socket
    zmq::context_t ctx(1);
    zmq::socket_t req(ctx, zmq::socket_type::req);
    req.connect(addr);

    std::vector <zmq::message_t> recv_msgs;

    // init container
    Container rqc{};
    strcpy(rqc.serial, serial);
    rqc.chain_num = chain_num;

    // Storageノードへリクエスト
    req.send(zmq::buffer("STORAGE REQ_BRDIDS"), zmq::send_flags::sndmore);
    req.send(zmq::buffer(&rqc, sizeof(rqc)), zmq::send_flags::none);

    // Storageノードからの応答を受け取り
    zmq::recv_multipart(req, std::back_inserter(recv_msgs));
    if (recv_msgs.size() < 2) return MESSAGE_ERROR; // ヘッダとID応答で2メッセージが必須

    Container rc = *recv_msgs.at(1).data<Container>();

    req.close();
    return rc.id;
}


/**
 * 現在接続中の基板情報を取得
 * @param addr      Storageノードのアドレス (e.g. tcp://127.0.0.1:8001)
 * @param boards    コンテナのリスト
 * @return  取得した基板数
 */
inline int get_connected_boards(const char *addr, std::vector<Container> *boards) {

    // prepare socket
    zmq::context_t ctx(1);
    zmq::socket_t req(ctx, zmq::socket_type::req);
    req.connect(addr);

    // build a message requesting a connected board
    char request[] = "STORAGE REQ_LAYOUT";

    // send request
    req.send(zmq::buffer(request), zmq::send_flags::none);

    // wait for replies
    std::vector<zmq::message_t> recv_msgs;
    zmq::recv_multipart(req, std::back_inserter(recv_msgs), zmq::recv_flags::dontwait);
    if(recv_msgs.empty()) return MESSAGE_ERROR;

    req.close();

    // erase header ( STORAGE REPLY )
    recv_msgs.erase(recv_msgs.begin());

    for(auto & recv_msg : recv_msgs) {
        Container c = *recv_msg.data<Container>();
        boards->push_back(c);
    }

    return recv_msgs.size();

}


/**
 * レイアウトをストレージに保存
 * @param addr      Storageノードのアドレス (e.g. tcp://127.0.0.1:8001)
 * @param bid       Board ID
 * @param x         Layout X
 * @param y         Layout Y
 */
inline void store_layout(const char *addr, unsigned int bid, int x, int y){
    // prepare socket
    zmq::context_t ctx(1);
    zmq::socket_t req(ctx, zmq::socket_type::req);
    req.connect(addr);

    // build a message requesting a connected board
    char request[] = "STORAGE STR_LAYOUT";
    char layout[128];
    snprintf(layout, 128, "%d %d %d", bid, x, y);

    // send request
    req.send(zmq::buffer(request), zmq::send_flags::sndmore);
    req.send(zmq::buffer(layout), zmq::send_flags::none);

}

#endif //MARBLED_RUNTIME_UTILITY_H
