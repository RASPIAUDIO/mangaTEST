#include <NeoPixelBus.h>
#include "Audio.h"
#include "SD.h"
#include "FS.h"

extern "C"
{
#include "hal_i2c.h"
#include "tinyScreen128x64.h"
}

#define I2S_BCLK    5
#define I2S_LRC     25
#define I2S_DOUT    26
#define SDA         18
#define SCL         23

#define LED2        GPIO_NUM_4

#define PUSH        GPIO_NUM_0
#define ROTARY_A    GPIO_NUM_32
#define ROTARY_B    GPIO_NUM_19

#define SDD GPIO_NUM_34     
#define SD_CS         13
#define SPI_MOSI      15
#define SPI_MISO      2
#define SPI_SCK       14
//////////////////////////////
// NeoPixel led control
/////////////////////////////
#define PixelCount 1
#define PixelPin 22
RgbColor RED(255, 0, 0);
RgbColor GREEN(0, 255, 0);
RgbColor BLUE(0, 0, 255);
RgbColor YELLOW(255, 128, 0);
RgbColor WHITE(255, 255, 255);
RgbColor BLACK(0, 0, 0);
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

Audio audio;
File root;
File F;
int stop;

TaskHandle_t Tencoder;
int N = 0;
char b[40];
uint8_t c[20];
void encoder(void* data)
{
  int va, vb;
  int ta = 0;
  int tb = 0;
  while(true)
  {
    va = gpio_get_level(ROTARY_A);
    vb = gpio_get_level(ROTARY_B);
    if((va == 1) && (ta == -1))ta = 0;
    if((vb == 1) && (tb == -1))tb = 0;
    if((va == 0) && (ta == 0)) ta = millis();
    if((vb == 0) && (tb == 0)) tb = millis();
    if((ta > 0) && (tb > 0))
    {
      if(ta > tb) N++; else N--;
      ta=tb=-1;
    }
    delay(1);
  }
}


void setup() {
  
Serial.begin(115200);
while(!Serial)delay(100);
 
////////////////////////////////////////////////////////////////
// init NeoPixel led handle
///////////////////////////////////////////////////////////////
  strip.Begin();  
///////////////////////////////////////////////////////////////
// init spdif led
///////////////////////////////////////////////////////////////
  gpio_reset_pin(LED2);
  gpio_set_direction(LED2, GPIO_MODE_OUTPUT); 
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
//init rotactor
///////////////////////////////////////////////////////////////

  gpio_reset_pin(PUSH);
  gpio_set_direction(PUSH, GPIO_MODE_INPUT); 
  gpio_set_pull_mode(PUSH, GPIO_PULLUP_ONLY); 

  gpio_reset_pin(ROTARY_A);
  gpio_set_direction(ROTARY_A, GPIO_MODE_INPUT); 
  gpio_set_pull_mode(ROTARY_A, GPIO_PULLUP_ONLY); 

  gpio_reset_pin(ROTARY_B);
  gpio_set_direction(ROTARY_B, GPIO_MODE_INPUT);   
  gpio_set_pull_mode(ROTARY_B, GPIO_PULLUP_ONLY); 
  
// init tiny screen
///////////////////////////////////////////////////////////////
   tinySsd_init(SDA, SCL, 0, 0x3C, 1);
//////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////
// test
//////////////////////////////////////////////////////////////    
   clearBuffer();
   drawBigStrC(24,"Ros&Co");
   sendBuffer();
   delay(1000);
   clearBuffer();
   drawBigStrC(16,"MM CAST");
   drawBigStrC(40,"Test");
   sendBuffer();
   delay(2000);
   clearBuffer();
   sendBuffer();
///////////////////////////////////////////////////////////////
// test #1 
// LED1 and LED2
///////////////////////////////////////////////////////////////
   clearBuffer();
   drawStrC(10, "Test #1: leds");
   drawBigStrC(24,"PUSH x4");
   sendBuffer();

   while(gpio_get_level(PUSH) == 1) delay(100);

   strip.SetPixelColor(0, RED);
   strip.Show();
   gpio_set_level(LED2, 1);
   delay(500);

   while(gpio_get_level(PUSH) == 1) delay(100);
      
   strip.SetPixelColor(0, GREEN);
   strip.Show();
   gpio_set_level(LED2, 1);
   delay(500);   

   while(gpio_get_level(PUSH) == 1) delay(100);
   
   strip.SetPixelColor(0, WHITE);
   strip.Show();
   gpio_set_level(LED2, 0);
   delay(500);   

   while(gpio_get_level(PUSH) == 1) delay(100);
     
   strip.SetPixelColor(0, BLACK);
   strip.Show();

   clearBuffer();
   drawStrC(10, "Test #1: leds");
   drawBigStrC(24,"OK");
   sendBuffer();

   delay(1000);
////////////////////////////////////////////////////
// test#2
// Rotational encoder
////////////////////////////////////////////////////

   xTaskCreatePinnedToCore(encoder, "encoder", 5000, NULL, 5, &Tencoder, 0);
   clearBuffer();
   drawStrC(10, "Test #2: encoder");
   drawBigStrC(24,"TURN");
   drawBigStrC(44,"LEFT");  
 
   sendBuffer();   
   delay(1000);
   N = 0;
   while(N > -16)
   {
    sprintf(b,"%d", N);
    clearBuffer();
    drawBigStrC(24, b);
    sendBuffer();
    delay(100);
   }
   delay(1000);
   clearBuffer();
   drawStrC(10, "Test #2");
   drawBigStrC(24,"OK");
   sendBuffer();
   delay(1000);
   clearBuffer();
   drawStrC(10, "Test #2: encoder");
   drawBigStrC(24,"TURN");   
   drawBigStrC(44,"RIGHT");

   sendBuffer();   
   delay(2000);
    N = 0;
   while(N < 16)
   {
    sprintf(b,"%d", N);
    clearBuffer();
    drawBigStrC(24, b);
    sendBuffer();
    delay(100);
   }
   delay(1000);
   clearBuffer();
   drawStrC(10, "Test #2: encoder");
   drawBigStrC(24,"OK");
   sendBuffer();
   delay(1000); 



/////////////////////////////////////////////////
// test#3
// SD
/////////////////////////////////////////////////   
   clearBuffer();
   drawStrC(10, "Test #3: SD");
   drawBigStrC(24,"PUSH");
   sendBuffer();
   while(gpio_get_level(PUSH) == 1) delay(100);   

   gpio_reset_pin(SDD);
   gpio_set_direction(SDD, GPIO_MODE_INPUT);  
   gpio_set_pull_mode(SDD, GPIO_PULLUP_ONLY);
   if(gpio_get_level(SDD) == 0)
   {
   clearBuffer();   
   drawStrC(10, "Test #3");
   drawStrC(24,"INSERT SD !");
   sendBuffer();
   }
   delay(1000);
   while(gpio_get_level(SDD) == 0) delay(100);

   SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
   if(!SD.begin(SD_CS))printf("init. SD failed !\n");   
   F = SD.open("/test", "w");
   F.write((const uint8_t*)"1234567890", 11);
   F.close();

   F = SD.open("/test", "r");
   int l = F.read(c, 11);
   F.close();

   if((l != 11) || (strcmp("1234567890",(char*) c) != 0))
   {
   clearBuffer();
   drawStrC(10, "Test #3: SD");
   drawBigStrC(24,"FAILED");
   sendBuffer();
   }
   else
   {
   clearBuffer();
   drawStrC(10, "Test #3: SD");
   drawBigStrC(24,"OK");
   sendBuffer();
   }
/////////////////////////////////////////////////////////////
// test#4
// sound
/////////////////////////////////////////////////////////////   
   delay(1000);
   clearBuffer();
   drawStrC(10, "Test #4: sound");
   drawBigStrC(24,"PUSH");
   sendBuffer();

   while(gpio_get_level(PUSH) == 1) delay(100); 
   audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
   if(!SPIFFS.begin())Serial.println("Erreur SPIFFS");
   //SPIFFS.format();
   File root = SPIFFS.open("/");
   audio.connecttoFS(SPIFFS, "/music.mp3");
   audio.setVolume(21); 
   clearBuffer();   
   drawStrC(10, "Test #4");
   drawBigStrC(24,"Music!..");
   sendBuffer();
   stop = millis() + 12000;
}

void loop() {

audio.loop();
if(millis() > stop)
{
   audio.stopSong();
   clearBuffer();
   drawStrC(10, "Test #4: sound");
   drawBigStrC(24,"OK");
   sendBuffer();  
}

}
