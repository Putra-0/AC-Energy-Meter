/*
 * Vrms & Irms values from analogRead A0 using ACS712 and NodeMCU ESP8266 ESP-12E module. 
 * Plus WiFi connection to an access point. 
 * And ThingSpeak IoT platform Analysis access. 
 * by Alex Roman
 */
 
/*--------LIBRARIES--------*/
#include <ESP8266WiFi.h>                            
#include "ThingSpeak.h"

/*--------dEFINE SSID AND THINGSPEAKS--------*/
#define SECRET_SSID "Kost Bengkel 69" // replace  with your WiFi network name
#define SECRET_PASS "kampusparty123" // replace  with your WiFi password
#define SECRET_CH_ID 1767920 // replace with your channel number
#define SECRET_WRITE_APIKEY "NBVJAAID0FACBZFI" // replace XYZ with your channel write API Key


/*--------NodeMCU--------*/
#define PIN A0
float resolution  = 3.3 / 1024;                     // Input Voltage Range is 1V to 3.3V
                                                    // ESP8266 ADC resolution is 10-bit. 2^10 = 1024
uint32_t period = 1000000 / 60;                     // One period of a 60Hz periodic waveform 
uint32_t t_start = 0;

// setup
float zero_ADC_Value = 0;   

// loop
int cnt = 0;
float ADC = 0, Vrms = 0, Current = 0, Q = 0.000, c = 0;
float sensitivity = 0.100;                          // 185 mV/A, 100 mV/A and 0.66 mV/A for ±5A, ±20A and ±30A current range respectively. 

/*--------WiFi--------*/
char ssid[] = SECRET_SSID;                          // your network SSID (name) 
char pass[] = SECRET_PASS;                          // your network password
int keyIndex = 0;                                   // your network key Index number (needed only for WEP)
WiFiClient  client;                                 // Object

/*--------ThingSpeak--------*/
unsigned long myChannelNumber = SECRET_CH_ID;       // your ThingSpeak Channel ID
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;   // your ThingSpeak Channel API key

void setup()
{
  Serial.begin(115200);                             // Initialize Serial communication
  pinMode(PIN, INPUT);                              // Set pin A0 as read. 
  /*--------WiFi--------*/
  Serial.println();
  WiFi.begin(ssid, pass);                 // Initializes the WiFi library's network settings and provides the current status.
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)   // Return the connection status.
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  //WiFi.mode(WIFI_OFF)
  wifi_set_sleep_type(NON`E_SLEEP_T);
  /*--------NodeMCU--------*/
  t_start = micros();
  uint32_t ADC_SUM = 0, n = 0;
  while(micros() - t_start < period) {
    ADC_SUM += analogRead(PIN);
    n++;
  }
  zero_ADC_Value = ADC_SUM / n;                      // The avg analog value when no current pass throught the ACS712 sensor
  /*--------ThingSpeak--------*/
  ThingSpeak.begin(client);                          // Initialize ThingSpeak 
    
}
void loop() {

   
  /*----Vrms & Irms Calculation----*/
  t_start = micros();                             
  uint32_t ADC_Dif = 0, ADC_SUM = 0, m = 0;        
  while(micros() - t_start < period) {            // Defining one period of the waveform. US frequency(f) is 60Hz. Period = 1/f = 0.016 seg = 16,666 microsec
    ADC_Dif = zero_ADC_Value - analogRead(PIN);   // To start from 0V we need to subtracting our initial value when no current passes through the current sensor, (i.e. 750 or 2.5V).
    ADC_SUM += ADC_Dif * ADC_Dif;                 // SUM of the square
    m++;                                          // counter to be used for avg.
  }
  ADC = sqrt(ADC_SUM / m);                        // The root-mean-square ADC value. 
  Vrms = ADC * resolution ;                       // The root-mean-square analog voltage value.   
  Current = (Vrms  / sensitivity) - Q;        // The root-mean-square analog current value. Note: Q
  //------------------------------//
  
  // Every 20 seconds avg. current will be uploaded to ThingSpeak.
  c += Current;
  cnt++;
  if (cnt == 20){
    c = c / cnt;
    ThingSpeak.setField(1, c);                    // Current
    ThingSpeak.setField(2, c*120);                // Voltage
     
    // write to the ThingSpeak channel
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if(x == 200){
      Serial.println("Channel update successful.");
    }
    else{
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
    
    c = 0;
    cnt = 0;
  } 
  
  delay(1000);
}
