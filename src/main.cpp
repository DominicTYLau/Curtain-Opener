#include <Arduino.h>
#include <WiFi.h>
#include <AccelStepper.h>

// Define Constants

// Define step constants
#define FULLSTEP 4
#define HALFSTEP 8

// Define Motor Pins (2 Motors used)

#define motorPin5 13 // Blue   - 28BYJ48 pin 1
#define motorPin6 12 // Pink   - 28BYJ48 pin 2
#define motorPin7 14 // Yellow - 28BYJ48 pin 3
#define motorPin8 27 // Orange - 28BYJ48 pin 4

// Define two motor objects
// The sequence 1-3-2-4 is required for proper sequencing of 28BYJ48
AccelStepper stepper2(FULLSTEP, motorPin5, motorPin7, motorPin6, motorPin8);

int totalSteps; // Determine the length of the curtain

const char *ssid = "ORBI47";
const char *password = "elatedsea489";

WiFiServer server(80);

bool turnMotor = false;
SemaphoreHandle_t turnMotorMutex;

TaskHandle_t Task1;
TaskHandle_t Task2;

void Task1code(void *pvParameters)

{

  for (;;)
  {

    WiFiClient client = server.available(); // listen for incoming clients

    if (client)
    {                                // if you get a client,
      Serial.println("New Client."); // print a message out the serial port
      String currentLine = "";       // make a String to hold incoming data from the client
      while (client.connected())
      { // loop while the client's connected
        if (client.available())
        {                         // if there's bytes to read from the client,
          char c = client.read(); // read a byte, then
          Serial.write(c);        // print it out the serial monitor
          if (c == '\n')
          { // if the byte is a newline character

            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0)
            {
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();

              // the content of the HTTP response follows the header:
              client.print("Click <a href=\"/H\">here</a> to turn the LED on pin 5 on.<br>");
              client.print("Click <a href=\"/L\">here</a> to turn the LED on pin 5 off.<br>");



              // The HTTP response ends with another blank line:
              client.println();
              // break out of the while loop:
              break;
            }
            else
            { // if you got a newline, then clear currentLine:
              currentLine = "";
            }
          }
          else if (c != '\r')
          {                   // if you got anything else but a carriage return character,
            currentLine += c; // add it to the end of the currentLine
          }

          // Check to see if the client request was "GET /H" or "GET /L":
          if (currentLine.endsWith("GET /H"))
          {
            turnMotor = true;
            Serial.println(turnMotor);
          }
          if (currentLine.endsWith("GET /L"))
          {
            turnMotor = false;
          }
        }
      }
      // close the connection:
      client.stop();
      Serial.println("Client Disconnected.");
    }
    vTaskDelay(pdMS_TO_TICKS(10)); // Allow other tasks to run
  }
}

void Task2code(void *pvParameters)
{
  for (;;)
  {
    if (turnMotor)
    {
      if (stepper2.distanceToGo() == 0){
        stepper2.setCurrentPosition(0);
        stepper2.moveTo(2048);
        stepper2.setSpeed(500);
        totalSteps += 2048;
      }
      stepper2.run();
    }
    else
    {
      stepper2.stop();
      totalSteps += stepper2.currentPosition();
      stepper2.setCurrentPosition(0);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(10);

  // Set up Stepper motor

  stepper2.setCurrentPosition(0);
  stepper2.setMaxSpeed(900);
  stepper2.setAcceleration(100);
  stepper2.moveTo(2048);
  stepper2.setSpeed(500);

  // Set up wifi

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();

  // Initialize semaphore
  turnMotorMutex = xSemaphoreCreateMutex();

  // Create tasks
  xTaskCreatePinnedToCore(
      Task1code, "Task1", 10000, NULL, 1, &Task1, 0);
  xTaskCreatePinnedToCore(
      Task2code, "Task2", 10000, NULL, 1, &Task2, 1);
}

void loop()
{
  // Empty loop as tasks handle the program flow
}
