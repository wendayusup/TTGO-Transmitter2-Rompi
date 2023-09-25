// Version 1.0
// Wenda Yusup
// PT. Makerindo Prima Solusi
// 26/08/2023
//COM16

//-------------------------------------------------TRANSMITTER RMP02------------------------------------------------

// -----------------------------------------------------------------------Library 
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <axp20x.h>
#include <TinyGPS++.h>

// -----------------------------------------------------------------------Definition UART GPS
#define RX 34
#define TX 12
HardwareSerial neogps(1);
TinyGPSPlus gps;

// -----------------------------------------------------------------------Definition SPI & LoRa
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26
#define BAND 915E6

//---------------------------Insialisasi variabel untuk kirim dan terima pesan pada komunikasi LoRa
String outgoing;
String ID = "MKRRMP021222";
String PayLoad;
String latitude = "0"; 
String longitude = "0"; 
String sog = "0"; 
String cog = "0";  
String STAT = "0";
int batt;
String RSI = "NAN";
String SNR = "NAN";

// -------------------------------------------------------------------------Definition Button Rompi
#define buttonreset 14
#define buttonreroute 25

// -------------------------------------------------------------------------Battery Setup
AXP20X_Class axp;
const float fullvol = 4.2;
const float minvol = 0;

// -------------------------------------------------------------------------OLED Setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#define NUMFLAKES 10
#define LOGO_HEIGHT 16
#define LOGO_WIDTH 16

//---------------------------------------------------------------------------To Gateway Setup
byte msgCount = 0;       // count of outgoing messages
byte NextAddress = 0x13; // address of this device
long lastSendTime = 0;   // last send time
int localAddress = 2;    // address of this device
int interval = 50;       // interval between sends
int count;
int destination = 0; // destination to send to
bool state, state2;

// ---------------------------------------PROCEDURE FOR MAIN CODE----------------------------------------------------

// Main Code Start Device Procedure
void startdevice()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("");
  display.println(String() + "   " + "START");
  display.println("");
  display.setTextSize(1);
  display.println(String() + "Press REROUTE button!");
  display.display();
}

// Main Code GPS & Showing OLED
void gpsdata()
{
  while (neogps.available())
    if (gps.encode(neogps.read()))

      if (gps.location.isValid())
      {
        // Calculation & Definition
        latitude = String(gps.location.lat(), 6);
        longitude = String(gps.location.lng(), 6);
        sog = String(gps.speed.kmph(), 1);
        cog = ((gps.course.deg()));
        batt = (axp.getBattVoltage() / fullvol - minvol) * 100 / 1000;

        // OLED Showing
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println(String() + "MKRRMP021222" + "    RMP02");
        display.println("---------------------");
        display.println(String() + "RSSI : " + LoRa.packetRssi());
        display.println(String() + "Lat  : " + latitude);
        display.println(String() + "Lon  : " + longitude);
        display.println(String() + "Sog  : " + sog + " Km/h");
        display.println(String() + "Cog  : " + cog + " Deg");
        display.println(String() + "Batt : " + batt + "%");
        display.display();

        // Serial Showing
        // Serial.print("Position:");
        // Serial.print(gps.location.lat(), 6);
        // Serial.print(F(","));
        // Serial.println(gps.location.lng(), 6);
        // Serial.print("Sog : ");
        // Serial.println(gps.speed.kmph());
        // Serial.print("Cog :");
        // Serial.println(gps.course.deg());
        // Serial.print("Batt :");
        // Serial.print(batt);
        // Serial.println("%");
      }
      else
      {
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println(String() + "Searching GPS...");
        display.setTextSize(1);
        display.println(String() + "LoRa Active!" + "         RSSI: " + LoRa.packetRssi() + " " + "   SNR:" + LoRa.packetSnr());
        display.println(String() + "Press RESET button ifit has still no GPS.");
        display.display();
      }
}

// Main Code STARTING LORA
void startLoRa()
{
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
  LoRa.begin(915E6);
  Serial.println("LoRa Initialization OK!");
}

// Main Code SENDING MESSAGE LORA
void sendMessage(String outgoing)
{
  LoRa.beginPacket();            // start packet
  LoRa.write(destination);       // add destination address
  LoRa.write(localAddress);      // add sender address
  LoRa.write(msgCount);          // add message ID
  LoRa.write(outgoing.length()); // add payload length
  LoRa.print(outgoing);          // add payload
  LoRa.endPacket();              // finish packet and send it
  msgCount++;                    // increment message ID
}

// Main Code SETUP MESSAGE LORA
void onReceive01(int packetSize)
{
  if (packetSize == 0)
    return; // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();       // recipient address
  byte sender = LoRa.read();         // sender address
  byte incomingMsgId = LoRa.read();  // incoming msg ID
  byte incomingLength = LoRa.read(); // incoming msg length
  String incoming = "";

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0x10)
  {
    return; // skip rest of function
  }
  else if (recipient == localAddress)
  {
    STAT = String() + "CONNECTED";
    sendMessage(PayLoad);
    count = 0;
    state = true;
  }
  while (LoRa.available())
  {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length())
  { // check length for error
    Serial.println("error: message length does not match length");
    return; // skip rest of function
  }

   Serial.println(incoming);
  RSI = String(LoRa.packetRssi());
  SNR = String(LoRa.packetSnr());
}

//----------------------------------------------MAIN VOID---------------------------------------------------------------------
void setup()
{
  //------------------------------------Begin Section/Pengaktifan Library

  // Starting Serial
  Serial.begin(115200);
  // Starting GPS
  neogps.begin(9600, SERIAL_8N1, RX, TX);
  // Starting OLED
  display.begin();
  // Starting Wire
  Wire.begin();
  // Starting LoRa
  startLoRa();

  //-----------------------------------pinMode
  pinMode(buttonreset, INPUT_PULLUP);
  pinMode(buttonreroute, INPUT_PULLUP);

  //-----------------------------------Protect Monitoring if Failed
  // Battery Monitoring
  if (!axp.begin(Wire, AXP192_SLAVE_ADDRESS))
    ;
  {
    Serial.println("Battery Failed");
  }

  // OLED Monitoring
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("OLED FAILED"));
    for (;;)
      ;
  }

  // Main Code Start Device
  // startdevice();
}

void loop()
{ 
  //-----------------------------------------------------------------RESET Button
  if (digitalRead(buttonreroute) == 0 && digitalRead(buttonreset) == 1)
  {

    int i = 0;
    while (i < 3)
    {
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 0);
      display.println("\n Resetting");
      display.println(String() + "     " + (i + 1));
      display.display();
      delay(1000);
      i++;
    }
    ESP.restart();
  }
  batt = (axp.getBattVoltage() / fullvol - minvol) * 100 / 1000;
  Serial.print("Batt :");
  Serial.print(batt);
  Serial.println("%");
  gpsdata();

  PayLoad = String() + ID + "," + latitude + "," + longitude + "," + sog + "," + batt + ",0,*";
  // Serial Showing

  //-------------------------------------------------------------SENDING DATA for LORA
  onReceive01(LoRa.parsePacket());