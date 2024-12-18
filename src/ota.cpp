#include "ota.hpp"
/*
 * BadgePirates OTA updater
 * Handles WiFi updates
 */

// The code here will change badge-to-badge
#pragma region Indication Code

TFT_eSPI OTA::tft = TFT_eSPI();

void OTA::indicate_start(){
    char countdown[2];
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM); 
    tft.drawString("STARTING", tft.width() / 2, (tft.height() / 2) - 40);
    tft.drawString("UPDATE IN", tft.width() / 2, (tft.height() / 2));
    delay(1000);

    tft.setTextSize(6);
    for (int i = 3; i > 0; i--) {
        tft.fillScreen(TFT_BLACK);
        sprintf(countdown, "%d", i);
        tft.drawString(countdown, tft.width() / 2, (tft.height() / 2) - 40);
        delay(1000);
    }
    tft.setTextSize(3);
}

void OTA::indicate_wifi_connected(){
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM); 
    tft.drawString("WIFI", tft.width() / 2, (tft.height() / 2) - 40 );
    tft.drawString("CONNECTED", tft.width() / 2, (tft.height() / 2) );

}

void OTA::indicate_manifest_load(){
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM); 
    tft.drawString("MANIFEST", tft.width() / 2, (tft.height() / 2) - 40);
    tft.drawString("LOADED", tft.width() / 2, (tft.height() / 2));

}

void OTA::indicate_updating(){
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM); 
    tft.drawString("UPDATE IN", tft.width() / 2, (tft.height() / 2) - 40);
    tft.drawString("PROGRESS", tft.width() / 2, (tft.height() / 2));
    tft.setTextSize(2);
    tft.drawString("DO NOT POWER OFF", tft.width() / 2, (tft.height() / 2) + 40);
    tft.setTextSize(3);
}

// Error indicators
void OTA::indicate_error_wifi_unavailable(){
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM); 
    tft.drawString("WIFI SSID", tft.width() / 2, (tft.height() / 2) - 40);
    tft.drawString("NOT FOUND", tft.width() / 2, (tft.height() / 2));
}

void OTA::indicate_error_wifi_rejected(){
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM); 
    tft.drawString("WIFI FAILED", tft.width() / 2, (tft.height() / 2) - 40);
    tft.drawString("TO CONNECT", tft.width() / 2, (tft.height() / 2));
}

void OTA::indicate_error_download(){
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM); 
    tft.drawString("ERROR", tft.width() / 2, (tft.height() / 2) - 40);
    tft.drawString("DOWNLOADING", tft.width() / 2, (tft.height() / 2));
}

void OTA::indicate_error_latest_version(){
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM); 
    tft.drawString("ALREADY", tft.width() / 2, (tft.height() / 2) - 80);
    tft.drawString("RUNNING", tft.width() / 2, (tft.height() / 2) - 40);
    tft.drawString("LATEST", tft.width() / 2, (tft.height() / 2));
}
#pragma endregion

#pragma region OTA Code

void OTA::checkOTASync()
{
    int wifi_creds;
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(3);

    // Since this is a task, we don't get to use the memory of the OTA class.
    HTTPClient httpClient;

    // Start
    indicate_start();

    // Check update server
    // Try to download the manifest(pull file version, containing an int)
    char urlbuf[80];
    strcpy(urlbuf, OTA_SERVER_URL);
    strcat(urlbuf, OTA_MANIFEST_NAME);
    httpClient.begin(urlbuf);
    int httpCode = httpClient.GET();

    if (httpCode == 200)
    {
        // I - Manifest downloaded
        indicate_manifest_load();

        // Check
        String newFWVersion = httpClient.getString();
        Serial.print("[OTA] Current firmware version: ");
        Serial.println(VERSION);
        Serial.print("[OTA] Available firmware version: ");
        Serial.println(newFWVersion);

        int newVersion = newFWVersion.toInt();
        if (newVersion > VERSION)
        {
            // D - Downloading update
            indicate_updating();

            Serial.println("[OTA] Update is downloading/installing...");
            Serial.println("[OTA] Do not power off the badge");
            char binbuf[80];
            strcpy(binbuf, OTA_SERVER_URL);
            strcat(binbuf, OTA_BINARY_NAME);
            t_httpUpdate_return ret = ESPhttpUpdate.update(binbuf);

            // I don't think this code actually is triggered unless HTTP fails.
            switch (ret)
            {
            case HTTP_UPDATE_FAILED:
                Serial.printf("[OTA] Download failed, Error: (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());

                indicate_error_download();
                for (;;)
                {
                    delay(200);
                }
                break;

            case HTTP_UPDATE_NO_UPDATES:
                Serial.println("[OTA] Your badge is running the latest firmware");
                // Latest version
                indicate_error_latest_version();
                delay(10000);
                ESP.restart();
                break;

            case HTTP_UPDATE_OK:
                Serial.println("[OTA] Your badge is updated!");
                ESP.restart();
                break;
            }
        }
        else
        {
            // Latest version
            indicate_error_latest_version();
            delay(10000);
            ESP.restart();
        }
        WiFi.disconnect();
    }
    else
    {
#ifdef DEBUG
        Serial.println("HTTP didn't throw a 200 :(");
        Serial.println(urlbuf);
        Serial.println(httpCode);
        WiFi.disconnect();
#endif

        Serial.println("[OTA] Download server error. Please try again later.");
        indicate_error_download();
        delay(10000);
        ESP.restart();
    }
}

#pragma endregion