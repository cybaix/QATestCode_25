#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <ESP32httpUpdate.h>
#include <WiFi.h>
#include <TFT_eSPI.h> 

#define OTA_SERVER_URL "http://badge.cactuscon.com/ota/"
#define OTA_MANIFEST_NAME "version"
#define OTA_BINARY_NAME "firmware.bin" // Don't make this firmware-version-X.bin, this shouldn't need to be changed.
#define VERSION -1

class OTA{
    private:
        static TFT_eSPI tft;
        static int isWifiAvailable();

        // Progess indicators
        static void indicate_start();
        static void indicate_wifi_connected();
        static void indicate_manifest_load();
        static void indicate_updating();

        // Error indicators
        static void indicate_error_wifi_unavailable();
        static void indicate_error_wifi_rejected(); // PW Fail or uncommon connection hiccup
        static void indicate_error_download();
        static void indicate_error_latest_version();

    public:
        static void checkOTASync();
};