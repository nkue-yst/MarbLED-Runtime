/**
 * @file    SerialManager.cpp
 * @brief   Implementing a class to manage serial communication.
 * @author  Yoshito Nakaue
 * @date    2022/08/22
 */

#include "SerialManager.hpp"

#include <iostream>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <vector>

#include "tllEngine.hpp"
#include "Color.hpp"
#include "Common.hpp"
#include "Event.hpp"
#include "PanelManager.hpp"

#include "utility.h"

#include <zmq.hpp>

namespace tll
{

    namespace
    {
        constexpr uint32_t led_width  = 18;
        constexpr uint32_t led_height = 18;

        void threadSendColor()
        {
            auto send_data = []() -> void
            {
                //////////////////////////////
                ///// 送信ソケットの作成 /////
                //////////////////////////////
                zmq::context_t ctx;
                zmq::socket_t pub(ctx, zmq::socket_type::pub);
                pub.bind("tcp://*:44100");

                // 基板情報のリストを取得する
                std::vector<Container> board_list;

                zmq::socket_t req(ctx, zmq::socket_type::req);
                if (tllEngine::get()->simulate_mode || get_connected_boards(ctx, req, "tcp://127.0.0.1:8001", &board_list) < 0)
                {
                    tllEngine::get()->simulate_mode = true;
                    printLog("Start with simulation mode.");

                    // Add virtual boards information
                    Container board{};
                    for (int32_t y = 0; y < TLL_ENGINE(PanelManager)->getHeight(); y += led_height)
                    {
                        for (int32_t x = 0; x < TLL_ENGINE(PanelManager)->getWidth(); x += led_width)
                        {
                            board.id = board_list.size();
                            board.layout_x = x;
                            board.layout_y = y;
                            board_list.push_back(board);
                        }
                    }

                    // Debug print for board list
                    // std::cout << "Board list:" << std::endl;
                    // for (auto board : board_list)
                    // {
                    //     std::cout <<
                    //         "ID: " << std::setw(3) << board.id <<
                    //         ", x: " << std::setw(3) << board.layout_x <<
                    //         ", y: " << std::setw(3) << board.layout_y
                    //     << std::endl;
                    // }
                }
                req.close();

                //////////////////////////////
                ///// 色情報の送信を開始 /////
                //////////////////////////////
                printLog("Start sending color data to MarbLED boards.");
                while (!TLL_ENGINE(EventHandler)->getQuitFlag())
                {
                    if (!TLL_ENGINE(SerialManager)->send_ready)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(16));
                        continue;
                    }

                    ///////////////////////////
                    ///// Make color data /////
                    ///////////////////////////
                    for (auto board : board_list)
                    {
                        uint16_t board_id = board.id;  // Board ID
                        std::vector<uint8_t> r_array;  // Red array
                        std::vector<uint8_t> g_array;  // Green array
                        std::vector<uint8_t> b_array;  // Blue array

                        for (uint32_t y = board.layout_y; y < board.layout_y + led_height; ++y)
                        {
                            for (uint32_t x = board.layout_x; x < board.layout_x + led_width; ++x)
                            {
                                Color c = TLL_ENGINE(PanelManager)->getColor(x, y);

                                r_array.push_back(c.r_);
                                g_array.push_back(c.g_);
                                b_array.push_back(c.b_);

                                // Debug print for color data
                                // std::cout <<
                                //     "x: "   << std::setw(3) << x <<
                                //     ", y: " << std::setw(3) << y <<
                                //     ", r: " << std::setw(3) << (int)c.r_ <<
                                //     ", g: " << std::setw(3) << (int)c.g_ <<
                                //     ", b: " << std::setw(3) << (int)c.b_
                                // << std::endl;
                            }
                        }

                        char topic[256] = {};
                        std::sprintf(topic, "BRD_COLOR %d", board_id);

                        // Debug print for topic
                        // std::cout << topic << std::endl;

                        //////////////////////////////////////////
                        ///// Send board ID and pixel colors /////
                        //////////////////////////////////////////
                        pub.send(zmq::message_t(topic),  zmq::send_flags::sndmore);   // Board ID
                        pub.send(zmq::message_t(r_array), zmq::send_flags::sndmore);  // Red
                        pub.send(zmq::message_t(g_array), zmq::send_flags::sndmore);  // Green
                        pub.send(zmq::message_t(b_array), zmq::send_flags::none);     // Blue
                    }

                    TLL_ENGINE(SerialManager)->send_ready = false;
                }

                pub.close();
            };

            std::thread th_send_data(send_data);
            th_send_data.detach();
        }
    }

    ISerialManager* ISerialManager::create()
    {
        return new SerialManager();
    }

    SerialManager::SerialManager() noexcept
    {
        printLog("Create Serial manager");
    }

    SerialManager::~SerialManager() noexcept
    {
        printLog("Destroy Serial manager");
    }

    void SerialManager::init()
    {
        threadSendColor();
    }

    void SerialManager::sendColorData()
    {
        TLL_ENGINE(SerialManager)->send_ready = true;
    }

}
