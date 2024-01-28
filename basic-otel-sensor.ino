/*
 * Basic sensor using OpenTelemetry standards
 *
 * This is a very basic example of counting the changes to a sensor, 
 * incrementing a counter and then sending this over OTLP as Protobuf.
 * 
 * Those compiling this will need to also compile NanoPB against the latest 
 * OpenTelemetry Proto, so the requiste *.c and *.h files are created. 
 */

// Out-of-the-box libs
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include "time.h"

// Custom, hand-written libs
#include "neopixelSPI.h"
#include "customOtelProtobuf.h"
#include "customSendProtobuf.h"

/*
 * Setup various connectivity pieces
 */
#include "connectionDetails.h"
// WiFi
const char *ssid = WIFISSID;
const char *pass = WIFIPSK;
// NTP
const char *ntphost = NTPHOST;
// OpenTelemetry Endpoint
char *host = OTELHOST;
int port = OTELPORT;
char *uri = OTELURI;
char *xsfkey = XSFKEY;

// Use SPI to set Neopixel LED
SPIClass *SPI1 = NULL;

// For locking
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// Average blink time is 5ms (4680us max observed on a Honeywell AS302P)
struct LEDStatus {
    const int sensorPin;
    volatile bool unseenChange;
    volatile unsigned long long blinkCount;
};

// Use the RX pin on the QT Py and reset the counter and change flag
LEDStatus ledStatus = { RX, false, 0 };

// Increment the counter on a blink and flag it
void IRAM_ATTR blinkResponseRising() {
    portENTER_CRITICAL_ISR(&mux);
    ledStatus.blinkCount++;
    ledStatus.unseenChange = true;
    portEXIT_CRITICAL_ISR(&mux);
}

void setup()
{
    // Connect WiFi
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(5000);
    }

    // Send Output on Serial Console
    Serial.begin(115200);
    Serial.println("Wireless connected");

    // NTP
    struct tm tm;
    time_t t;
    struct timeval tv;
    configTime(0, 0, ntphost);

    // Get time from NTP
    getLocalTime(&tm);

    // Set time on HW clock
    t = mktime(&tm);
    tv = { .tv_sec = t };
    settimeofday(&tv, NULL);

    // Read Data from this pin
    pinMode(digitalPinToInterrupt(ledStatus.sensorPin), INPUT_PULLDOWN);

    // TODO - try again with RISING? If so, debounce will need to be done possibly
    attachInterrupt(ledStatus.sensorPin, blinkResponseRising, RISING);
    
    // Set up the pins for the Neopixel LED
    // FSPI on QT Py: SCK(36), MISO(37), MOSI(35), SS(34)
    SPI1 = new SPIClass(FSPI);
    SPI1->begin(SCK, MISO, PIN_NEOPIXEL, SS);
}

void loop()
{
    // If we get an increment...
    if (ledStatus.unseenChange == true) {
        // Reset the "seen" flag
        portENTER_CRITICAL(&mux);
        ledStatus.unseenChange = false;
        portEXIT_CRITICAL(&mux);
        
        // Turn on the Neopixel LED using SPI
        setNeopixelOn(SPI1);

        // Initialise an empty protobuf buffer
        uint8_t payloadData[MAX_PROTOBUF_BYTES] = { 0 };

        // Create the data store with data structure (default)
        Rmetricsptr myDataPtr = createDataPtr();

        // Update the Payload with our metadata
        myDataPtr->attributes->key = "service.name";
        myDataPtr->attributes->value = "house-power";
        myDataPtr->metrics->description = "Honeywell AS302P";
        myDataPtr->metrics->name = "smartmeter";
        myDataPtr->metrics->unit = "1";    
        myDataPtr->metrics->datapoints->attributes->key = "location";
        myDataPtr->metrics->datapoints->attributes->value = "downstairs";

        // Update the Payload with our datapoint metrics
        myDataPtr->metrics->datapoints->time = getEpochNano();
        myDataPtr->metrics->datapoints->value.as_int = ledStatus.blinkCount;

        // Convert dataset to protobuf
        size_t payloadSize = buildProtobuf(myDataPtr, payloadData, MAX_PROTOBUF_BYTES);

        // Send the data if there's something there
        if(payloadSize > 0) {
            sendProtobuf(host, port, uri, xsfkey, payloadData, payloadSize);
        }

        // Free the data store
        releaseDataPtr(myDataPtr);

        // Serial output
        Serial.println(ledStatus.blinkCount);

        // Turn off the Neopixel LED using SPI
        setNeopixelOff(SPI1);
    }

    // Don't check constantly
    delay(10000);
}