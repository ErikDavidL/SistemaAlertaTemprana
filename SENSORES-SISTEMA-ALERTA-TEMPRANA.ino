//Librerias necesarias para el funcionamiento de las caracteristicas del módulo Wifi y ThingSpeak
//Estas librerias nos permiten acceder a las funcionalidades de ThingSpeak
#include "WiFiEsp.h"
#include "secrets.h"
#include "ThingSpeak.h" 

// Librerias I2C para controlar el MPU6050
// La libreria MPU6050.h necesita de la libreria I2Cdev.h, I2Cdev.h necesita Wire.h
#include "I2Cdev.h"
#include "MPU6050.h"
#include "Wire.h"

char ssid[] = SECRET_SSID;   // variable que almacena el nombre de la red Wifi que se usara
char pass[] = SECRET_PASS;   // variable que almacena la contraseña de la red Wifi
int keyIndex = 0;            // número de índice de la clave de red (necesario solo para WEP)
WiFiEspClient  client;       // se llama al objeto Wifi para el módulo ESP8266-01

// La dirección del MPU6050 puede ser 0x68 o 0x69, dependiendo 
// del estado de AD0. Si no se especifica, 0x68 estará implicito
MPU6050 sensor;

// Valores RAW (sin procesar) del acelerometro y giroscopio en los ejes x,y,z
int ax, ay, az;
int gx, gy, gz;

//Emular el puerto Serial1 con SoftwareSerial en los pines 6/7 si no se detecta el puerto
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(6, 7); // Valores de (RX, TX) para el modulo Wifi

//Definir el baudrate al que trabajara el arduino
#define ESP_BAUDRATE  9600
#else
#define ESP_BAUDRATE  115200
#endif

unsigned long myChannelNumber = SECRET_CH_ID;     //Variable para el ID del canal
const char * myWriteAPIKey = SECRET_WRITE_APIKEY; //Variable para la Write API Key del canal


void setup() {
  
  //Inicializamos el puerto serial y esperamos a que este se abra
  Serial.begin(115200);  // Inicializamos el serial
  Wire.begin();           //Iniciando I2C  
  sensor.initialize();    //Iniciando el sensor
  while(!Serial){
    ; // Esperando por la conexion del puerto serial
  }
  
  setEspBaudRate(ESP_BAUDRATE); //Inicializamos el modulo ESP8266
  while (!Serial) {
    ; // Esperamos por la conexion del puerto serial
  }
  
  //Aqui esperamos a que se detecte el modulo MPU6050
  if (sensor.testConnection()) Serial.println("Sensor iniciado correctamente");
  else Serial.println("Error al iniciar el sensor");
  
  Serial.print("Buscando el modulo ESP8266...");
  // initialize ESP module
  WiFi.init(&Serial1);

  // Comprobando la presencia de algun shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield no se encuentra");
    // ya no continua la comprobacion
    while (true);
  }
  Serial.println("Encontrado!");
   
  ThingSpeak.begin(client);  // Inicializamos ThingSpeak
}

void loop() {

  // Conectar o reconectar el modulo wifi
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Intentando conectar con la SSID: ");
    Serial.println(SECRET_SSID);
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, pass);  // Conectado con la red WPA/WPA2
      Serial.print(".");
      delay(5000);     
    } 
    Serial.println("\nConectado.");
  }

  //Leer los valores originales de los sensores
  int valorHumedad = analogRead(A0);
  int valorLluvia = analogRead(A1);

  //Valores de los sensores para exportar a ThingSpeak
  int sensorHumedad = map(valorHumedad,1023,0,0,100); //se redimensionan los valores de 0-1023 a  0-100
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
  
  // Enviar los valores de los sensores a los field del canal de ThingSpeak
  ThingSpeak.setField(1, sensorHumedad);
  ThingSpeak.setField(2, sensorLluvia); 
  ThingSpeak.setField(3, ax_m_s2);
  ThingSpeak.setField(4, ay_m_s2);
  ThingSpeak.setField(5, az_m_s2);
  ThingSpeak.setField(6, accel_ang_x);
  ThingSpeak.setField(7, accel_ang_y);
  
  //Escribir en el canal de ThingSpeak
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if(x == 200){
    Serial.println("Canal actualizado satisfactoriamente.");
  }
  else{
    Serial.println("Error actualizando el canal. HTTP error code " + String(x));
  }
  
  delay(15000); //Esperamos 15 segundos para que el canal se vuelva a actualizar
}

//Esta función intenta configurar la velocidad en baudios del ESP8266. 
//Placas con puertos serie de hardware adicionales.
//Se puede usar 115200; de lo contrario,está limitada a 19200.
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
