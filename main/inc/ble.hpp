#pragma once
#include <cstring>
#include "NimBLEDevice.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "gps.hpp"
#include "shared_data.hpp"

// UUID
constexpr uint16_t BLE_SERVICE_UUID = 0x4A00;
constexpr uint16_t BLE_CHAR_NOTIFY_UUID = 0x4A01;
constexpr uint16_t BLE_CHAR_WRITE_YOLO_UIID = 0x4A02;
constexpr uint16_t BLE_CHAR_WRITE_HOMEGPS_UIID = 0x4A03;

class BLE : public NimBLEServerCallbacks, public NimBLECharacteristicCallbacks
{

    SharedNavigationData &nav_data;
    const char *TAG = "BLE_MODULE";
    std::string name_;
    std::uint32_t last_time_notify{0};
    bool connected{false};
    bool subscribed{false};
    NimBLEService *pService = nullptr;
    NimBLEServer *pServer = nullptr;
    NimBLECharacteristic *pCharacteristic_data = nullptr; // dane do apki z telefonu
    NimBLECharacteristic *pCharacteristic_cmnd = nullptr; // yolo komenda do ruchu + bit czy jest wizja
    NimBLECharacteristic *pCharacteristic_gps = nullptr;  ////info o stojaku azimuth lokalizacja i te sprawy

    // kolejka do uchybu danych  z yolo;

    // rozloaczono wznawiamy adverisitng
    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override
    {
        printf("Client disconnected - start advertising\n");
        connected = false;
        NimBLEDevice::startAdvertising();
    }

    // polaczenie z tel po adverisisingu
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override
    {
        // printf("Client connected:\n%s", connInfo.toString().c_str()); // pozniej dodac

        // dodac weryfikacje czy ulaczymy sie faktycznie z naszym stoajkiem a nie innym gownem
        connected = true;
        pServer->updateConnParams(connInfo.getConnHandle(), 6, 12, 0, 100);

    }
    // telefon wlaczyl lub wylaczycl charakterystyka dodac jakas flage zeby skorzystac z notify
    void onSubscribe(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo, uint16_t subValue) override
    {
        // dodac toglowanie switchy domyslnie off a poten on sprawdzac czy jestesmy podlaczeniu jak tak to pozwlamy na notify
        if (subValue == 0)
            subscribed = false;

        if (subValue == 1)
            subscribed = true;
    }
    // przyslzy komendy z yolo trza wrzucic do kolejki do taska ktory bedzie je parsowac
    // DOGADAC SIE ZE STACHEM NA JAKAS WSPONA RAMKE!!
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
    {
        if (pCharacteristic == pCharacteristic_cmnd)
        {
            NimBLEAttValue received_data = pCharacteristic->getValue();
            // tutaj treba sie zastanowic jak to placz

            // TEST!!!!!
            std::uint8_t len = received_data.length();

            if(len == 2*sizeof(float)+sizeof(uint8_t)) // pozniej sie doda jakis bit kontrolny czy cos ale to jak juz sie naklepie aplikcja
            {

                float ble_angle_x = 0.0f;
                float ble_angle_y = 0.0f;
                uint8_t yolo_cmnd_flags{}; // pozniej pewnie mozna cos dodac fajnego ni

                std::memcpy(&ble_angle_x, received_data.data(), sizeof(float));
                std::memcpy(&yolo_cmnd_flags,received_data.data()+2*sizeof(float),sizeof(uint8_t));
                std::memcpy(&ble_angle_y,received_data.data()+sizeof(float),sizeof(float));
                // ESP_LOGI("BLE", "Odebrano %d bajtow. Kąt float: %.2f", len, ble_angle);

                bool lost_target = (yolo_cmnd_flags >> 7) & 1;

                nav_data.yolo_target_lost.store(lost_target,std::memory_order_relaxed);
                nav_data.latest_yolo_error_x.store(ble_angle_x, std::memory_order_relaxed);
                nav_data.latest_yolo_error_y.store(ble_angle_y,std::memory_order_relaxed);
                nav_data.yolo_last_timestamp.store(static_cast<uint32_t>(esp_timer_get_time() / 1000ULL), std::memory_order_relaxed);
                nav_data.new_yolo_data.store(true, std::memory_order_release);
            }
        }
        if (pCharacteristic == pCharacteristic_gps)
        {
            NimBLEAttValue rx_data = pCharacteristic->getValue();
            if (rx_data.length() == sizeof(gps_struct_t))
            {
                gps_struct_t gps_pkt{};
                std::memcpy(&gps_pkt, rx_data.data(), sizeof(gps_struct_t));

                nav_data.base_lat.store(gps_pkt.latitude, std::memory_order_relaxed);
                nav_data.base_lon.store(gps_pkt.longitude, std::memory_order_relaxed);
                nav_data.base_azimuth.store(gps_pkt.azimuth, std::memory_order_relaxed);
                if (gps_pkt.homing)
                {
                    nav_data.request_homing.store(true, std::memory_order_release);
                }
            }
        }
    }
    // telefon prosi o dane mozna uzyc za pierwsyzm razem zeby zhomingowac sobie stepper i wyznaczyc zero
    void onRead(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
    {
        // printf("%s Descriptor read\n", pDescriptor->getUUID().toString().c_str());
    }

public:
    BLE(SharedNavigationData &shared_data, std::string name) : nav_data(shared_data), name_(name)
    {
    }

    void send_data(uint8_t *data, size_t size)
    {
        if (subscribed)
        {

            if ((uint32_t)esp_timer_get_time() / 1000UL - last_time_notify > 50)
            {
                pCharacteristic_data->setValue(data, size);

                pCharacteristic_data->notify();
            }
            else
            {
                ESP_LOGE(TAG, "NOTIFY TOO FAST!!");
                return;
            }
            last_time_notify = (uint32_t)esp_timer_get_time() / 1000UL;
        }
    }

    void ble_init()
    {

        NimBLEDevice::init(name_);

        pServer = NimBLEDevice::createServer();
        pServer->setCallbacks(this);

        NimBLEUUID bleUUID(BLE_SERVICE_UUID);
        NimBLEUUID bleUUID1(BLE_CHAR_NOTIFY_UUID);
        NimBLEUUID bleUUID2(BLE_CHAR_WRITE_HOMEGPS_UIID);
        NimBLEUUID bleUUID3(BLE_CHAR_WRITE_YOLO_UIID);

        pService = pServer->createService(bleUUID);

        pCharacteristic_gps = pService->createCharacteristic(bleUUID2, NIMBLE_PROPERTY::WRITE);

        pCharacteristic_cmnd = pService->createCharacteristic(bleUUID3, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);

        pCharacteristic_data = pService->createCharacteristic(bleUUID1, NIMBLE_PROPERTY::NOTIFY);

        pService->start();

        pCharacteristic_cmnd->setCallbacks(this);
        pCharacteristic_data->setCallbacks(this);
        pCharacteristic_gps->setCallbacks(this);

        NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
        pAdvertising->addServiceUUID(bleUUID);
        pAdvertising->setName(name_);
        pAdvertising->enableScanResponse(true);
        pAdvertising->start();

        //  kolejki do motor thread;
        // cmndQueue = xQueueCreate(1, sizeof(float));

        ESP_LOGI(TAG, "ble init completed!\n");
    }
};
