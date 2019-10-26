#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN     4
// How many NeoPixels are attached to the Arduino?
#define LED_COUNT  30
// NeoPixel brightness, 0 (min) to 255 (max)
#define BRIGHTNESS 50
#define BLYNK_PRINT Serial
#define SERIAL_TIMEOUT_THRESHOLD 1000000 // number of loops before we consider serial to be silent

union RgbColor
{
  uint32_t val;
  uint8_t colours[3]; //RGB
};


// Keep track of app state all in one place
struct State {
    byte mode;                      // Current mode: OFF/MQTT/SERIAL
    int serialByteIndex;            // how many serial bytes we've read since the last frame
    int serialTimeoutCounter;       // keep track of how many loops() since we last heard from the serial port
    RgbColor targetColor;           // target (and after the fade, the current) color set by MQTT
    RgbColor prevColor;             // the previous color set by MQTT
    bool isFading;                  // whether or not we are fading between MQTT colors
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


const char *ssid =  "mylogin";     // replace with your wifi ssid and wpa2 key
const char *pass =  "mypass1234";

WiFiClient client;


WiFiUDP Udp;
unsigned int localUdpPort = 1001;
char incomingPacket[256];
char replyPacket[10];



void setup()
{
  //ESP.wdtEnable(WDTO_8S);
  ESP.wdtDisable();
  Serial.begin(115200);
  //Blynk.begin("Authentication Token", "SSID", "Password");
  //delay(10);
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(255); // Set BRIGHTNESS to about 1/5 (max = 255)

  colorWipe(strip.Color(255,   0,   0)     , 0); // Red
  
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
  
  Udp.begin(localUdpPort);
  Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);

  // Fill along the length of the strip in various colors...
  
  colorWipe(strip.Color(  0, 255,   0)     , 0); // Green
  colorWipe(strip.Color(  0,   0, 255)     , 0); // Blue


}


void loop()
{
static int mState=0;
static int idx=0;
uint32_t color;
int32_t len;

  state.serialTimeoutCounter++;
  
  int packetSize = Udp.parsePacket();
  if (packetSize)
  {
    // receive incoming UDP packets
    //Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
    len = Udp.read(incomingPacket, 255);
    
    if (len > 4)
    {
      //Serial.printf("%d\n",len);

      //Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      //sprintf(replyPacket, "%d", ESP.getFreeHeap());
      //Udp.write(replyPacket);
      //Udp.endPacket();

      
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
  }
    


  



//kick the dog
  ESP.wdtFeed();
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



void handleSerialTimeout() {
    if (state.serialTimeoutCounter >= SERIAL_TIMEOUT_THRESHOLD) 
    {
            colorWipe(strip.Color(0, 0, 0),0);
            Serial.printf("timeout!\n");
    }
    //delay(1);
}



