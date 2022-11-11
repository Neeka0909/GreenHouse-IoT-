
#include <MQUnifiedsensor.h> //MQ135 Sensor Library
#include "DHT.h" // DHT22 Sensor Library
#include <Adafruit_NeoPixel.h> // Neopixel Library
#include <BH1750.h>
#include <Wire.h> //i2c communication library
#include <WiFi.h> // Wifi Library
#include <AsyncTCP.h> //Async web server TCP library
#include <ESPAsyncWebServer.h> //Async web server library

// Replace with our wifi network credentials
const char* ssid = "NCV_FIBER";
const char* password = "n4jq6ule3sz";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");


unsigned long lastTime = 0;  
unsigned long timerDelay = 30000;  // send readings timer


//MQ135 Sensor Init
#define placa "ESP-32"
#define Voltage_Resolution 3.3
#define pin 35
#define type "MQ-135"
#define ADC_Bit_Resolution 12 
#define RatioMQ135CleanAir 3.6
double co2 = (0);  
MQUnifiedsensor MQ135(placa, Voltage_Resolution, ADC_Bit_Resolution, pin, type);

//DHT22 Sensor data Define
#define DHTPIN 33   // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);



//Neopixel Data Define
#define PIN 32 //Conneceted pin
#define NUMPIXELS 12 //Number of LEDs
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);//LED Init


//BH1750 Sensor define

//define couston i2c pins
#define I2C_SDA 22 //SDA pin
#define I2C_SCL 21 // SCL pin
TwoWire I2CBME = TwoWire(0);
BH1750 lightMeter; //Define BH1750 Sensor as lightmeter
float lux;

//Water Moter and fans pins define
#define moter1 17 //Moter 1
#define moter2 18 //Moter 2
#define fan 13 // fan
#define mistMaker 26 // Mist Maker

//Soil Moisture Sensor
#define p1 27 // Plant 1 moisture Sensor
#define p2 25 // Plant 1 moisture Sensor

float h;
float t;
float f;


void DHT22Readings (){
  //DHT22 Sensor Reading Init
    dht.begin();
  //DHT22 Sensor getting Readings
    // Read Humidity
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);
    
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
       Serial.println(F("Failed to read from DHT sensor!"));
       return;
    }
    
    // Compute heat index in Fahrenheit (the default)
    float hif = dht.computeHeatIndex(f, h);
    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(t, h, false);

    // Serial monitor to debug the DHT22 sensor Readings 
      Serial.print(F("Humidity: "));
      Serial.print(h);
      Serial.print(F("%  Temperature: "));
      Serial.print(t);
      Serial.print(F("°C "));
      Serial.print(f);
      Serial.print(F("°F  Heat index: "));
      Serial.print(hic);
      Serial.print(F("°C "));
      Serial.print(hif);
      Serial.println(F("°F"));
}

String processor(const String& var){
  DHT22Readings();
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return String(t);
  }
  else if(var == "HUMIDITY"){
    return String(h);
  }
  else if(var == "CO2"){
    return String(co2);
  }
 else if(var == "LIGHT"){
    return String(lux);
  }
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p {  font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #4B1D3F; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .card.temperature { color: #0e7c7b; }
    .card.humidity { color: #17bebb; }
    .card.pressure { color: #3fca6b; }
    .card.gas { color: #d62246; }
  </style>
</head>
<body>
  <div class="topnav">
    <h3>Automated Green House</h3>
  </div>
<div class="content">
    <div class="cards">
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> TEMPERATURE</h4><p><span class="reading"><span id="temp">%TEMPERATURE%</span> &deg;C</span></p>
      </div>
      <div class="card humidity">
        <h4><i class="fas fa-tint"></i> HUMIDITY</h4><p><span class="reading"><span id="hum">%HUMIDITY%</span> &percnt;</span></p>
      </div>
      <div class="card pressure">
        <h4><i class="fas fa-angle-double-down"></i> PRESSURE</h4><p><span class="reading"><span id="pres">%CO2%</span> PPM</span></p>
      </div>
      <div class="card gas">
        <h4><i class="fas fa-wind"></i> GAS</h4><p><span class="reading"><span id="light">%LIGHT%</span> Lux</span></p>
      </div>
    </div>
  </div>
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);
 
 source.addEventListener('temperature', function(e) {
  console.log("temperature", e.data);
  document.getElementById("temp").innerHTML = e.data;
 }, false);
 
 source.addEventListener('humidity', function(e) {
  console.log("humidity", e.data);
  document.getElementById("hum").innerHTML = e.data;
 }, false);
 
 source.addEventListener('pressure', function(e) {
  console.log("pressure", e.data);
  document.getElementById("pres").innerHTML = e.data;
 }, false);
 
 source.addEventListener('gas', function(e) {
  console.log("gas", e.data);
  document.getElementById("gas").innerHTML = e.data;
 }, false);
}
</script>
</body>
</html>)rawliteral";

void setup(){ 
    
    Serial.begin(115200);   // Serial Monitor is enable to help debug

      // Set the device as a Station and Soft Access Point simultaneously
    WiFi.mode(WIFI_AP_STA);
    
    // Set device as a Wi-Fi Station
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Setting as a Wi-Fi Station..");
    }
    Serial.print("Station IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println();


    //MQ135 sensor
    //Set math model to calculate the PPM concentration and the value of constants   
    MQ135.setRegressionMethod(1); //_PPM =  a*ratio^b   
    MQ135.setA(110.47); 
    MQ135.setB(-2.862); 
    // Configurate the ecuation values to get NH4 concentration    
    MQ135.init();    
    Serial.print("Calibrating please wait.");   
    float calcR0 = 0;   
    
    for(int i = 1; i<=10; i ++)   {     
        MQ135.update(); // Update data, the arduino will be read the voltage on the analog pin     
        calcR0 += MQ135.calibrate(RatioMQ135CleanAir);    
        Serial.print(".");   
    }   
    MQ135.setR0(calcR0/10);   
    Serial.println("  done!.");      
    if(isinf(calcR0)) { Serial.println("Warning: Conection issue founded, R0 is infite (Open circuit detected) please check your wiring and supply"); while(1);}   
    if(calcR0 == 0){Serial.println("Warning: Conection issue founded, R0 is zero (Analog pin with short circuit to ground) please check your wiring and supply"); while(1);}   
    /*****************************  MQ CAlibration **************************/                   
    MQ135.serialDebug(false);

    
    
    
    // INITIALIZE NeoPixel strip object 
    pixels.begin();
    pixels.setBrightness(150); // Set BRIGHTNESS to about 1/5 (max = 255)

    //BH1750 Sensor init
    I2CBME.begin(I2C_SDA, I2C_SCL, 100000); //Custom i2c pin init
    lightMeter.begin(); //BH1750 Sensor init

    //Pinmode Selection
    pinMode(moter1,OUTPUT);
    pinMode(moter2,OUTPUT);
    pinMode(fan,OUTPUT);
    pinMode(mistMaker,OUTPUT);
    pinMode(p1,INPUT);
    pinMode(p2,INPUT);

    // Handle Web Server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Handle Web Server Events
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.begin();
}

void loop(){
    //MQ135 Sensor getting Readings   
    MQ135.update(); // Update data, the arduino will be read the voltage on the analog pin   
    co2 = MQ135.readSensor(); // Sensor will read CO2 concentration using the model and a and b values setted before or in the setup   
    Serial.print("CO2: ");   
    Serial.println(co2);    

    

      // BH1750 Readings 
       lux = lightMeter.readLightLevel(); //Lux value
        Serial.print("Light: ");
        Serial.print(lux);
        Serial.println(" lx");

      analogReadResolution(10); // activate 10bit ADC
       int pw1 = analogRead(p1);
       int pw2 = analogRead(p2);
       while(pw1<500){
        //digitalWrite(moter1,HIGH);
        pw1 = analogRead(p1);
        //Serial.println("M1 is on");
        
       }
       while(pw2<500){
        //digitalWrite(moter2,HIGH);
        pw2 = analogRead(p2);
        //Serial.println("M2 is on");
       }
      DHT22Readings ();
        
     //LEDs Colour Setup Green/Red/Blue i=LED number
     // Plant1  255, 239, 247
     int Green1 = 239;
     int Red1 = 255;
     int Blue1 = 247;
     pixels.setPixelColor(0, pixels.Color(Red1, Green1, Blue1));
     pixels.setPixelColor(1, pixels.Color(Red1, Green1, Blue1));
     pixels.setPixelColor(2, pixels.Color(Red1, Green1, Blue1));
     pixels.setPixelColor(3, pixels.Color(Red1, Green1, Blue1));
     pixels.setPixelColor(4, pixels.Color(Red1, Green1, Blue1));
     pixels.setPixelColor(5, pixels.Color(Red1, Green1, Blue1));
     pixels.show();   // Send the updated pixel colors to the hardware.
     
     for(int i=0; i<6; i++){
        pixels.setPixelColor(i, pixels.Color(255, 0, 0));
        pixels.setPixelColor(i++, pixels.Color(0, 0, 247));
        pixels.show(); 
        delay(100);
     }
      // Plant2  255, 239, 247
     int Green2 = 239;
     int Red2 = 255;
     int Blue2 = 247;
     pixels.setPixelColor(6, pixels.Color(Red2, Green2, Blue2));
     pixels.setPixelColor(7, pixels.Color(Red2, Green2, Blue2));
     pixels.setPixelColor(8, pixels.Color(Red2, Green2, Blue2));
     pixels.setPixelColor(9, pixels.Color(Red2, Green2, Blue2));
     pixels.setPixelColor(10, pixels.Color(Red2, Green2, Blue2));
     pixels.setPixelColor(11, pixels.Color(Red2, Green2, Blue2));
     pixels.show();   // Send the updated pixel colors to the hardware.
     
     for(int i=6; i<12; i++){
        pixels.setPixelColor(i, pixels.Color(255, 0, 0));
        pixels.setPixelColor(i++, pixels.Color(0, 0, 247));
        pixels.show(); 
        delay(100);
      }
  

  events.send("ping",NULL,millis());
    events.send(String(t).c_str(),"temperature",millis());
    events.send(String(h).c_str(),"humidity",millis());
    events.send(String(co2).c_str(),"co2",millis());
    events.send(String(lux).c_str(),"light",millis());
    
    lastTime = millis();
}
