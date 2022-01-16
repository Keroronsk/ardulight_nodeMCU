#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h> 


//#define USE_CAPTIVE_PORTAL

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN     4
// How many NeoPixels are attached to the Arduino?
#define LED_COUNT  60
// NeoPixel brightness, 0 (min) to 255 (max)
#define BRIGHTNESS 255
#define BLYNK_PRINT Serial
#define SERIAL_TIMEOUT_THRESHOLD 10000000 // number of loops before we consider serial to be silent




union RgbColor
{
  uint32_t val;
  uint8_t colours[3]; //RGB
};

WiFiManager wifiManager;
WiFiServer wifiServer(80);
// Keep track of app state all in one place
struct State {
    byte mode;                      // Current mode: OFF/MQTT/SERIAL
    int serialByteIndex;            // how many serial bytes we've read since the last frame
    uint32_t serialTimeoutCounter;  // keep track of how many loops() since we last heard from the serial port
    RgbColor targetColor;           // target (and after the fade, the current) color set by MQTT
    RgbColor prevColor;             // the previous color set by MQTT
    bool  isFading;                 // whether or not we are fading between MQTT colors
    float fadeProgress;             // how far we have faded
    float fadeStep;                 // how much the fade progress should change each iteration
} state;


// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)


const char *ssid =  "keroro_2g";     // replace with your wifi ssid and wpa2 key
const char *pass =  "zzzaq12wsxxx";

WiFiClient client;


WiFiUDP Udp;
unsigned int localUdpPort = 1001;
char incomingPacket[256];
char replyPacket[10];
// Indicates whether ESP has WiFi credentials saved from previous session
bool initialConfig = false;
const int TRIGGER_PIN = 5; // D1 on NodeMCU and WeMos.

void setup()
{
char buff[32];

  //ESP.wdtEnable(WDTO_8S);
  ESP.wdtDisable();

  Serial.begin(115200);

 switch (ESP.getResetInfoPtr()->reason) {
    
    case REASON_DEFAULT_RST: 
      // do something at normal startup by power on
      strcpy_P(buff, PSTR("Power on"));
      break;
      
    case REASON_WDT_RST:
      // do something at hardware watch dog reset
      strcpy_P(buff, PSTR("Hardware Watchdog"));     
      break;
      
    case REASON_EXCEPTION_RST:
      // do something at exception reset
      strcpy_P(buff, PSTR("Exception"));      
      break;
      
    case REASON_SOFT_WDT_RST:
      // do something at software watch dog reset
      strcpy_P(buff, PSTR("Software Watchdog"));
      break;
      
    case REASON_SOFT_RESTART: 
      // do something at software restart ,system_restart 
      strcpy_P(buff, PSTR("Software/System restart"));
      break;
      
    case REASON_DEEP_SLEEP_AWAKE:
      // do something at wake up from deep-sleep
      strcpy_P(buff, PSTR("Deep-Sleep Wake"));
      break;
      
    case REASON_EXT_SYS_RST:
      // do something at external system reset (assertion of reset pin)
      strcpy_P(buff, PSTR("External System"));
      break;
      
    default:  
      // do something when reset occured for unknown reason
      strcpy_P(buff, PSTR("Unknown"));     
      break;
  }
  
  Serial.printf("\n\nReason for reboot: %s\n", buff);
  Serial.println("----------------------------------------------");

  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  //Blynk.begin("Authentication Token", "SSID", "Password");
  delay(100);
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(BRIGHTNESS); // Set BRIGHTNESS

  colorWipe(strip.Color(255,   0,   0)     , 0); // Red
  

  //start wifi init
  Serial.println("\n Starting");
  
#if !defined USE_CAPTIVE_PORTAL

   Serial.println("Connecting to ");
   Serial.println(ssid); 
  
   WiFi.begin(ssid, pass); 
   while (WiFi.status() != WL_CONNECTED) 
   {
        delay(500);
        Serial.print(".");
        yield();
   }
  Serial.println("");
  Serial.println("WiFi connected"); 
  
#else

// is configuration portal requested?
  if (WiFi.SSID()=="" || digitalRead(TRIGGER_PIN) == HIGH){
    
    Serial.println("We haven't got any access point credentials, so get them now");   
    //initialConfig = true;

    colorWipe(strip.Color(0,   255,   0)     , 0); // Green
    
    Serial.println("Configuration portal requested.");
     
    //digitalWrite(PIN_LED, LOW); // turn the LED on by making the voltage LOW to tell us we are in configuration mode.
     
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    //sets timeout in seconds until configuration portal gets turned off.
    //If not specified device will remain in configuration mode until
    //switched off via webserver or device is restarted.
    //wifiManager.setConfigPortalTimeout(600);

    //it starts an access point 
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.startConfigPortal()) {
      Serial.println("Not connected to WiFi but continuing anyway.");
    } else {
      //if you get here you have connected to the WiFi
      Serial.println("connected...yeey :)");
    }
    
    //digitalWrite(PIN_LED, HIGH); // Turn led off as we are not in configuration mode.
    
    ESP.reset(); // This is a bit crude. For some unknown reason webserver can only be started once per boot up 
    // so resetting the device allows to go back into config mode again when it reboots.
    delay(5000);

  }
  else{
    
    
    
    //digitalWrite(PIN_LED, HIGH); // Turn led off as we are not in configuration mode.
    WiFi.mode(WIFI_STA); // Force to station mode because if device was switched off while in access point mode it will start up next time in access point mode.
    unsigned long startedAt = millis();
    
    int connRes = WiFi.waitForConnectResult();
    float waited = (millis()- startedAt);
    Serial.print(waited/1000);
    Serial.print("connection result is ");
    Serial.println(connRes);
    
  }
  
 #endif


 

  //end wifi init
  Udp.begin(localUdpPort);
  Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);

}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(void) {
  static uint16_t i, j;
  //for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    //Serial.println("*");
    j++;
    if(j>255*5)j=0;
    //delay(wait);
  //}
}


void loop()
{
static int mState=0;
static int idx=0;
static int rainbowSpeed=0;
uint32_t color;
int32_t len;
static bool fl_needDebounce=false;
static bool fl_SoftOf=false;

  state.serialTimeoutCounter++;


  if(digitalRead(TRIGGER_PIN) == HIGH)
  {
    //key is pressed, wait to release key
    if(fl_needDebounce==false)
    {
      if(fl_SoftOf==false){fl_SoftOf=true;}
      else 
      {
        fl_SoftOf=false;
        mState=0;
      }
    }
    
    fl_needDebounce=true;
  }
  else
  {
    //key released, ready to next press
    fl_needDebounce=false;
  }

  // change serialTimeoutCounter for rainbow speed
  if(mState==0)
  {
    //is not soft-off state, GAY mode
    if(fl_SoftOf==false)
    {
      state.serialTimeoutCounter=0;
      rainbowSpeed++;
      if(rainbowSpeed==10000)
      {
        rainbowCycle();
        rainbowSpeed=0;
      }
    }
    else
    {
      //soft-off, go black
      colorWipe(strip.Color(0,   0,   0)     , 0); 
    }
  }
  
  int packetSize = Udp.parsePacket();
  
  if (packetSize)
  {
    mState=1;
    // receive incoming UDP packets
    //Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
    len = Udp.read(incomingPacket, 255);
    
    if (len > 4)
    {

      
      incomingPacket[len] = 0; //string terminator for printf
      state.serialTimeoutCounter = 0;
      idx=0;

      #if 1
      while(len>0)
      {
 
          if(incomingPacket[idx]==0xFF)
          {
            state.serialByteIndex=0;
            idx++;
            len--;
            //Serial.printf("s!\n");
          }
          else
          {
            if(len>2)
            {
              color=strip.Color(incomingPacket[idx], incomingPacket[idx+1], incomingPacket[idx+2]);
              //Serial.printf("%d\n",state.serialByteIndex);
              strip.setPixelColor(state.serialByteIndex++, color);
              
              if (state.serialByteIndex == LED_COUNT) {
                  strip.show();
                  //state.mode = MODE_SERIAL_PREFIX;
                  state.serialByteIndex = 0;
              } 
            }

            len=len-3;
            idx+=3;
            
          }
      }
      #endif
      

        
    }

    idx=0;
    //Serial.printf("%s\n", incomingPacket);

    // send back a reply, to the IP address and port we got the packet from
    //Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    //Udp.write(replyPacket);
    //Udp.endPacket();
  }
  else 
  {
        //handleSerialTimeout();
        if(state.serialTimeoutCounter<SERIAL_TIMEOUT_THRESHOLD)state.serialTimeoutCounter++;
        else mState=0;
  }

//kick the dog
  ESP.wdtFeed();
  yield();
}

// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// strip.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}


void showWarning(uint32_t color, int wait1, int wait2)
{
  colorWipe(color, wait1);
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}
