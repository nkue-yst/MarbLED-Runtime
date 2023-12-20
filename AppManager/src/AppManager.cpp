#include "AppManager.h"

#include <algorithm>
#include <chrono>
#include <dlfcn.h>
#include <filesystem>
#include <thread>

#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <termios.h>
#include <unistd.h>

#include "tllEngine.hpp"
#include "AppInterface.hpp"
#include "OscHandler.hpp"

#include "ip/UdpSocket.h"
#include "ip/IpEndpointName.h"

namespace
{
    bool with_osc = true;
}

namespace tll
{

    namespace
    {
        const std::string extention =
            #ifdef __APPLE__
            ".dylib";
            #elif __linux__
            ".so";
            #endif
    }

    AppManager::AppManager()
    {
        this->osc_receiver = new TUIO::UdpReceiver();
        this->tuio_client  = new TUIO::TuioClient(this->osc_receiver);
        this->tuio_client->addTuioListener(this);
        this->tuio_client->connect();

        tll::OscHandler::sendMessage("/tll/init", "192.168.0.100", 3333);
    }

    AppManager::~AppManager()
    {
        tll::OscHandler::sendMessage("/tll/terminate", "192.168.0.100", 3333);
        this->tuio_client->disconnect();
        delete this->tuio_client;
        delete this->osc_receiver;
    }

    void AppManager::run(int32_t width, int32_t height)
    {
        init(width, height);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        this->loadApps();
	    this->switchApp("Theremin");

        while (loop())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(33));

            this->running_app->run();
        }

        if (this->running_app)
            this->running_app->terminate();
        quit();
    }

    bool AppManager::switchApp(std::string app_name)
    {
        bool is_found = false;

        if (this->running_app)
        {
            this->running_app->terminate();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            this->running_app.reset();
        }

        std::for_each(this->app_list.begin(), this->app_list.end(), [this, app_name, &is_found](std::unordered_map<std::string, void*>::value_type app)
        {
            if (app.first == app_name)
            {
                createApp* createAppFunc = (createApp*)(dlsym(this->app_list.find(app_name)->second, "create"));
                std::unique_ptr<class AppInterface> app = createAppFunc();
                this->running_app = std::move(app);

                clear();
                this->running_app->init();

                is_found = true;
            }
        });

        return is_found;
    }

    uint32_t AppManager::loadApps()
    {
        uint32_t app_num = 0;

        std::cout << std::endl <<  "[Loaded applications]" << std::endl;

        auto dirs = std::filesystem::directory_iterator(std::filesystem::path("./app"));
        for (auto& dir : dirs)
        {
            std::string path = dir.path().string();

            // アプリファイルを検索し、ロードする
            if (path.find(extention.c_str()) != std::string::npos)
            {
                std::string app_file_name = dir.path().stem().string();        // 拡張子を削除
                std::string app_name = app_file_name.substr(3);                // 先頭の"lib"を削除
                this->app_list[app_name] = dlopen(path.c_str(), RTLD_LAZY);    // DLLを読み込む

                std::cout << " - " << app_name << std::endl;

                app_num++;    // 読み込んだアプリ数をカウント
            }
        }
        std::cout << std::endl;
        
        return app_num;
    }

}

int main(int argc, char** argv)
{
    int32_t width  = DEFAULT_WIDTH;
    int32_t height = DEFAULT_HEIGHT;
    bool simulate_mode = false;

    // Handling the arguments
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--without-osc") == 0)
        {
            with_osc = false;
        }
        else if (std::strcmp(argv[i], "-W") == 0)
        {
            if (i + 1 < argc)
            {
                if (std::stoi(argv[++i]) % PANEL_WIDTH != 0)
                {
                    std::cout << "Error: -W option requires a multiple of " << PANEL_WIDTH << "." << std::endl;
                    continue;
                }
                else
                {
                    width = std::stoi(argv[i]);
                }
            }
            else
            {
                std::cout << "Error: -W option requires one argument." << std::endl;
                continue;
            }
        }
        else if (std::strcmp(argv[i], "-H") == 0)
        {
            if (i + 1 < argc)
            {
                if (std::stoi(argv[++i]) % PANEL_HEIGHT != 0)
                {
                    std::cout << "Error: -H option requires a multiple of " << PANEL_HEIGHT << "." << std::endl;
                    continue;
                }
                else
                {
                    height = std::stoi(argv[i]);
                }
            }
            else
            {
                std::cout << "Error: -H option requires one argument." << std::endl;
                continue;
            }
        }
        else if (std::strcmp(argv[i], "--simulate") == 0)
        {
            simulate_mode = true;
        }
        else if (std::strcmp(argv[i], "--help") == 0)
        {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --without-osc  Disable OSC communication." << std::endl;
            std::cout << "  -W <width>     Set the width of the display." << std::endl;
            std::cout << "  -H <height>    Set the height of the display." << std::endl;
            std::cout << "  --simulate     Start with simulation mode." << std::endl;
            std::cout << "  --help         Show this help." << std::endl;
            return 0;
        }
        else
        {
            std::cout << "Error: Unknown option: " << argv[i] << std::endl << std::endl;

            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --without-osc  Disable OSC communication." << std::endl;
            std::cout << "  -W <width>     Set the width of the display." << std::endl;
            std::cout << "  -H <height>    Set the height of the display." << std::endl;
            std::cout << "  --simulate     Start with simulation mode." << std::endl;
            std::cout << "  --help         Show this help." << std::endl;
            return 1;
        }
    }

    tll::tllEngine::get()->simulate_mode = simulate_mode;
    tll::AppManager* app_manager = new tll::AppManager();

    if (with_osc)
    {
        std::thread thread_osc(tll::runOscReceiveThread, app_manager);
        thread_osc.detach();
    }

    app_manager->run(width, height);

    delete app_manager;
}
