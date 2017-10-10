/*
    This software contains parts from:
    Talking to Hue from an Arduino
    By James Bruce (MakeUseOf.com)
    Adapted from code by Gilson Oguime. https://github.com/oguime/Hue_W5100_HT6P20B/blob/master/Hue_W5100_HT6P20B.ino

    Sketch by Johan Holmberg, Malmö University
*/
#include <SPI.h>
#include <Ethernet.h>

/* Install from https://github.com/knolleary/pubsubclient/releases/latest */
/* If you want to send packets greater than 90 bytes with Arduino WiFi Shield,
   enable the MQTT_MAX_TRANSFER_SIZE option in PubSubClient.h.
   If you want to send packets greater than 128 bytes using any shiled, set
   the desired value (MQTT_MAX_PACKET_SIZE) in PubSubClient.h.
*/
#include <PubSubClient.h>

/* Install from https://github.com/bblanchon/ArduinoJson */
#include <ArduinoJson.h>

/* Set this to 'true' to use terminal output, the the sketch will halt until a terminal is connected
   Set to 'false' to just run the sketch with no serial terminal connected */
#define TERMINAL false

//  Hue constants
const char hueHubIP[] = "0.0.0.0";  // Hue hub IP
const char hueUsername[] = "0";  // Hue username
const int hueHubPort = 80;

//  Hue variables
boolean hueOn;  // on/off
int hueBri;  // brightness value
long hueHue;  // hue value
String hueCmd;  // Hue command

// JSON buffers
#define JSON_SIZE 1024
StaticJsonBuffer<JSON_SIZE> jsonIn;
StaticJsonBuffer<JSON_SIZE> jsonOut;
char encodedJson[JSON_SIZE];

void callback(char* topic, byte* payload, unsigned int length);
void reconnectToBroker(void);

//  Ethernet

byte mac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  // W5100 MAC address
IPAddress ip(0, 0, 0, 0); // Arduino IP
EthernetClient client;
EthernetClient httpClient;

// MQTT
#define MQTT_PORT 1
#define MQTT_PUBLISH_TOPIC ""
#define MQTT_SUBSCRIBE_TOPIC ""
#define MQTT_USERNAME ""

#define MQTT_CLIENTID ""
#define MQTT_SERVER ""
#define MQTT_PASSWORD ""
#define DEVICE_ID ""  /* not really used seprately in the MQTT comm */

PubSubClient mqtt(MQTT_SERVER, MQTT_PORT, callback, client);

/*
 * Setup
 */
void setup()
{
  if (TERMINAL)
  {
    Serial.begin(9600);  // Initialize the serial port
    while (!Serial)
    {
      ; // Wait for the port to connect. Needed only for Leonardo according to Arduino
    }
    Serial.println("--- Initializing sketch ---");
  }

  // Initialize the network connection
  if (Ethernet.begin(mac) == 0 && TERMINAL)
  {
    Serial.println("Failed to configure Ethernet using DHCP");
  }
  else if (TERMINAL)
  {
    Serial.print("Connected with ");
    for (byte thisByte = 0; thisByte < 4; thisByte++)
    {
      // print the value of each byte of the IP address:
      Serial.print(Ethernet.localIP()[thisByte], DEC);
      Serial.print(".");
    }
    Serial.println();
  }

  // Initialize the MQTT connection
  reconnectToBroker();
}

void loop()
{
  if (!mqtt.connected())  // Client disconnected from the MQTT broker?
  {
    reconnectToBroker();  // If disconnected, reconnect to the broker
  }

  mqtt.loop();  // Called regularly to handle incoming messages
}

/********************************
  This is the MQTT side of things
 ********************************/

/*
 * This function is based on code from Nick O'Leary - @knolleary
 * Probably not needed to change in your own application!
 */
void reconnectToBroker(void)
{
  while (!mqtt.connected())  // Loop until reconnected
  {
    if (TERMINAL)
    {
      Serial.println("Attempting MQTT connection to broker...");
    }
    mqtt.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD);
    delay(2000);
  }
  if (TERMINAL)
  {
    Serial.println("Connected to MQTT broker...");
  }
  mqtt.subscribe(MQTT_SUBSCRIBE_TOPIC);
}

void callback(char* topic, byte* payload, unsigned int length)
{
  jsonIn.clear();
  if (TERMINAL)
  {
    Serial.print("Message arrived: [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++)
    {
      Serial.print((char)payload[i]);
    }
    Serial.println();
  }
  
  // This is where we play with the JSON data
  char charPayload[length+1];
  for (int i = 0; i < length; i++)
  {
    charPayload[i] = (char) payload[i];
  }
  charPayload[length] = '\0';
  JsonArray& root = jsonIn.parseArray(charPayload, 5);

  int light = 0;
  boolean on = false;
  long brightness = 0;
  long hue = 0;
  long saturation = 0;

  // A message may contain zero or more Hue commands. Thus, we
  // need to take care of each of them.
  for (int i = 0; i < root.size(); i++)
  {
    // First, let's read one Hue command
    light =      root[i]["id"];
    on =         root[i]["state"]["on"];
    brightness = root[i]["state"]["bri"];
    hue =        root[i]["state"]["hue"];
    saturation = root[i]["state"]["sat"];

    // Then, let's send it on its merry way
    setParameterizedHue(light, on, brightness, hue, saturation);
  }
}

/********************************
   This is the Hue side of things
 ********************************/

boolean setParameterizedHue(int light, boolean on, long brightness, long hue, long saturation)
{
  JsonObject& root = jsonOut.createObject();
  root["on"] = on;
  root["bri"] = brightness;
  root["hue"] = hue;
  root["sat"] = saturation;
  root["colormode"] = "hs";

  root.printTo((char*) encodedJson, root.measureLength() + 1);
  if (TERMINAL)
  {
    Serial.println("Sending data:");
    Serial.println(encodedJson);
  }
  
  return setHue(light, encodedJson);
}

/*
 * setHue() is our main command function, which needs to be passed a light number and a
 * properly formatted command string in JSON format (basically a Javascript style array
 * of variables and values. It then makes a simple HTTP PUT request to the Bridge at the
 * IP specified at the start.
 */
boolean setHue(int lightNum, String command)
{
  if (httpClient.connect(hueHubIP, hueHubPort))
  {
    if (TERMINAL) {
      Serial.print("Hue command: ");
      Serial.println(command);
    }
    while (httpClient.connected())
    {
      httpClient.print("PUT /api/");
      httpClient.print(hueUsername);
      httpClient.print("/lights/");
      httpClient.print(lightNum);  // hueLight zero based, add 1
      httpClient.println("/state HTTP/1.1");
      httpClient.println("keep-alive");
      httpClient.print("Host: ");
      httpClient.println(hueHubIP);
      httpClient.print("Content-Length: ");
      httpClient.println(command.length());
      httpClient.println("Content-Type: text/plain;charset=UTF-8");
      httpClient.println();  // blank line before body
      httpClient.println(command);  // Hue command
    }
    httpClient.stop();
    return true;  // command executed
  }
  else
  {
    return false;  // command failed
  }
}

/*
 * A helper function in case your logic depends on the current state of the light.
 * This sets a number of global variables which you can check to find out if a
 * light is currently on or not and the hue etc. Not needed just to send out commands
 */
boolean getHue(int lightNum)
{
  if (client.connect(hueHubIP, hueHubPort))
  {
    client.print("GET /api/");
    client.print(hueUsername);
    client.print("/lights/");
    client.print(lightNum);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(hueHubIP);
    client.println("Content-type: application/json");
    client.println("keep-alive");
    client.println();
    while (client.connected())
    {
      if (client.available())
      {
        client.findUntil("\"on\":", "\0");
        hueOn = (client.readStringUntil(',') == "true");  // if light is on, set variable to true

        client.findUntil("\"bri\":", "\0");
        hueBri = client.readStringUntil(',').toInt();  // set variable to brightness value

        client.findUntil("\"hue\":", "\0");
        hueHue = client.readStringUntil(',').toInt();  // set variable to hue value

        break;  // not capturing other light attributes yet
      }
    }
    client.stop();
    return true;  // captured on,bri,hue
  }
  else
    return false;  // error reading on,bri,hue
}

