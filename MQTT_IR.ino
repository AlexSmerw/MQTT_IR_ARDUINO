#include <ir_Lego_PF_BitStreamEncoder.h>
#include <IRremoteInt.h>
#include <boarddefs.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include <DHT_U.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <DHT.h>
#include "IRremote.h"

//{"data1":"0x9840060A","data2":"0x4008E","type":"gree"}
// Update these with values suitable for your network.
byte mac[] = { 0xEE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 172);
IPAddress server(192, 168, 1, 171);
EthernetClient ethClient;

#define DHTPIN          7
#define DHTTYPE         DHT11
DHT dht(DHTPIN, DHTTYPE);

PubSubClient clientMqtt(server, 1883, callback, ethClient);
char mqttClient[] = "arduinoClient";
char mqttLogin[] = "sensor";
char mqttPassword[] = "qwerty";
char temperatureTopic[] = "living_room/temperature";
char humidityTopic[] = "living_room/humidity";
char irListenTopoic[] = "ir/living_room/listener";

const int sendIrCount = 3;
unsigned long timingSend;
unsigned long timingRecive;

IRrecv irrecv(2);
IRsend irsend;

StaticJsonBuffer<200> jsonBuffer;

void setup() {
	Ethernet.begin(mac, ip);
	dht.begin();
	//irrecv.enableIRIn();
	Serial.begin(9600);
}

void loop() {
	if (!clientMqtt.connected()) {
		reconnectMqtt();
	}
	if (millis() - timingSend > 15 * 1000) { //15 seconds
		sendTemperature();
		timingSend = millis();
	}
	//client.disconnect();
	//This should be called regularly to allow the client to process incoming messages and maintain its connection to the server.
	clientMqtt.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {

	char inData[120];
	for (int i = 0; i < length; i++) {
		inData[i] = (char)payload[i];
	}

	
    JsonObject& root = jsonBuffer.parseObject(inData);

    char* type = root["type"];
	unsigned long data1 = strtoul(root["data1"], NULL, 16);
	unsigned long data2 = strtoul(root["data2"], NULL, 16);
	int nbits = (int)root["nbits"];
	sendIRSignalToDevise(type, data1, data2, nbits);
	jsonBuffer.clear();
	//digitalWrite(base, HIGH);
}

void reconnectMqtt() {
	// Loop until we're reconnected
	while (!clientMqtt.connected()) {
		clientMqtt.connect(mqttClient, mqttLogin, mqttPassword);
		clientMqtt.subscribe(irListenTopoic);
		delay(500);
	}
}

char * getTemperature() {
	float temperature;
	char tempChar[6];
	temperature = dht.readTemperature();
	dtostrf(temperature, 4, 2, tempChar);
	return tempChar;
}

char * getHumidity() {
	float humidity;
	char humChar[6];
	humidity = dht.readHumidity();
	dtostrf(humidity, 4, 2, humChar);
	return humChar;
}

void sendTemperature() {
	if (!clientMqtt.connected()) {
		reconnectMqtt();
	}
	float humidity;
	char humChar[6];
	float temperature;
	char tempChar[6];

	temperature = dht.readTemperature();
	dtostrf(temperature, 4, 2, tempChar);
	humidity = dht.readHumidity();
	dtostrf(humidity, 4, 2, humChar);

	clientMqtt.publish(temperatureTopic, tempChar);
	clientMqtt.publish(humidityTopic, humChar);
}

void sendIRSignalToDevise(char* type, unsigned long data1, unsigned long data2, int nbits)
{
	int iterations = 0;
	Serial.println(type[0]);
	while (iterations < sendIrCount) {
		iterations++;
		switch (type[0]) {
		case 'g':
			irsend.sendGREE(data1, data2);
			break;
		case 'p':
			irsend.sendRC6(data1, nbits);
			irsend.sendRC6(data2, nbits);
			break;
		default:
			break;
			// выполняется, если не выбрана ни одна альтернатива
			// default необязателен
		}
		delay(100);
	}
}