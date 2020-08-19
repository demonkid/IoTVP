#ifndef IOTVP_H
#define IOTVP_H

#define BS_getData(type) (*((type*) data)); data = &data[sizeof(type)]
#define BS_addData(data) addData((const uint8_t*)&data, sizeof(data))
#define BSA_addData(data) addData((const uint8_t*)data, sizeof(data))
#define BSC_addData(data, type) addData((const uint8_t*)&(const type&)data, sizeof((const type&)data))
#define createFunction(funcName, block) class Callback ## funcName: public Callback{ virtual void run(uint8_t *data) block }; Callback ## funcName funcName

#include "Arduino.h"
#include "Client.h"
#include "WiFiClientSecure-old.h"
#include "PubSubClient.h"
#include <functional>
#include <ssl_client.h>
#include <WiFiClientSecure.h>

#ifndef MAX_VECTOR
#define MAX_VECTOR 256
#endif // !MAX_VECTOR

class Callback{
    public:
        void setCallback(Callback *callback);
        void clearCallback();
        void start(uint8_t* data);
        virtual void run(uint8_t* data) = 0;
    private:
        Callback *chain = nullptr;
};

class BinarySerializator {
    public:
        BinarySerializator(uint8_t *buffer): BinarySerializator(buffer, 0){};
        BinarySerializator(uint8_t *buffer, uint8_t start);
        void setup(uint8_t *buffer, uint8_t start);
        void setStart(uint8_t start);
        void addData(const uint8_t *data, uint8_t size);
        uint8_t* getData();
        uint8_t getSize();
        void reset();
    private:
        uint8_t start;
        uint8_t index;
        uint8_t *buffer;
};

class IoTVP {
    public:
        IoTVP(WiFiClientSecure &client, const char* deviceId, const char* deviceToken, uint8_t *buffer);
        IoTVP(iotvp::WiFiClientSecure &client, const char* deviceId, const char* deviceToken, uint8_t *buffer);
        IoTVP(Client &client, const char* deviceId, const char* deviceToken, uint8_t *buffer);
        void setTopic(const char* topic);
        void sendData(const char *topic);
        void sendDebug(const char *topic);
        void sendStatus(const char *topic);
        void addData(const uint8_t *data, uint8_t size);
        void addCallback(uint8_t vector, Callback &callback);
        void loop();
        void reset();
        void blockUntilConnected();
        void receiveCallback(char* topic, uint8_t* payload, unsigned int length);

        const static char* SERVER; // Server IP
        const static int PORT = 8883; // Server Port
        const static char* ROOT_CA;

        const static uint8_t TOKEN_SIZE = 32;

    private:
        void composeTopic();

        String topicData;
        String topicDebug;
        String topicStatus;
        String topicCommand;
        String deviceId;
        String deviceToken;
        Callback* callbackList[MAX_VECTOR];
        BinarySerializator data;
        Client *client;
        PubSubClient mqttClient;
};

#endif