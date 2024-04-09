#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)

#define timeSeconds 10

Adafruit_BME280 bme;

double temperature, humidity, pressure, altitude;

/*Put your SSID & Password*/
const char* ssid = "SSID";  // Enter SSID here
const char* password = "PASSWORD";  //Enter Password here

const int ledPin = 4; // LED connected to pin 2
const int ledPin2 = 2;

const int motionSensor = 27;
const int buzzer = 26;

unsigned long now = millis();
unsigned long lastTrigger = 0;
unsigned long alarmTrigger = 0;
boolean startTimer = false;
boolean motion = false;

boolean sensor_armed = true;


String code = "arm";

//password
String alarm_password = "1234";

WebServer server(80);


////////////////////////////////

void IRAM_ATTR detectsMovement();
void handle_OnConnect();
void handle_ToggleLED();
void handle_SendData();
void handle_NotFound();
String SendHTML(float temperature, float humidity, float pressure, float altitude);
char html_page[3000];
///////////////////////////////////

char web_page[3000] = "<!DOCTYPE HTML><html>\n"
                  "<head>\n"
                  "  <title>ESP-IDF BME280 Web Server</title>\n"
                  "  <meta http-equiv=\"refresh\" content=\"10\">\n"
                  "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
                  "  <link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\" integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">\n"
                  "  <link rel=\"icon\" href=\"data:,\">\n"
                  "  <style>\n"
                  "    html { font-family: Arial; display: inline-block; text-align: center; }\n"
                  "    p { font-size: 1.2rem; }\n"
                  "    body { margin: 0; }\n"
                  "    .topnav { overflow: hidden; background-color: #4B1D3F; color: white; font-size: 1.7rem; }\n"
                  "    .content { padding: 20px; }\n"
                  "    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }\n"
                  "    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }\n"
                  "    .reading { font-size: 2.8rem; }\n"
                  "    .card.temperature { color: #0e7c7b; }\n"
                  "    .card.humidity { color: #17bebb; }\n"
                  "    .card.pressure { color: #3fca6b; }\n"
                  "    .card.gas { color: #d62246; }\n"
                  "  </style>\n"
                  "</head>\n"
                  "<body>\n"
                  "  <div class=\"topnav\">\n"
                  "    <h3>ESP-IDF BME280 WEB SERVER</h3>\n"
                  "  </div>\n"
                  "  <div class=\"content\">\n"
                  "    <div class=\"cards\">\n"
                  "      <div class=\"card temperature\">\n"
                  "        <h4><i class=\"fas fa-thermometer-half\"></i> TEMPERATURE</h4><p><span class=\"reading\">%.2f&deg;C</span></p>\n"
                  "      </div>\n"
                  "      <div class=\"card humidity\">\n"
                  "        <h4><i class=\"fas fa-tint\"></i> HUMIDITY</h4><p><span class=\"reading\">%.2f</span> &percnt;</span></p>\n"
                  "      </div>\n"
                  "      <div class=\"card pressure\">\n"
                  "        <h4><i class=\"fas fa-angle-double-down\"></i> PRESSURE</h4><p><span class=\"reading\">%.2fhPa</span></p>\n"
                  "      </div>\n"
                  "    </div>\n"
                  "    <form action=\"/led\" method=\"post\">\n"
                  "      <button class=\"button\" type=\"submit\">Toggle LED</button>\n"
                  "    </form>\n"
                  "    <form action=\"/sendData\" method=\"post\">\n"
                  "      <label for=\"data\">Enter code:</label>\n"
                  "      <input type=\"text\" id=\"data\" name=\"data\">\n"
                  "      <button class=\"button\" type=\"submit\">Send Code</button>\n"
                  "    </form>\n"
                  "  </div>\n"
                  "</body>\n"
                  "</html>";


void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  pinMode(ledPin2, OUTPUT);
  digitalWrite(ledPin2, LOW);

  pinMode(buzzer, OUTPUT);
  // PIR Motion Sensor mode INPUT_PULLUP
  pinMode(motionSensor, INPUT_PULLUP);
  // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING);

  bme.begin(0x76);   

  Serial.println("Connecting to ");
  Serial.println(ssid);

  //connect to your local wi-fi network
  WiFi.begin(ssid, password);

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());

  server.on("/", handle_OnConnect);
  server.on("/toggle", handle_ToggleLED);

  server.on("/sendData", handle_SendData);
  
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");

}

void loop() {
  server.handleClient();

  now = millis();
  
  if((digitalRead(ledPin2) == HIGH) && (motion == false)) {
    Serial.println("MOTION DETECTED!!!");
    alarmTrigger = millis();
    motion = true;
  }

  // Turn off the LED after the number of seconds defined in the timeSeconds variable
  if(startTimer && (now - lastTrigger > (timeSeconds*1000))) {
    Serial.println("Motion stopped...");
    digitalWrite(ledPin2, LOW);
    Serial.println("ALARM!!!");
    startTimer = false;
    motion = false;
  }
  if(startTimer && (now - alarmTrigger > (timeSeconds*1000))) {
    digitalWrite(buzzer, HIGH);
  }
  
  if(!sensor_armed){
    digitalWrite(buzzer, LOW);
    digitalWrite(ledPin, LOW);
  }
}

////////////////////////////////////////////////////////////////////////

// Checks if motion was detected, sets LED HIGH and starts a timer
void IRAM_ATTR detectsMovement() {
  if(sensor_armed){
  digitalWrite(ledPin2, HIGH);
  startTimer = true;
  lastTrigger = millis();
  }
}

void handle_OnConnect() {
  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure() / 100.0F;
  altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

  snprintf(html_page, sizeof(html_page), web_page, temperature, humidity, pressure);

  server.send(200, "text/html", html_page);
  //server.send(200, "text/html", SendHTML(temperature,humidity,pressure,altitude)); 
}

void handle_ToggleLED() {
  digitalWrite(ledPin, !digitalRead(ledPin)); // Toggle LED
  server.send(200, "text/plain", "LED state changed");
}

void handle_SendData() {
  String data = server.arg("data"); // Get the received data from the request
  Serial.println("Received data: " + data);

  // Check if received data is "1234", then turn on the LED, otherwise turn it off
  if (data == code) {
      sensor_armed = true;
      Serial.println("armed");
      delay(5000);
   }
   if (data == alarm_password) {
      sensor_armed = false;
      Serial.println("Sensor disarmed");
      // Activate LED
      digitalWrite(ledPin, HIGH);
      delay(3000);
      digitalWrite(ledPin, LOW);
    } 

  server.send(200, "text/plain", "Data received successfully");
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}
