#include "iotvp.h"

const char* IoTVP::SERVER = "103.30.247.117";

void Callback::setCallback(Callback *callback){
    chain = callback;
}

void Callback::clearCallback(){
    chain = nullptr;
}

void Callback::start(uint8_t* data){
    run(data);
    if(chain) chain->start(data);
}

BinarySerializator::BinarySerializator(uint8_t *buffer, uint8_t start){
    setup(buffer, start);
}

void BinarySerializator::setup(uint8_t *buffer, uint8_t start){
    this->buffer = buffer;
    setStart(start);
}

void BinarySerializator::setStart(uint8_t start){
    this->start = start;
    this->index = this->start;
}

void BinarySerializator::addData(const uint8_t *data, uint8_t size){
    for (uint8_t i = 0; i < size; i++){
        this->buffer[this->index+i] = data[i];
    }
    this->index += size;
}

void BinarySerializator::reset(){
    index = start;
}

uint8_t* BinarySerializator::getData(){
    return buffer;
}

uint8_t BinarySerializator::getSize(){
    return index;
}

IoTVP::IoTVP(WiFiClientSecure &client, const char* deviceId, const char* deviceToken, uint8_t* buffer)
    : IoTVP((Client &) client, deviceId, deviceToken, buffer){
    client.setCACert(IoTVP::ROOT_CA);
}

IoTVP::IoTVP(iotvp::WiFiClientSecure &client, const char* deviceId, const char* deviceToken, uint8_t* buffer)
    : IoTVP((Client &) client, deviceId, deviceToken, buffer){
    client.setCACert(IoTVP::ROOT_CA);
}

IoTVP::IoTVP(Client &client, const char* deviceId, const char* deviceToken, uint8_t* buffer)
    : deviceId(deviceId), deviceToken(deviceToken), data{buffer, 0}, 
      client(&client), mqttClient(client){
    mqttClient.setServer(IoTVP::SERVER, IoTVP::PORT);
    mqttClient.setCallback(std::bind(&IoTVP::receiveCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    data.addData((const uint8_t*)this->deviceToken.c_str(), IoTVP::TOKEN_SIZE);
    data.setStart(IoTVP::TOKEN_SIZE);
    composeTopic();
    for(int i = 0; i < MAX_VECTOR; i++){
        callbackList[i] = nullptr;
    }
}

void IoTVP::composeTopic(){
    topicData = "data/" + deviceId + "/";
    topicDebug = "debug/" + deviceId + "/";
    topicStatus = "status/" + deviceId + "/";
    topicCommand = "cmd/" + deviceId;
}

void IoTVP::sendData(const char *topic){
    blockUntilConnected();
    String currentTopic = topicData + topic;
    // Serial.print("Size of data : ");
    // Serial.println(data.getSize());
    mqttClient.publish(currentTopic.c_str(), data.getData(), data.getSize());
    reset();
}

void IoTVP::sendDebug(const char *topic){
    blockUntilConnected();
    String currentTopic = topicDebug + topic;
    // Serial.print("Size of data : ");
    // Serial.println(data.getSize());
    mqttClient.publish(currentTopic.c_str(), data.getData(), data.getSize());
    reset();
}

void IoTVP::sendStatus(const char *topic){
    blockUntilConnected();
    String currentTopic = topicStatus + topic;
    // Serial.print("Size of data : ");
    // Serial.println(data.getSize());
    mqttClient.publish(currentTopic.c_str(), data.getData(), data.getSize());
    reset();
}

void IoTVP::addData(const uint8_t *data, uint8_t size){
    // Serial.print("Size of data : ");
    // Serial.println(size);
    this->data.addData(data, size);
}

void IoTVP::addCallback(uint8_t vector, Callback &callback){
    if(callbackList[vector]){
        callback.setCallback(callbackList[vector]);
    }
    callbackList[vector] = &callback;
}

void IoTVP::reset(){
    data.reset();
}

void IoTVP::loop(){
    blockUntilConnected();
    mqttClient.loop();
}

void IoTVP::blockUntilConnected() {
    /* Loop until reconnected */
    while (!mqttClient.connected()) {
        Serial.print("MQTT connecting ...");
        /* connect now */
        if (mqttClient.connect(deviceId.c_str())) {
            Serial.println("connected");
            /* subscribe topic */
            mqttClient.subscribe(topicCommand.c_str());
        } else {
            Serial.print("failed, status code =");
            Serial.print(mqttClient.state());
            Serial.println("try again in 5 seconds");
            /* Wait 5 seconds before retrying */
            delay(5000);
        }
    }
}

void IoTVP::receiveCallback(char* topic, uint8_t* payload, unsigned int length){
    uint8_t* data = payload;
    uint8_t vector = BS_getData(uint8_t);
    if(callbackList[vector]){
        callbackList[vector]->start(data);
    }
}

const char* IoTVP::ROOT_CA = \
    "-----BEGIN CERTIFICATE-----\n" \
    "MIIEFzCCAv+gAwIBAgIUF16TZGRCgQzBhPcZDDYZ3yv4ytMwDQYJKoZIhvcNAQEL\n" \
    "BQAwgZoxCzAJBgNVBAYTAklEMRIwEAYDVQQIDAlXZXN0IEphdmExEDAOBgNVBAcM\n" \
    "B0JhbmR1bmcxEzARBgNVBAoMClFpbXRyb25pY3MxEzARBgNVBAsMCklvVCBTeXN0\n" \
    "ZW0xEjAQBgNVBAMMCWlvdHZwLmNvbTEnMCUGCSqGSIb3DQEJARYYc2FsbWFuLmdh\n" \
    "bGlsZW9AZ21haWwuY29tMB4XDTIwMDYxMDAzNDk1OVoXDTMwMDYwODAzNDk1OVow\n" \
    "gZoxCzAJBgNVBAYTAklEMRIwEAYDVQQIDAlXZXN0IEphdmExEDAOBgNVBAcMB0Jh\n" \
    "bmR1bmcxEzARBgNVBAoMClFpbXRyb25pY3MxEzARBgNVBAsMCklvVCBTeXN0ZW0x\n" \
    "EjAQBgNVBAMMCWlvdHZwLmNvbTEnMCUGCSqGSIb3DQEJARYYc2FsbWFuLmdhbGls\n" \
    "ZW9AZ21haWwuY29tMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA6bYz\n" \
    "Rp89t/A7LhuhkycBCZ9KvJ4+5h9F93uP8bNRDLaummr1CJN+ci8iL7ly3a73mEH7\n" \
    "lXEh+ssfL2vko1YTGXw0FXZAqtsw8H4fmYP6t9WvweD0iScTuh24XVhNDMP42ZA3\n" \
    "FYiA6iL0+9qOGjHC3kkUCEox/b0u5d5mzmHGr6UQ8dbocO/tYVbss9E2qarffEQh\n" \
    "RTWf3GxOkNEYkri/hbrIBafLRJGDIkP0x+aPsx4FLZdIhGurPXVi4Kfou5EFN285\n" \
    "hr7Hmiq1YiEBfyKj1vctwKp3LHtg6BLn1u8dMAQ8Re/RpWhFeIdUIqIBszaLQz8o\n" \
    "1X53ZdIiHe+EfEYAOwIDAQABo1MwUTAdBgNVHQ4EFgQUdiCT6h/9ojaiIiUUwnWH\n" \
    "4tTeEcIwHwYDVR0jBBgwFoAUdiCT6h/9ojaiIiUUwnWH4tTeEcIwDwYDVR0TAQH/\n" \
    "BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAW1HGiiQj0d/UN3E9dL0mlp4T/GEP\n" \
    "L2vEgoCysj2o2fypiCPMdtagCUaBin14bQ45dXOow6ro6DsXp8nqdMF8PsNIgN8K\n" \
    "h0UyJNKwieVSVyLBmIhfWQqiOV/PFSarYf1nORDT3OrxulG1YP4AhupyL4b4rsle\n" \
    "/2zdOy19yQRXDR2bJuRoEO98HTX9qyOtv75zL/Y85IKz8H1Tyq+poRdcldEYT8l6\n" \
    "gIr+Jbxa+Gc6bTvd6vRz3+Tr343CGdzJmv+Ueqcvzg+Nn+gPOQz+wpXF0AQDpxPD\n" \
    "08vjjHLTKGvsA5ullY5tNfM8f24tYxrKuXhNCfkFPrlAn36mNmXuo02ZbQ==\n" \
    "-----END CERTIFICATE-----";

const char* local_root_ca = \
    "-----BEGIN CERTIFICATE-----\n" \
    "MIIEFzCCAv+gAwIBAgIUF16TZGRCgQzBhPcZDDYZ3yv4ytMwDQYJKoZIhvcNAQEL\n" \
    "BQAwgZoxCzAJBgNVBAYTAklEMRIwEAYDVQQIDAlXZXN0IEphdmExEDAOBgNVBAcM\n" \
    "B0JhbmR1bmcxEzARBgNVBAoMClFpbXRyb25pY3MxEzARBgNVBAsMCklvVCBTeXN0\n" \
    "ZW0xEjAQBgNVBAMMCWlvdHZwLmNvbTEnMCUGCSqGSIb3DQEJARYYc2FsbWFuLmdh\n" \
    "bGlsZW9AZ21haWwuY29tMB4XDTIwMDYxMDAzNDk1OVoXDTMwMDYwODAzNDk1OVow\n" \
    "gZoxCzAJBgNVBAYTAklEMRIwEAYDVQQIDAlXZXN0IEphdmExEDAOBgNVBAcMB0Jh\n" \
    "bmR1bmcxEzARBgNVBAoMClFpbXRyb25pY3MxEzARBgNVBAsMCklvVCBTeXN0ZW0x\n" \
    "EjAQBgNVBAMMCWlvdHZwLmNvbTEnMCUGCSqGSIb3DQEJARYYc2FsbWFuLmdhbGls\n" \
    "ZW9AZ21haWwuY29tMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA6bYz\n" \
    "Rp89t/A7LhuhkycBCZ9KvJ4+5h9F93uP8bNRDLaummr1CJN+ci8iL7ly3a73mEH7\n" \
    "lXEh+ssfL2vko1YTGXw0FXZAqtsw8H4fmYP6t9WvweD0iScTuh24XVhNDMP42ZA3\n" \
    "FYiA6iL0+9qOGjHC3kkUCEox/b0u5d5mzmHGr6UQ8dbocO/tYVbss9E2qarffEQh\n" \
    "RTWf3GxOkNEYkri/hbrIBafLRJGDIkP0x+aPsx4FLZdIhGurPXVi4Kfou5EFN285\n" \
    "hr7Hmiq1YiEBfyKj1vctwKp3LHtg6BLn1u8dMAQ8Re/RpWhFeIdUIqIBszaLQz8o\n" \
    "1X53ZdIiHe+EfEYAOwIDAQABo1MwUTAdBgNVHQ4EFgQUdiCT6h/9ojaiIiUUwnWH\n" \
    "4tTeEcIwHwYDVR0jBBgwFoAUdiCT6h/9ojaiIiUUwnWH4tTeEcIwDwYDVR0TAQH/\n" \
    "BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAW1HGiiQj0d/UN3E9dL0mlp4T/GEP\n" \
    "L2vEgoCysj2o2fypiCPMdtagCUaBin14bQ45dXOow6ro6DsXp8nqdMF8PsNIgN8K\n" \
    "h0UyJNKwieVSVyLBmIhfWQqiOV/PFSarYf1nORDT3OrxulG1YP4AhupyL4b4rsle\n" \
    "/2zdOy19yQRXDR2bJuRoEO98HTX9qyOtv75zL/Y85IKz8H1Tyq+poRdcldEYT8l6\n" \
    "gIr+Jbxa+Gc6bTvd6vRz3+Tr343CGdzJmv+Ueqcvzg+Nn+gPOQz+wpXF0AQDpxPD\n" \
    "08vjjHLTKGvsA5ullY5tNfM8f24tYxrKuXhNCfkFPrlAn36mNmXuo02ZbQ==\n" \
    "-----END CERTIFICATE-----";