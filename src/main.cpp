#include <WiFi.h>
#include <Arduino.h>
#include <FT6336U.h>
#include <lvgl.h>
#include <Adafruit_MAX1704X.h>  // Include MAX17048 library
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
lv_obj_t * buzzer_slider_label;

void create_main_menu();
void create_window1();
void create_window2();

Adafruit_MAX17048 max17048;  // Create MAX17048 object

SPIClass hspi = SPIClass(HSPI); // Using HSPI as it's unused

// Array of LED pins
int ledPins[] = {LED_PIN_1, LED_PIN_2, LED_PIN_3, LED_PIN_4, LED_PIN_5, LED_PIN_6};
int numLeds = sizeof(ledPins) / sizeof(ledPins[0]);
bool ledStatus = false;
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

static void slider_event_cb(lv_event_t * e) {
    lv_obj_t * slider = (lv_obj_t *)lv_event_get_target(e);
    int tone_value = lv_slider_get_value(slider); // Get the slider value

    /* Update the label */
    char buf[16];
    snprintf(buf, sizeof(buf), "Tone: %dHz", tone_value);
    lv_label_set_text(buzzer_slider_label, buf);

    /* Set RGB LED color */
    if (tone_value == 0)
        noTone(BUZZER_PIN);
    else
        tone(BUZZER_PIN, tone_value);
}

void create_buzzer_window(void) {
    window2 = lv_obj_create(NULL);  // Create a new screen for Window 1
    lv_scr_load(window2);           // Load the new screen

    lv_obj_set_flex_flow(window2, LV_FLEX_FLOW_ROW_WRAP);

    // Set padding and spacing for Flexbox layout
    lv_obj_set_style_pad_row(window2, 10, 0);  // Row spacing
    lv_obj_set_style_pad_column(window2, 10, 0); // Column spacing
    lv_obj_set_style_pad_all(window2, 20, 0);  // Padding around the grid

    lv_obj_t * slider = lv_slider_create(window2);
    lv_obj_set_width(slider, 200); // Set the slider width
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
    lv_slider_set_range(slider, 0, 2000); // Slider range
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Create a label to display the slider value */
    buzzer_slider_label = lv_label_create(window2);
    lv_label_set_text(buzzer_slider_label, "Tone: 0 Hz");
    lv_obj_align_to(buzzer_slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    // Back button to return to the main menu
    lv_obj_t * back_btn = lv_btn_create(window2);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(back_btn, [](lv_event_t * e) {
        create_main_menu();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
}

void neopixel_event_handler(lv_event_t * e) {
    lv_obj_t * btn = (lv_obj_t *)lv_event_get_target(e);
    const char * label = lv_label_get_text(lv_obj_get_child(btn, 0));

    if(strcmp(label, "Red") == 0) {
        for (int i = 0; i < NUM_NEOPIXELS; i++) {
            setNeoPixelColor(i, Adafruit_NeoPixel::Color(255, 0, 0));
        }
    } else if(strcmp(label, "Green") == 0) {
        for (int i = 0; i < NUM_NEOPIXELS; i++) {
            setNeoPixelColor(i, Adafruit_NeoPixel::Color(0, 255, 0));
        }
    } else if(strcmp(label, "Blue") == 0) {
        for (int i = 0; i < NUM_NEOPIXELS; i++) {
            setNeoPixelColor(i, Adafruit_NeoPixel::Color(0, 0, 255));
        }
    } else if(strcmp(label, "Off") == 0) {
        for (int i = 0; i < NUM_NEOPIXELS; i++) {
            clearNeoPixels();
        }
    }
}

void create_neo_window(void) {
    const String buttons[] = {"Red", "Green", "Blue", "Off"};
    window1 = lv_obj_create(NULL);  // Create a new screen for Window 1
    lv_scr_load(window1);           // Load the new screen

    lv_obj_set_flex_flow(window1, LV_FLEX_FLOW_ROW_WRAP);

    // Set padding and spacing for Flexbox layout
    lv_obj_set_style_pad_row(window1, 10, 0);  // Row spacing
    lv_obj_set_style_pad_column(window1, 10, 0); // Column spacing
    lv_obj_set_style_pad_all(window1, 20, 0);  // Padding around the grid

    for (String name : buttons) {
        lv_obj_t * btn = lv_btn_create(window1); // Button to open Window 1
        lv_obj_set_size(btn, 120, 50);
        lv_obj_add_event_cb(btn, neopixel_event_handler, LV_EVENT_CLICKED, NULL);
        lv_obj_t * label = lv_label_create(btn);
        lv_label_set_text(label, name.c_str());
        lv_obj_center(label);
    }

    // Back button to return to the main menu
    lv_obj_t * back_btn = lv_btn_create(window1);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(back_btn, [](lv_event_t * e) {
        create_main_menu();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
}

void button_event_handler(lv_event_t * e) {
    lv_obj_t * btn = (lv_obj_t *)lv_event_get_target(e);
    const char * label = lv_label_get_text(lv_obj_get_child(btn, 0));

    if (strcmp(label, "NeoPixels") == 0) {
        create_neo_window();
    } else if (strcmp(label, "LEDs") == 0) {
        ledStatus = !ledStatus;
        for (int i = 0; i < numLeds; i++) {
                digitalWrite(ledPins[i], ledStatus);
        }
    } else if (strcmp(label, "Buzzer") == 0) {
        create_buzzer_window();
    }
}

void create_main_menu()
{
    const String buttons[] = {"NeoPixels", "LEDs", "Buzzer", "SD Card", "Battery Meter", "Buttons", "Activate OTA"};
    main_menu = lv_obj_create(NULL);  // New screen
    lv_scr_load(main_menu);           // Load main menu screen

    lv_obj_set_flex_flow(main_menu, LV_FLEX_FLOW_ROW_WRAP);

    // Set padding and spacing for Flexbox layout
    lv_obj_set_style_pad_row(main_menu, 10, 0);  // Row spacing
    lv_obj_set_style_pad_column(main_menu, 10, 0); // Column spacing
    lv_obj_set_style_pad_all(main_menu, 20, 0);  // Padding around the grid

    for (String name : buttons) {
        lv_obj_t * btn = lv_btn_create(main_menu); // Button to open Window 1
        lv_obj_set_size(btn, 120, 50);
        lv_obj_add_event_cb(btn, button_event_handler, LV_EVENT_CLICKED, NULL);
        lv_obj_t * label = lv_label_create(btn);
        lv_label_set_text(label, name.c_str());
        lv_obj_center(label);
    }
    
}

void setup() {
    Serial.begin(115200);
    while (!Serial);

    // Initialize each LED pin as an output
    for (int i = 0; i < numLeds; i++) {
        pinMode(ledPins[i], OUTPUT);
    }

    //initializeScreen();  // Initialize the TFT screen
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
    
    create_main_menu();
}

void loop() {
    lv_timer_handler();
    delay(5);
}
