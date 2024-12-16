#include <WiFi.h>
#include <Arduino.h>
#include <FT6336U.h>
#include <lvgl.h>
//#include <Adafruit_MAX1704X.h>  // Include MAX17048 library
#include "pins.h"
#include <SPI.h>
#include <SD.h>
#include <ESP32RotaryEncoder.h>
#include "QA_Test/QA_Test_Sequence.h"
#include "LED/Status_LED.h"
#include "WiFi/WiFi_Settings.h"   // Wi-Fi connection settings
#include "WiFi/WiFi_Module.h"     // Wi-Fi scanning functionality
#include "version.h"
#include "Diagnostics/Diagnostics.h"
#include "BadgePirates/PirateShipAnimation.h"
#include "LED/NeoPixelControl.h"
#include "Screen/Screen_Module.h"

#define TFT_HOR_RES   240
#define TFT_VER_RES   320
#define TFT_ROTATION  LV_DISPLAY_ROTATION_270

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

lv_obj_t * main_menu; // Main screen container
lv_obj_t * window1;   // First window
lv_obj_t * window2;   // Second window

//Adafruit_MAX17048 max17048;  // Create MAX17048 object

//unsigned long lastBatteryCheck = 0;        // Track last battery check time
//const unsigned long batteryCheckInterval = 60000;  // Check battery every 60 seconds

unsigned long lastDiagnosticsRun = 0;      // Track last diagnostics run time
const unsigned long diagnosticsInterval = 600000;  // Run every 10 minutes (600000 ms)

SPIClass hspi = SPIClass(HSPI); // Using HSPI as it's unused

// Array of LED pins
int ledPins[] = {LED_PIN_1, LED_PIN_2, LED_PIN_3, LED_PIN_4, LED_PIN_5, LED_PIN_6};
int numLeds = sizeof(ledPins) / sizeof(ledPins[0]);
FT6336U ft6336u(TOUCH_I2C_SDA, TOUCH_I2C_SCL, TOUCH_RST, TOUCH_INT_PIN);

// Define color array and number of colors for NeoPixel
uint32_t colors[] = {
    statusLED.Color(255, 0, 0),   // Red
    statusLED.Color(0, 255, 0),   // Green
    statusLED.Color(0, 0, 255)    // Blue
};
int numColors = sizeof(colors) / sizeof(colors[0]);

// Variables for rotary encoder
RotaryEncoder rotaryEncoder(ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_BUTTON_PIN);
volatile bool rotaryEncoderTurnedLeftFlag, rotaryEncoderTurnedRightFlag, rotaryEncoderButtonPushedFlag;

// State tracking for the boot button with debounce
unsigned long lastEnterPressTime = 0;
bool firstEnterPressDetected = false;

/*Read the touchpad*/
void my_touchpad_read( lv_indev_t * indev, lv_indev_data_t * data )
{
    lv_display_rotation_t rotation = lv_disp_get_rotation(lv_disp_get_default());
    uint16_t x,y;

    if (ft6336u.read_td_status()) {
        data->state = LV_INDEV_STATE_PRESSED;

        // the touch driver gives coordinates rotated 180, fix them
        data->point.x = TFT_HOR_RES - ft6336u.read_touch1_x();
        data->point.y = TFT_VER_RES - ft6336u.read_touch1_y();
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/*use Arduinos millis() as tick source*/
static uint32_t my_tick(void)
{
    return millis();
}

void printDeviceInfo() {
    // Display code version
    Serial.print("Code Version: ");
    Serial.println(CODE_VERSION);

    // Display ESP MAC address
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());

    // Calculate memory usage
    uint32_t totalHeap = ESP.getHeapSize();
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t usedHeap = totalHeap - freeHeap;
    float memoryUsagePercent = (float(usedHeap) / float(totalHeap)) * 100;

    // Display memory usage
    Serial.print("Total Heap: ");
    Serial.print(totalHeap);
    Serial.println(" bytes");
    Serial.print("Free Heap: ");
    Serial.print(freeHeap);
    Serial.println(" bytes");
    Serial.print("Used Heap: ");
    Serial.print(usedHeap);
    Serial.print(" bytes (");
    Serial.print(memoryUsagePercent, 2);
    Serial.println("% used)");
}

void rotaryButtonCallback(unsigned long) {
    rotaryEncoderButtonPushedFlag = true;
}

void rotaryEncoderCallback(long value) {
    switch( value )
	{
		case 1:
	  		rotaryEncoderTurnedRightFlag = true;
		break;

		case -1:
	  		rotaryEncoderTurnedLeftFlag = true;
		break;
    }

    rotaryEncoder.setEncoderValue(0);
}

static void btn_ota_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    /*lv_obj_t * btn = lv_event_get_target(e);*/
    if(code == LV_EVENT_CLICKED) {
        
    }
}

static void btn_neo_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    /*lv_obj_t * btn = lv_event_get_target(e);*/
    if(code == LV_EVENT_CLICKED) {
        for (int j = 0; j < numColors; j++) {
            cycleNeoPixelColors();
            delay(500);
        }

        clearNeoPixels();
    }
}

/**
 * Create a button with a label and react on click event.
 */
void lv_example_get_started_2(void)
{
    lv_obj_t * btn_ota = lv_button_create(lv_screen_active());     
    lv_obj_set_pos(btn_ota, 30, 10);                            
    lv_obj_set_size(btn_ota, 120, 50);                         
    lv_obj_add_event_cb(btn_ota, btn_ota_event_cb, LV_EVENT_ALL, NULL);     

    lv_obj_t * label = lv_label_create(btn_ota);          /*Add a label to the button*/
    lv_label_set_text(label, "Activate OTA");                     /*Set the labels text*/
    lv_obj_center(label);

    lv_obj_t * btn_neo = lv_button_create(lv_screen_active());     
    lv_obj_set_pos(btn_neo, 170, 10);                            
    lv_obj_set_size(btn_neo, 120, 50);                         
    lv_obj_add_event_cb(btn_neo, btn_neo_event_cb, LV_EVENT_ALL, NULL);     

    lv_obj_t * label_neo = lv_label_create(btn_neo);          /*Add a label to the button*/
    lv_label_set_text(label_neo, "Test NeoPixels");                     /*Set the labels text*/
    lv_obj_center(label_neo);
}

void setup() {
    Serial.begin(115200);
    while (!Serial);

    // Initialize each LED pin as an output
    for (int i = 0; i < numLeds; i++) {
        pinMode(ledPins[i], OUTPUT);
    }

    initializeScreen();  // Initialize the TFT screen
    displayWelcomeMessage();  // Display a welcome message

    // Initialize I2C for MAX17048
    //if (!max17048.begin(&Wire)) {
    //    Serial.println("Could not find MAX17048 chip!");
    //} else {
    //    Serial.println("MAX17048 found!");
    //}

    // Initialize buttons with pull-up resistors
    pinMode(BUTTON_ENTER_PIN, INPUT_PULLUP);
    pinMode(BUTTON_BACK_PIN, INPUT_PULLUP);
    
    // Display ESP32 information on startup
    printDeviceInfo();

    // Initialize the NeoPixels LED
    initStatusLED();    // Initialize NeoPixels on GPIO18
    initNeoPixels();

    // Initialize SPI for SD card with custom pins
    hspi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

    ft6336u.begin();
    
    // Initialize SD card
    if (SD.begin(SD_CS, hspi)) {
        Serial.println("SD card initialized successfully.");
        if (SD.cardSize() > 0) {
            Serial.print("SD Card Size: ");
            Serial.print(SD.cardSize() / (1024 * 1024));
            Serial.println(" MB");
        } else {
            Serial.println("SD card detected but unable to determine size.");
        }
    } else {
        Serial.println("No SD card detected. Please insert an SD card.");
    }

    // Initialize rotary encoder pins
    rotaryEncoder.setEncoderType(EncoderType::FLOATING);
    rotaryEncoder.setBoundaries( -1, 1, false );
    rotaryEncoder.onTurned(&rotaryEncoderCallback);
    rotaryEncoder.onPressed(&rotaryButtonCallback);
    rotaryEncoder.begin();
    //pinMode(ENCODER_BUTTON_PIN, INPUT_PULLUP);

    // Initialize boot button
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

    // Run an initial Wi-Fi scan and connect to Wi-Fi
    //scanWiFiNetworks();     // Use scan function from WiFi_Module
    //connectToWiFi();        // Use connect function from WiFi_Settings

    // Display initial battery status
    //checkBatteryStatus();

    // Initialize diagnostics logging
    if (!initDiagnostics()) {
        Serial.println("Error initializing diagnostics.");
    }

    Serial.println("YoHoHo its a Pirates Life for me...");
    playPirateShipAnimation();  // Play the animation on startup
    lv_init();

    /*Set a tick source so that LVGL will know how much time elapsed. */
    lv_tick_set_cb(my_tick);

    /* register print function for debugging */
#if LV_USE_LOG != 0
    lv_log_register_print_cb( my_print );
#endif

    lv_display_t * disp;
    /*TFT_eSPI can be enabled lv_conf.h to initialize the display in a simple way*/
    disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, sizeof(draw_buf));

    /*Initialize the (dummy) input device driver*/
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
    lv_indev_set_read_cb(indev, my_touchpad_read);

    lv_display_set_rotation(disp, TFT_ROTATION);
    
    lv_example_get_started_2();
}

void loop() {
    lv_timer_handler();
    delay(5);
}
