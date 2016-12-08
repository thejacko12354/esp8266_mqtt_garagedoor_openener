//Requires PubSubClient found here: https://github.com/knolleary/pubsubclient
//
//ESP8266 MQTT garagedoor opener

#include <PubSubClient.h>
#include <ESP8266WiFi.h>


//setup
#define MQTT_SERVER "###.###.###.###"
const char* ssid = "#############";
const char* password = "#############";

int open_reed_pin = 4;
int closed_reed_pin = 5;
int opener_pin = 14;


char* StateTopic = "/garage/garagedoor";
char* CommandTopic = "/garage/garagedoor/cmd";

WiFiClient wifiClient;
PubSubClient client(wifiClient);

bool DoorOpen = false;


//-----------------------------------------------------------------------//

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]_");
  String message = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message = message+(char)payload[i];
  }
  Serial.print("_\n");
  Serial.print("_"+message+"_");
  String topicStr = topic;
  if (message == "open" & !DoorOpen){
    Serial.print("\nOpen called\n");
    openGarageDoor();
  }else if (message == "close" & DoorOpen){
    Serial.print("\nClose called\n");
    openGarageDoor();
  }else if( message == "STOP"){
    Serial.print("\nStop called\n");
    openGarageDoor();
  }
}

//-----------------------------------------------------------------------//


//networking functions

void reconnect() {
  Serial.println("reconnect()");
  
  //attempt to connect to the wifi if connection is lost
  if(WiFi.status() != WL_CONNECTED){

    //loop while we wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print("Try again");
    }

  }

  //make sure we are connected to WIFI before attemping to reconnect to MQTT
  if(WiFi.status() == WL_CONNECTED){
  // Loop until we're reconnected to the MQTT server
    while (!client.connected()) {

      // Generate client name based on MAC address and last 8 bits of microsecond counter
      String clientName;
      clientName += "esp8266-";
      uint8_t mac[6];
      WiFi.macAddress(mac);
      clientName += macToStr(mac);

      //if connected, subscribe to the topic(s) we want to be notified about
      if (client.connect((char*) clientName.c_str())) {
        //subscribe to topics here
        Serial.println("client connected");
        client.subscribe(CommandTopic);
        
      }
    }
  }
}

//-----------------------------------------------------------------------//

//generate unique name from MAC addr
String macToStr(const uint8_t* mac){

  String result;

  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);

    if (i < 5){
      result += ':';
    }
  }

  return result;
}

//-----------------------------------------------------------------------//

void openGarageDoor(){
  digitalWrite(opener_pin, LOW);
  delay(500);
  digitalWrite(opener_pin, HIGH);
}

//-----------------------------------------------------------------------//

void state(){
  if (digitalRead(open_reed_pin) == LOW & digitalRead(closed_reed_pin) != LOW){
    DoorOpen = true;
    //publish open
    client.publish(StateTopic, "open");
    Serial.println("Garage Door Open");
  }else if (digitalRead(open_reed_pin) != LOW & digitalRead(closed_reed_pin) == LOW){
    DoorOpen = false;
    //publish closed
    client.publish(StateTopic, "closed");
    Serial.println("Garage Door Closed");
  }else if(digitalRead(open_reed_pin) != LOW & digitalRead(closed_reed_pin) != LOW){
    if (DoorOpen){
      //publish closing
      client.publish(StateTopic, "closing");
      Serial.println("Garage Door Closing");
    }else{
      //publish opening
      client.publish(StateTopic, "opening");
      Serial.println("Garage Door Opening");
    }
  }else{
    //error
  }
}

//-----------------------------------------------------------------------//

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  delay(100);
  
  Serial.println("");
  Serial.println("MQTT Garage Door Opener");

  pinMode(closed_reed_pin, INPUT_PULLUP);
  pinMode(opener_pin, OUTPUT);
  pinMode(open_reed_pin, INPUT_PULLUP);

  digitalWrite(opener_pin, HIGH);

  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(callback);

  //start wifi subsystem
  WiFi.begin(ssid, password);
  WiFi.enableAP(false);
  

  //attempt to connect to the WIFI network and then connect to the MQTT server
  reconnect();

}

//-----------------------------------------------------------------------//

void loop() {
 state();

 if (!client.connected() && WiFi.status() == 3) {reconnect();}
 client.loop();
 delay(5000);
}
