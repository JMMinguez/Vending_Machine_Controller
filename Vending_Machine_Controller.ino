#include <LiquidCrystal.h>
#include <Thread.h>
#include <ThreadController.h>
#include <StaticThreadController.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include "TimerOne.h"

#define DHTPIN 12
#define DHTTYPE DHT11

// Definition of the pins and variables for the LED and BUTTON
int LED1 = A2;
int LED2 = 11;
int led_intensity;
int BUTTON = 10;

// Initialization of the LCD
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

// Definition of the pins and variables for the DHT sensor
DHT dht(DHTPIN, DHTTYPE);
int humidity;
int temperature;
bool show_temp = true;

// Definition of the pins and variables for the joystick
int joyButton = 13;
int pinX = A0;
int pinY = A1;
int joystickX;
int joystickY;

// Configuration for the ultrasonic sensor data pins nad variables
int trigPin = 9;
int echoPin = 8;
bool distance = false;
long t;
long d;

// Variables
int menu_position = 0;
float price[] = {1, 1.10, 1.25, 1.5, 2};
String cafe[] = {"Cafe solo", "Cafe cortado", "Cafe doble", "Cafe premium", "Chocolate"};
int num_random;
unsigned long time = 0;
unsigned long initial_time = 0;
volatile bool check_admin = false;

// Creation of threads and controllers
Thread check_distanceThread = Thread();
Thread clockThread = Thread();
Thread show_distanceThread = Thread();
Thread measure_tempThread = Thread();
Thread measure_humThread = Thread();
ThreadController controller = ThreadController();

void callback_check_distance() {
  // Send a 10us pulse
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Calculate the distance in cm
  t = pulseIn(echoPin, HIGH);
  d = t/59;
  lcd.clear();
  // If someone is < 1m from the sensor, distance = true
  if (d < 100){
    distance = true;
  }
  else{
    distance = false;
  }

  return distance;
}

void callback_clock(){
  initial_time = millis();
  lcd.clear();
  lcd.print(initial_time);
}

void callback_measure_distance(){
  // Send a 10us pulse
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Calculate the distance in cm
  t = pulseIn(echoPin, HIGH);
  d = t/59;

  // Print on the LCD the actual disntance
  lcd.clear();
  lcd.print(d);
  lcd.print(" cm");
}


void callback_measure_temp() {
  // Lógica para medir la temperatura aquí
  temperature = dht.readTemperature();
  lcd.clear();
  lcd.print("T: ");
  lcd.print(temperature);
  lcd.print("C");
  delay(1000);
}
void callback_measure_hum() {
  // Lógica para medir la humedad aquí
  humidity = dht.readHumidity();
  lcd.clear();
  lcd.print("H: ");
  lcd.print(humidity);
  lcd.print("%");
}

void button_pressed(){
  static unsigned long tiempoInicio = 0;
  tiempoInicio = millis(); 
  while (digitalRead(BUTTON) == LOW){
      Serial.println(digitalRead(BUTTON));
  }
  unsigned long tiempoPresionado = millis() - tiempoInicio; 
  tiempoInicio = 0;

  if (tiempoPresionado > 2000 && tiempoPresionado < 3000) {
    reset();
  }
  else if (tiempoPresionado > 5000) {
    if (check_admin == false) {
      check_admin = true;
      admin();
    }
  
    else {
      check_admin = false;
    }
  }
}

void setup() {
  Serial.begin(9600);

  // Set LED as OUTPUT
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  // LCD screen initialization (16 colums & 2 rows)
  lcd.begin(16, 2);

  // DHT sensor initialization
  dht.begin();

  // Set BUTTONS as INPUT with pull-up resistance
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(joyButton, INPUT_PULLUP);

  // Ultrasonic sensor configuration
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(trigPin, LOW);

  // check_distance Thread config
  check_distanceThread.enabled = true;
  check_distanceThread.setInterval(2500);
  check_distanceThread.onRun(callback_check_distance);
  // clock Thread config
  clockThread.enabled = true;
  clockThread.setInterval(200);
  clockThread.onRun(callback_clock);
  // show_distance Thread config
  show_distanceThread.enabled = true;
  show_distanceThread.setInterval(200);
  show_distanceThread.onRun(callback_measure_distance);
  // measure_temp Thread config
  measure_tempThread.enabled = true;
  measure_tempThread.setInterval(1000);
  measure_tempThread.onRun(callback_measure_temp);
  // measure_hum Thread config
  measure_humThread.enabled = true;
  measure_humThread.setInterval(1000);
  measure_humThread.onRun(callback_measure_hum);
  // adding measure_temp Thread and measure_hum Thread to the controller
  controller.add(&measure_tempThread);
  controller.add(&measure_humThread);
  
  initial_time = millis();
  attachInterrupt(digitalPinToInterrupt(BUTTON), button_pressed, CHANGE);
  boot();
}

void loop() {
  // Execute the disntance thread
  if(check_distanceThread.shouldRun()){
    check_distanceThread.run();
  }
  
  if (distance == true){
    // Read the joystick on the Y axis
    joystickY = analogRead(pinY);
  
    // Depending on the position of the joystick on the Y axis, goes up or down in the menu
    if (joystickY > 600) {
      lcd.clear();
      menu_position = menu_position + 1;
      delay (500);
    }
    else if (joystickY < 400){
      lcd.clear();
      menu_position = menu_position - 1;
      delay (500);
    }

    // If the button is pressed, goes to the serve function
    if (digitalRead(joyButton) == LOW) {
      serve();
    }

    menu();
  }
  else{
    lcd.setCursor(0, 0); 
    lcd.print("Esperando");
    lcd.setCursor(0, 1);
    lcd.print("cliente...");
  }
}

void boot() {
  // Cleans and shows the text in the LCD
  lcd.clear(); 
  lcd.print("CARGANDO ...");

  int i = 0;
  while (i < 3) {
    i = i + 1;
    
    // Flashing led, on->off in 1 sec
    digitalWrite(LED1, HIGH);
    delay(500);
    digitalWrite(LED1, LOW);
    delay(500);
  }
}

void reset(){
  loop();
}

void temphum() {
  // Measure temperature
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  time = millis();

  // Shows the humidity in the upper row of the LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("H: ");
  lcd.print(humidity);
  lcd.print("%");

  // Shows the temperature in the lower row of the LCD
  lcd.setCursor(0, 1);
  lcd.print("T: ");
  lcd.print(temperature);
  lcd.print("C");

  // Wait 5 sec and clean the LCD
  while (millis() < (time + 5000)) {}
  lcd.clear();
}

void menu(){
  // Shows the temp only when is required
  if (show_temp == true) {
    temphum();
    show_temp = false;
  }

  if (menu_position > 4){
    menu_position = 0;
  }
  else if (menu_position < 0){
    menu_position = 4;
  }

  // Depending on the position in the menu, it will show one or other product
  lcd.setCursor(0, 0);
  lcd.print(cafe[menu_position]);
  lcd.setCursor(0, 1);
  lcd.print(price[menu_position]);
}

void serve(){
  // Cleans and shows the text in the LCD
  lcd.clear();
  lcd.print("Preparando Cafe &");

  // Choose a random number between 4 and 8, and adjust the increment depending on the number chosen
  num_random = random(4,8);
  led_intensity = 255 / (num_random-1);
  
  // Loop that lasts the random number in seconds
  int t = 0;
  while (t < num_random){
    // Increases the intensity value of LED 2 depending on time (t=1, led_intensity= 0(min) & t=num_random, led_intensity = 255(max))
    analogWrite(LED2, led_intensity * t);  

    // Wait 1 sec
    delay(1000);
    t = t + 1;
  }

  // Cleans and shows the text in the LCD, wait 3 sec and turn off the LED
  lcd.clear();
  lcd.print("RETIRAR BEBIDA");
  
  // Wait 3 sec and turn off the LED
  time = millis();
  while (millis() < (time + 3000)) {}
  analogWrite(LED2, 0);

  show_temp = true;
}

void admin(){
  // Turn on both LEDs
  analogWrite(LED2, 255);
  digitalWrite(LED1, HIGH);

  while (true){
    // Read the joystick on the Y axis
    joystickY = analogRead(pinY);
  
    // Depending on the position of the joystick on the Y axis, goes up or down in the menu
    if (joystickY > 600) {
      lcd.clear();
      menu_position = menu_position + 1;
      delay (500);
    }
    else if (joystickY < 400){
      lcd.clear();
      menu_position = menu_position - 1;
      delay (500);
    }

    // If the button is pressed, depending on the position, goes to the corresponding function 
    if (digitalRead(joyButton) == LOW) {
      switch (menu_position){
        case 0:
          admin_temp();
          break;

        case 1:
          show_distance();
          break;

        case 2:
          show_count();
          break;

        case 3:
          mod_price();
          break;

        default:
          menu_position = (menu_position > 3) ? 0 : (menu_position < 0) ? 3 : menu_position;
          break;
      }
    }

    admin_menu();
  }
}

void admin_menu(){
  // Depending on the position in the menu, it will show one or other product
  switch (menu_position){
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("Ver");
      lcd.setCursor(0, 1);
      lcd.print("temperatura");
      break;
  
    case 1:
      lcd.setCursor(0, 0); 
      lcd.print("Ver distancia");
      lcd.setCursor(0, 1);
      lcd.print("sensor");
      break;

    case 2:
      lcd.setCursor(0, 0); 
      lcd.print("Ver");
      lcd.setCursor(0, 1);
      lcd.print("contador");
      break;
  
    case 3:
      lcd.setCursor(0, 0); 
      lcd.print("Modificar");
      lcd.setCursor(0, 1);
      lcd.print("precios");
      break;

    // If it reaches the limit of the menu, it returns to the beginning or end
    default:
      menu_position = (menu_position > 3) ? 0 : (menu_position < 0) ? 3 : menu_position;
      break;
  }
}

void admin_temp(){
  while (analogRead(pinX) > 400){
    controller.run();
  }
}

void show_distance(){
  while (analogRead(pinX) > 400){
    if(show_distanceThread.shouldRun()){
      show_distanceThread.run();
    }
  }
}

void show_count(){
  while (analogRead(pinX) > 400){
    if(clockThread.shouldRun()){
      clockThread.run();
    }
  }
}

void mod_price(){
  while (analogRead(pinX) > 400){
    if (analogRead(pinX) > 600){
      menu_position = menu_position + 1;
    }

    // Depending on the position in the menu, it will show one or other product
    lcd.setCursor(0, 0);
    lcd.print(cafe[menu_position]);
    lcd.setCursor(0, 1);
    lcd.print(price[menu_position]);
    // Modify the price
    if (analogRead(pinY) > 600){
      price[0] = price[0] + 0.05;
      delay(500);
    }
    else if (analogRead(pinY) < 400){
      price[0] = price[0] - 0.05;
      if (price[0] < 0){
        price[0] = 0;
      }
      delay(500);
    }
        
  }
  return price[0], price[1], price[2], price[3], price[4];
}