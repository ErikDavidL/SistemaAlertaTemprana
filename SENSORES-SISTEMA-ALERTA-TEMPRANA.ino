#include "WiFiEsp.h"
#include "secrets.h"
#include "ThingSpeak.h" // always include thingspeak header file after other header files and custom macros

// Librerias I2C para controlar el mpu6050
// la libreria MPU6050.h necesita I2Cdev.h, I2Cdev.h necesita Wire.h
#include "I2Cdev.h"
#include "MPU6050.h"
#include "Wire.h"

char ssid[] = SECRET_SSID;   // your network SSID (name) 
char pass[] = SECRET_PASS;   // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)
WiFiEspClient  client;

// La dirección del MPU6050 puede ser 0x68 o 0x69, dependiendo 
// del estado de AD0. Si no se especifica, 0x68 estará implicito
MPU6050 sensor;
// Valores RAW (sin procesar) del acelerometro y giroscopio en los ejes x,y,z
int ax, ay, az;
int gx, gy, gz;


// Emulate Serial1 on pins 6/7 if not present
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(6, 7); // RX, TX
#define ESP_BAUDRATE  9600
#else
#define ESP_BAUDRATE  115200
#endif

unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;

String myStatus = "";

void setup() {
  
  //Initialize serial and wait for port to open
  Serial.begin(115200);  // Initialize serial
  Wire.begin();           //Iniciando I2C  
  sensor.initialize();    //Iniciando el sensor
  while(!Serial){
    ; // wait for serial port to connect. Needed for Leonardo native USB port only
  }
  
  // initialize serial for ESP module  
  setEspBaudRate(ESP_BAUDRATE);
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo native USB port only
  }
  
  //aqui esperamos a que se detecte el modulo MPU6050
  if (sensor.testConnection()) Serial.println("Sensor iniciado correctamente");
  else Serial.println("Error al iniciar el sensor");
  
  Serial.print("Searching for ESP8266..."); 
  // initialize ESP module
  WiFi.init(&Serial1);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }
  Serial.println("found it!");
   
  ThingSpeak.begin(client);  // Initialize ThingSpeak
}

void loop() {

  // Connect or reconnect to WiFi
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);     
    } 
    Serial.println("\nConnected.");
  }

  //leer los valores originales de los sensores
  int valorHumedad = analogRead(A0);
  int valorLluvia = analogRead(A1);

  //valores de los sensores para exportar a ThingSpeak
  int sensorHumedad = map(valorHumedad,1023,0,0,100);
  int sensorLluvia = map(valorLluvia,1023,0,0,100);
  
  // Leer las aceleraciones y velocidades angulares
  sensor.getAcceleration(&ax, &ay, &az);
  sensor.getRotation(&gx, &gy, &gz);
  float ax_m_s2 = ax * (9.81/16384.0);
  float ay_m_s2 = ay * (9.81/16384.0);
  float az_m_s2 = az * (9.81/16384.0);
  float gx_deg_s = gx * (250.0/32768.0);
  float gy_deg_s = gy * (250.0/32768.0);
  float gz_deg_s = gz * (250.0/32768.0);

  //Calcular los angulos de inclinacion:
  float accel_ang_x=atan(ax/sqrt(pow(ay,2) + pow(az,2)))*(180.0/3.14);
  float accel_ang_y=atan(ay/sqrt(pow(ax,2) + pow(az,2)))*(180.0/3.14);
  
  // enviar los valores de los sensores a los field del canal de ThingSpeak
  ThingSpeak.setField(1, sensorHumedad);
  ThingSpeak.setField(2, sensorLluvia); 
  ThingSpeak.setField(3, ax_m_s2);
  ThingSpeak.setField(4, ay_m_s2);
  ThingSpeak.setField(5, az_m_s2);
  ThingSpeak.setField(6, accel_ang_x);
  ThingSpeak.setField(7, accel_ang_y);
  
  // write to the ThingSpeak channel
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if(x == 200){
    Serial.println("Channel update successful.");
  }
  else{
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
  
  
  delay(15000); // Wait 20 seconds to update the channel again
}


// This function attempts to set the ESP8266 baudrate. Boards with additional hardware serial ports
// can use 115200, otherwise software serial is limited to 19200.

void setEspBaudRate(unsigned long baudrate){
  long rates[6] = {115200,74880,57600,38400,19200,9600};

  Serial.print("Setting ESP8266 baudrate to ");
  Serial.print(baudrate);
  Serial.println("...");

  for(int i = 0; i < 6; i++){
    Serial1.begin(rates[i]);
    delay(100);
    Serial1.print("AT+UART_DEF=");
    Serial1.print(baudrate);
    Serial1.print(",8,1,0,0\r\n");
    delay(100);  
  }
    
  Serial1.begin(baudrate);
}
