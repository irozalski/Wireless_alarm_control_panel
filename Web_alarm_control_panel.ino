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
const char* ssid = "ssid";  // Enter SSID here
const char* password = "password";  //Enter Password here

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
// Checks if motion was detected, sets LED HIGH and starts a timer
void IRAM_ATTR detectsMovement() {
  if(sensor_armed){
  digitalWrite(ledPin2, HIGH);
  startTimer = true;
  lastTrigger = millis();
  }
}

String code = "arm";

//password
String alarm_password = "1234";

WebServer server(80);

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

void handle_OnConnect() {
  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure() / 100.0F;
  altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  server.send(200, "text/html", SendHTML(temperature,humidity,pressure,altitude)); 
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

String SendHTML(float temperature, float humidity, float pressure, float altitude) {
  String ptr = "<!DOCTYPE html>";
  ptr += "<html>";
  ptr += "<head>";
  ptr += "<title>ESP32 Weather Station</title>";
  ptr += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  ptr += "<link href='https://fonts.googleapis.com/css?family=Open+Sans:300,400,600' rel='stylesheet'>";
  ptr += "<style>";
  ptr += "html { font-family: 'Open Sans', sans-serif; display: block; margin: 0px auto; text-align: center;color: #444444;}";
  ptr += "body{margin: 0px;} ";
  ptr += "h1 {margin: 50px auto 30px;} ";
  ptr += ".side-by-side{display: table-cell;vertical-align: middle;position: relative;}";
  ptr += ".text{font-weight: 600;font-size: 19px;width: 200px;}";
  ptr += ".reading{font-weight: 300;font-size: 50px;padding-right: 25px;}";
  ptr += ".temperature .reading{color: #F29C1F;}";
  ptr += ".humidity .reading{color: #3B97D3;}";
  ptr += ".pressure .reading{color: #26B99A;}";
  ptr += ".altitude .reading{color: #955BA5;}";
  ptr += ".superscript{font-size: 17px;font-weight: 600;position: absolute;top: 10px;}";
  ptr += ".data{padding: 10px;}";
  ptr += ".container{display: table;margin: 0 auto;}";
  ptr += ".icon{width:65px}";
  ptr += "</style>";
  ptr += "<style>";
  ptr += ".button {";
  ptr += "background-color: #008CBA;";
  ptr += "border: none;";
  ptr += "color: white;";
  ptr += "padding: 15px 32px;";
  ptr += "text-align: center;";
  ptr += "text-decoration: none;";
  ptr += "display: inline-block;";
  ptr += "font-size: 16px;";
  ptr += "border-radius: 8px;";
  ptr += "}";
  ptr += "</style>";
  ptr += "</head>";
  ptr += "<body>";
  ptr += "<h1>ESP32 Alarm Panel</h1>";
  ptr += "<h3>-------------------------</h3>";
  ptr += "<div class='container'>";
  ptr += "<div class='data temperature'>";
  ptr += "<div class='side-by-side icon'>";
  ptr += "<svg enable-background='new 0 0 19.438 54.003'height=54.003px id=Layer_1 version=1.1 viewBox='0 0 19.438 54.003'width=19.438px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M11.976,8.82v-2h4.084V6.063C16.06,2.715,13.345,0,9.996,0H9.313C5.965,0,3.252,2.715,3.252,6.063v30.982";
  ptr += "C1.261,38.825,0,41.403,0,44.286c0,5.367,4.351,9.718,9.719,9.718c5.368,0,9.719-4.351,9.719-9.718";
  ptr += "c0-2.943-1.312-5.574-3.378-7.355V18.436h-3.914v-2h3.914v-2.808h-4.084v-2h4.084V8.82H11.976z M15.302,44.833";
  ptr += "c0,3.083-2.5,5.583-5.583,5.583s-5.583-2.5-5.583-5.583c0-2.279,1.368-4.236,3.326-5.104V24.257C7.462,23.01,8.472,22,9.719,22";
  ptr += "s2.257,1.01,2.257,2.257V39.73C13.934,40.597,15.302,42.554,15.302,44.833z'fill=#F29C21 /></g></svg>";
  ptr += "</div>";
  ptr += "<div class='side-by-side text'>Temperature</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += (int)temperature;
  ptr += "<span class='superscript'>&deg;C</span></div>";
  ptr += "</div>";
  ptr += "<div class='data humidity'>";
  ptr += "<div class='side-by-side icon'>";
  ptr += "<svg enable-background='new 0 0 29.235 40.64'height=40.64px id=Layer_1 version=1.1 viewBox='0 0 29.235 40.64'width=29.235px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><path d='M14.618,0C14.618,0,0,17.95,0,26.022C0,34.096,6.544,40.64,14.618,40.64s14.617-6.544,14.617-14.617";
  ptr += "C29.235,17.95,14.618,0,14.618,0z M13.667,37.135c-5.604,0-10.162-4.56-10.162-10.162c0-0.787,0.638-1.426,1.426-1.426";
  ptr += "c0.787,0,1.425,0.639,1.425,1.426c0,4.031,3.28,7.312,7.311,7.312c0.787,0,1.425,0.638,1.425,1.425";
  ptr += "C15.093,36.497,14.455,37.135,13.667,37.135z'fill=#3C97D3 /></svg>";
  ptr += "</div>";
  ptr += "<div class='side-by-side text'>Humidity</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += (int)humidity;
  ptr += "<span class='superscript'>%</span></div>";
  ptr += "</div>";
  ptr += "<div class='data pressure'>";
  ptr += "<div class='side-by-side icon'>";
  ptr += "<svg enable-background='new 0 0 40.542 40.541'height=40.541px id=Layer_1 version=1.1 viewBox='0 0 40.542 40.541'width=40.542px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M34.313,20.271c0-0.552,0.447-1,1-1h5.178c-0.236-4.841-2.163-9.228-5.214-12.593l-3.425,3.424";
  ptr += "c-0.195,0.195-0.451,0.293-0.707,0.293s-0.512-0.098-0.707-0.293c-0.391-0.391-0.391-1.023,0-1.414l3.425-3.424";
  ptr += "c-3.375-3.059-7.776-4.987-12.634-5.215c0.015,0.067,0.041,0.13,0.041,0.202v4.687c0,0.552-0.447,1-1,1s-1-0.448-1-1V0.25";
  ptr += "c0-0.071,0.026-0.134,0.041-0.202C14.39,0.279,9.936,2.256,6.544,5.385l3.576,3.577c0.391,0.391,0.391,1.024,0,1.414";
  ptr += "c-0.195,0.195-0.451,0.293-0.707,0.293s-0.512-0.098-0.707-0.293L5.142,6.812c-2.98,3.348-4.858,7.682-5.092,12.459h4.804";
  ptr += "c0.552,0,1,0.448,1,1s-0.448,1-1,1H0.05c0.525,10.728,9.362,19.271,20.22,19.271c10.857,0,19.696-8.543,20.22-19.271h-5.178";
  ptr += "C34.76,21.271,34.313,20.823,34.313,20.271z M23.084,22.037c-0.559,1.561-2.274,2.372-3.833,1.814";
  ptr += "c-1.561-0.557-2.373-2.272-1.815-3.833c0.372-1.041,1.263-1.737,2.277-1.928L25.2,7.202L22.497,19.05";
  ptr += "C23.196,19.843,23.464,20.973,23.084,22.037z'fill=#26B999 /></g></svg>";
  ptr += "</div>";
  ptr += "<div class='side-by-side text'>Pressure</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += (int)pressure;
  ptr += "<span class='superscript'>hPa</span></div>";
  ptr += "</div>";
  ptr += "<div class='data altitude'>";
  ptr += "<div class='side-by-side icon'>";
  ptr += "<svg enable-background='new 0 0 58.422 40.639'height=40.639px id=Layer_1 version=1.1 viewBox='0 0 58.422 40.639'width=58.422px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M58.203,37.754l0.007-0.004L42.09,9.935l-0.001,0.001c-0.356-0.543-0.969-0.902-1.667-0.902";
  ptr += "c-0.655,0-1.231,0.32-1.595,0.808l-0.011-0.007l-0.039,0.067c-0.021,0.03-0.035,0.063-0.054,0.094L22.78,37.692l0.008,0.004";
  ptr += "c-0.149,0.28-0.242,0.594-0.242,0.934c0,1.102,0.894,1.995,1.994,1.995v0.015h31.888c1.101,0,1.994-0.893,1.994-1.994";
  ptr += "C58.422,38.323,58.339,38.024,58.203,37.754z'fill=#955BA5 /><path d='M19.704,38.674l-0.013-0.004l13.544-23.522L25.13,1.156l-0.002,0.001C24.671,0.459,23.885,0,22.985,0";
  ptr += "c-0.84,0-1.582,0.41-2.051,1.038l-0.016-0.01L20.87,1.114c-0.025,0.039-0.046,0.082-0.068,0.124L0.299,36.851l0.013,0.004";
  ptr += "C0.117,37.215,0,37.62,0,38.059c0,1.412,1.147,2.565,2.565,2.565v0.015h16.989c-0.091-0.256-0.149-0.526-0.149-0.813";
  ptr += "C19.405,39.407,19.518,39.019,19.704,38.674z'fill=#955BA5 /></g></svg>";
  ptr += "</div>";
  ptr += "<div class='side-by-side text'>Altitude</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += (int)altitude;
  ptr += "<span class='superscript'>m</span></div>";
  ptr +="<button onclick='toggleLED()'>Toggle LED</button>";
  ptr +="</div>";
  ptr += "</div>";

  ptr +="<form id=\"dataForm\">";
  ptr +="<label for=\"data\">Wpisz dane:</label>";
  ptr +="<input type=\"text\" id=\"data\" name=\"data\">"; // This is the text input field
  ptr +="<button type=\"button\" class=\"button\" onclick=\"sendData()\">Send Code</button>";
  ptr += "</form>";
  ptr +="</div>";
 
  
ptr +="<script>";
ptr +="function toggleLED() {";
ptr +="var xhttp = new XMLHttpRequest();";
ptr +="xhttp.open('GET', '/toggle', true);";
ptr +="xhttp.send();";
ptr +="}";

ptr +="function sendData() {";
ptr +="var data = document.getElementById(\"data\").value;"; // Get the value from the text input field

// Send data to ESP32 using XMLHttpRequest
ptr +="var xhttp = new XMLHttpRequest();";
ptr +="xhttp.onreadystatechange = function() {";
ptr +="if (this.readyState === XMLHttpRequest.DONE && this.status === 200) {";
ptr +="}";
ptr +="};";
ptr +="xhttp.open(\"POST\", \"/sendData\", true);";
ptr +="xhttp.setRequestHeader(\"Content-type\", \"application/x-www-form-urlencoded\");";
ptr +="xhttp.send(\"data=\" + data);"; // Send the data as a parameter named "data"
ptr +="}";

ptr +="setInterval(function() {";
ptr +="window.location.reload();";
ptr +="}, 10000);"; // Refresh the page every 10 seconds
ptr +="</script>";
ptr +="</body>";
ptr +="</html>";
return ptr;
}
  
