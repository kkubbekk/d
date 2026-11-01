    #include "NimBLEDevice.h"
    #include "esp_log.h"
    #include "esp_timer.h"
    #include "my_events.hpp"
    #include "my_events.hpp"

    // ESP_EVENT_DECLARE_BASE(MOTOR_EVENT_BASE);

    //TODO: pozniej zrobi sie  ten stojak bardziej event driven to yolo bedzie nadawac komendy i odpytywac o dane 

        
    

    class BLE : public NimBLEServerCallbacks,public NimBLECharacteristicCallbacks
    {
        
        esp_event_loop_handle_t event_loop;
        const char* TAG = "BLE_MODULE";
        std::string name_;
        std::uint16_t uuid_;
        std::uint16_t char_1_;
        std::uint16_t char_2_;
        std::uint32_t last_time_notify{0};
        bool connected{false};
        bool subscribed{false};
        NimBLEService *pService = nullptr;
        NimBLEServer *pServer = nullptr;
        NimBLECharacteristic *pCharacteristic_data = nullptr;;
        NimBLECharacteristic *pCharacteristic_cmnd = nullptr;;



        //kolejka do uchybu danych  z yolo;


        //rozloaczono wznawiamy adverisitng
        void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
            printf("Client disconnected - start advertising\n");
            connected = false;
            NimBLEDevice::startAdvertising();
        }

        //polaczenie z tel po adverisisingu
        void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
            // printf("Client connected:\n%s", connInfo.toString().c_str()); // pozniej dodac
            
            //dodac weryfikacje czy ulaczymy sie faktycznie z naszym stoajkiem a nie innym gownem
            connected = true;
            

        }
        //telefon wlaczyl lub wylaczycl charakterystyka dodac jakas flage zeby skorzystac z notify
        void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) override 
        {
            //dodac toglowanie switchy domyslnie off a poten on sprawdzac czy jestesmy podlaczeniu jak tak to pozwlamy na notify 
            if(subValue == 0) subscribed = false;
        
        
            if(subValue == 1) subscribed = true;

        }
        //przyslzy komendy z yolo trza wrzucic do kolejki do taska ktory bedzie je parsowac 
        //DOGADAC SIE ZE STACHEM NA JAKAS WSPONA RAMKE!!
        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
            if(pCharacteristic == pCharacteristic_cmnd)
            {
            NimBLEAttValue received_data = pCharacteristic->getValue();
                //tutaj treba sie zastanowic jak to placz

                //TEST!!!!!
                std::uint8_t len = received_data.length();
                float ble_angle{};
                if(len>0)
                {
                    ble_angle = atof(received_data.c_str());
                    ESP_LOGI("BLE", "Odebrano string: %s, Kąt float: %.2f", received_data.c_str(), ble_angle);

                    esp_event_post_to(event_loop, MOTOR_EVENT_BASE, YOLO_CMND_EVENT, 
                              &ble_angle, sizeof(float), portMAX_DELAY);

                }
                
                
             
            }
            
        }
        //telefon prosi o dane mozna uzyc za pierwsyzm razem zeby zhomingowac sobie stepper i wyznaczyc zero
        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
            // printf("%s Descriptor read\n", pDescriptor->getUUID().toString().c_str());
        }
        public:



        BLE(esp_event_loop_handle_t event_handle,std::string name, std::uint16_t uuid_server,uint16_t char_1,uint16_t char_2) : name_(name),uuid_(uuid_server),char_1_(char_1),char_2_(char_2)
        {

        }
        void send_data(uint8_t * data,size_t size)
        {
            if(subscribed)
            {

            
            if((uint32_t)esp_timer_get_time()/1000UL -last_time_notify >50)
            {
                pCharacteristic_data->setValue(data,size);

                pCharacteristic_data->notify();
            }
            else
            {
                ESP_LOGE(TAG,"NOTIFY TOO FAST!!");
                return;
            }
            last_time_notify =(uint32_t)esp_timer_get_time()/1000UL;
        }

    
        }

        void ble_init()
        {

            NimBLEDevice::init(name_);
        
            pServer = NimBLEDevice::createServer();
            pServer->setCallbacks(this);

            NimBLEUUID bleUUID(uuid_);
            NimBLEUUID bleUUID1(char_1_);
            NimBLEUUID bleUUID2(char_2_);

        
            pService = pServer->createService(bleUUID);
            pCharacteristic_cmnd = pService->createCharacteristic(bleUUID1, NIMBLE_PROPERTY::WRITE );

            pCharacteristic_data = pService->createCharacteristic(bleUUID2, NIMBLE_PROPERTY::NOTIFY);
            
            pService->start();

            pCharacteristic_cmnd->setCallbacks(this);
            pCharacteristic_data->setCallbacks(this);

            NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
            pAdvertising->addServiceUUID(bleUUID);
            pAdvertising->setName(name_);        
            pAdvertising->enableScanResponse(true);
            pAdvertising->start(); 

            //  kolejki do motor thread;
            // cmndQueue = xQueueCreate(1, sizeof(float));

            

            ESP_LOGI(TAG,"ble init completed!\n");
            
        }
    };

    





    
