#pragma once

#include "TLL.h"
#include "AppInterface.hpp"

#include <TuioListener.h>
#include <UdpReceiver.h>
#include <TuioClient.h>

#include <iostream>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <memory>

#define PANEL_WIDTH  18
#define PANEL_HEIGHT 18

#define DEFAULT_WIDTH  (PANEL_WIDTH  * 3)
#define DEFAULT_HEIGHT (PANEL_HEIGHT * 1)

namespace tll
{
    /* アプリを動作させるためのアプリクラス */
    class AppManager : public TUIO::TuioListener
    {
    public:
        AppManager();
        ~AppManager();

        void addTuioObject(TUIO::TuioObject *tobj) override
        {
            if (this->running_app)
            {
                this->running_app->onTouched(
                    TouchInfo{
                        static_cast<uint32_t>(tobj->getSymbolID()),
                        static_cast<int32_t>(tobj->getX()),
                        static_cast<int32_t>(tobj->getY())
                    }
                );
            }
        }

        void updateTuioObject(TUIO::TuioObject *tobj) override
        {
            if (this->running_app)
            {
                this->running_app->onMoved(
                    TouchInfo{
                        static_cast<uint32_t>(tobj->getSymbolID()),
                        static_cast<int32_t>(tobj->getX()),
                        static_cast<int32_t>(tobj->getY())
                    }
                );
            }
        }

        void removeTuioObject(TUIO::TuioObject *tobj) override
        {
            if (this->running_app)
            {
                this->running_app->onReleased(
                    TouchInfo
                    {
                        static_cast<uint32_t>(tobj->getSymbolID()),
                        static_cast<int32_t>(tobj->getX()),
                        static_cast<int32_t>(tobj->getY())
                    }
                );
            }
        }

        void addTuioCursor(TUIO::TuioCursor *tcur) override {}
        void updateTuioCursor(TUIO::TuioCursor *tcur) override {}
        void removeTuioCursor(TUIO::TuioCursor *tcur) override {}
        
        void addTuioBlob(TUIO::TuioBlob *tblb) override
        {
            this->running_app->addBlob(
                BlobInfo
                {
                    static_cast<uint32_t>(tblb->getBlobID()),
                    static_cast<int32_t>(tblb->getX()),
                    static_cast<int32_t>(tblb->getY()),
                    static_cast<int32_t>(tblb->getWidth()),
                    static_cast<int32_t>(tblb->getHeight())
                }
            );
        }

        void updateTuioBlob(TUIO::TuioBlob *tblb) override
        {
            this->running_app->moveBlob(
                BlobInfo
                {
                    static_cast<uint32_t>(tblb->getBlobID()),
                    static_cast<int32_t>(tblb->getX()),
                    static_cast<int32_t>(tblb->getY()),
                    static_cast<int32_t>(tblb->getWidth()),
                    static_cast<int32_t>(tblb->getHeight())
                }
            );
        }

        void removeTuioBlob(TUIO::TuioBlob *tblb) override
        {
            this->running_app->removeBlob(
                BlobInfo
                {
                    static_cast<uint32_t>(tblb->getBlobID()),
                    static_cast<int32_t>(tblb->getX()),
                    static_cast<int32_t>(tblb->getY()),
                    static_cast<int32_t>(tblb->getWidth()),
                    static_cast<int32_t>(tblb->getHeight())
                }
            );
        }

        void refresh(TUIO::TuioTime ftime) override {}

        void run(int32_t width, int32_t height);
        bool switchApp(const std::string app_name);

        AppInterface* getRunningApp() { return this->running_app.get(); }

    private:
        uint32_t loadApps();

        // 読み込んだ（アプリ名 - DLL）のリスト
        std::unordered_map<std::string, void*> app_list;

        std::unique_ptr<class AppInterface> running_app;

        TUIO::TuioClient* tuio_client;
        TUIO::OscReceiver* osc_receiver;
    };

}
