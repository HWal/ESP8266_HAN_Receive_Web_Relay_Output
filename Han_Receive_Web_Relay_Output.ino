//**************************************************************************
// READ MESSAGES FROM HAN PORT ON KAIFA MA304H3E 3-PHASE ELECTRICITY METER.
// THIS PROGRAM ALSO HANDLES MESSAGES FROM 1-PHASE METER OF THE SAME TYPE.
// SHOW THE RESULTS ON A WEBPAGE.
// TWO OUTPUT RELAYS CAN BE CONTROLLED MANUALLY.
// SEND MOBILE NOTIFICATION WHEN ACTIVE POWER EXCEEDS LIMIT SET BY USER.
//**************************************************************************
// NodeMCU V3 developement board:
// GPIO(MCU pin)  NodeMCU V3 pin                        Pull Up / Down
//    0           D3  Boot select - Use if it works ok      Pull Up
//    1           D10 UART0 TX Serial - Avoid it            Pull Up
//    2           D4  Boot select - Use if it works ok      Pull Up
//    3           D9  UART0 RX Serial - Avoid it            Pull Up
//    4           D2  Normally for I2C SDA                  Pull Up
//    5           D1  Normally for I2C SCL                  Pull Up
//    6           --- For SDIO Flash - Not useable          Pull Up
//    7           --- For SDIO Flash - Not useable          Pull Up
//    8           --- For SDIO Flash - Not useable          Pull Up
//    9           S2  For SDIO Flash - May not work         Pull Up
//   10           S3  For SDIO Flash - May not work         Pull Up
//   11           --- For SDIO Flash - Not useable          Pull Up
//   12           D6  Ok Use                                Pull Up
//   13           D7  Ok Use                                Pull Up
//   14           D5  Ok Use                                Pull Up
//   15           D8  Boot select - Use if it works ok      Pull Up (N/A)
//   16           D0  Wake up - May cause reset - Avoid it  Pull Down
//
// The value of the internal pull-resistors are between 30k and 100k ohms.
// A google search shows that using them is not the way to go (difficult).
//
// By running Serial.swap() UART0 is remapped to other pins, so that GPIO1
// (D10, TX) is moved to GPIO15, and GPIO3 (D9, RX) is moved to GPIO13.
//
// Hardware Serial buffer size is 128B on ESP8266-12E. One slot must always
// be empty.
//
// The 3.3V supply on the board may deliver up to 800mA (also from google).
// Maximum load on a GPIO output pin is 12mA.
//**************************************************************************
// Logic input/output levels with 3.3V power supply to ESP8266-12E:
// LOW: -0.3V -> 0.825V, HiGH: 2.475V -> 3.6V
//**************************************************************************
// BOOT SELECT ALTERNATIVES:
// GPIO15   GPIO0    GPIO2         Mode
// 0V       0V       3.3V          UART Bootloader
// 0V       3.3V     3.3V          Boot sketch (SPI flash)
// 3.3V     x        x             SDIO mode (not used for Arduino)
//**************************************************************************

#include <ESP8266WiFi.h>
// #include <WiFiClient.h> already included in ESP8266WiFi.h
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <SoftwareSerial.h>

#define BUF_SIZE 256                // Size of the array to receive characters
#define OUTPIN0    D2               // GPIO4
#define INPIN0     D1               // GPIO5
#define OUTPIN1    D6               // GPIO12
#define INPIN1     D3               // GPIO0

// WiFi details
static const char ssid[] = "Your_ssid";
static const char password[] = "Your_password";

// Create variables
String Do0 = "<font color=\"black\"><b>Off</b></font>"; // Indication startup default
int valInPin0 = 0;                  // Read value on pin D1 = GPIO5
String Do1 = "<font color=\"black\"><b>Off</b></font>"; // Indication startup default
int valInPin1 = 0;                  // Read value on pin D3 = GPIO0
String thresHold = "99999";         // Threshold String value for mobile notification
int thrValue = 99999;               // Numeric representation of the String threshold
byte buf[BUF_SIZE];                 // Array to receive message
int ndx = 0;                        // Index for the characters in receive buffer
int buf_len = 0;                    // Length of the buf[], equals to ndx + 1
long messagesOk = 0;                // Counting valid messages
long messagesFaulty = 0;            // Counting messages not valid
boolean newData = false;            // Indicate if some data has been read
String listType = " ";              // Type of the last read HAN List (see above)
String msgDate = " ";               // HAN field - all list types
String msgTime = " ";               // HAN field - all list types
String listVersion = " ";           // HAN field - list types 2 and 3 - 1&3 phase
String meterId = " ";               // HAN field - list types 2 and 3 - 1&3 phase
String meterModel = " ";            // HAN field - list types 2 and 3 - 1&3 phase
String actPowerPlus = " ";          // HAN field - all list types
String actPowerMinus = " ";         // HAN field - list types 2 and 3 - 1&3 phase
String reactPowerPlus = " ";        // HAN field - list types 2 and 3 - 1&3 phase
String reactPowerMinus = " ";       // HAN field - list types 2 and 3 - 1&3 phase
String currentL1 = " ";             // HAN field - list types 2 and 3 - 1&3 phase
String currentL2 = " ";             // HAN field - list types 2 and 3 - 3 phase
String currentL3 = " ";             // HAN field - list types 2 and 3 - 3 phase
String voltageL1 = " ";             // HAN field - list types 2 and 3 - 1&3 phase
String voltageL2 = " ";             // HAN field - list types 2 and 3 - 3 phase
String voltageL3 = " ";             // HAN field - list types 2 and 3 - 3 phase
String activeEnergyPlus = " ";      // HAN field - list type 3 - 1&3 phase
String activeEnergyMinus = " ";     // HAN field - list type 3 - 1&3 phase
String reactiveEnergyPlus = " ";    // HAN field - list type 3 - 1&3 phase
String reactiveEnergyMinus = " ";   // HAN field - list type 3 - 1&3 phase
bool debug = false;                 // Set true to display telegram length info
                                    // and HEX values on serial monitor
char * MakerIFTTT_Event = "Kaifa notification";   // Notification
char * MakerIFTTT_Key = "bZPx6sVkOF66XiyjTNu2e3"; // Notification
char * hostDomain = "maker.ifttt.com";            // Notification
const int hostPort = 80;                          // Notification

typedef struct {
  int act_pow_pos; /* OBIS Code 1.0.1.7.0.255 - Active Power + (Q1+Q4) */
} Items1;

typedef struct {
  char obis_list_version[1000]; /* OBIS Code 1.1.0.2.129.255 - OBIS List Version Identifier */
  char gs1[1000]; /* OBIS Code 0.0.96.1.0.255 - Meter-ID(GIAI GS1 - 16 digits */
  char meter_model[1000]; /* OBIS Code 0.0.96.1.7.255 - Meter Type */
  int act_pow_pos; /* Active Power + */
  int act_pow_neg; /* Actve Power - */
  int react_pow_pos; /* Reactive Power + */
  int react_pow_neg; /* Reactive Power - */
  int curr_L1; /* Current Phase L1 */
  int volt_L1; /* Voltage L1 */
} Items9;

typedef struct {
  char obis_list_version[1000]; /* OBIS Code 1.1.0.2.129.255 - OBIS List Version Identifier */
  char gs1[1000]; /* OBIS Code 0.0.96.1.0.255 - Meter-ID(GIAI GS1 - 16 digits */
  char meter_model[1000]; /* OBIS Code 0.0.96.1.7.255 - Meter Type */
  int act_pow_pos;  /* Active Power + */
  int act_pow_neg;  /* Active Power - */
  int react_pow_pos;  /* Reactive Power + */
  int react_pow_neg;  /* Reactive Power - */
  int curr_L1;  /* Current Phase L1 mA */
  int curr_L2;  /* Current Phase L2 mA */
  int curr_L3; /* Current Phase L3 mA */
  int volt_L1; /* Voltage L1 */
  int volt_L2; /* Voltage L2 */
  int volt_L3; /* Voltage L3 */
} Items13;

typedef struct {
  char obis_list_version[1000]; /* OBIS Code 1.1.0.2.129.255 - OBIS List Version Identifier */
  char gs1[1000]; /* OBIS Code 0.0.96.1.0.255 - Meter-ID(GIAI GS1 - 16 digits */
  char meter_model[1000]; /* OBIS Code 0.0.96.1.7.255 - Meter Type */
  int act_pow_pos; /* Active Power + */
  int act_pow_neg; /* Active Power - */
  int react_pow_pos; /* Reactive Power + */
  int react_pow_neg; /* Reactive Power - */
  int curr_L1;
  int volt_L1;
  char date_time[1000]; /* OBIS Code: 0.0.1.0.0.255 - Clock and Date in Meter */
  int act_energy_pos;
  int act_energy_neg;
  int react_energy_pos;
  int react_energy_neg;  
} Items14;

typedef struct {
  char obis_list_version[1000]; /* OBIS Code 1.1.0.2.129.255 - OBIS List Version Identifier */
  char gs1[1000]; /* OBIS Code 0.0.96.1.0.255 - Meter-ID(GIAI GS1 - 16 digits */
  char meter_model[1000]; /* OBIS Code 0.0.96.1.7.255 - Meter Type */
  int act_pow_pos; /* Active Power + */
  int act_pow_neg; /* Active Power - */
  int react_pow_pos; /* Reactive Power + */
  int react_pow_neg; /* Reactive Power - */
  int curr_L1; /* Current L1 */
  int curr_L2; /* Current L2 */
  int curr_L3; /* Current L3 */
  int volt_L1; /* Voltage L1 */
  int volt_L2; /* Voltage L2 */
  int volt_L3; /* Voltage L3 */
  char date_time[1000]; /* OBIS Code: 0.0.1.0.0.255 - Clock and Date in Meter */
  int act_energy_pa; /* Active Energy +A */
  int act_energy_ma; /* Active Energy -A */
  int act_energy_pr; /* Active Energy +R */
  int act_energy_mr; /* Active Energy -R */
} Items18;

typedef struct {
  int year;
  int month;
  int day;
  int hour;
  int min;
  int sec;
  char tm[100];
  int num_items;
  union {
    Items1 msg1;   /* List 1: 1-and-3-phase */
    Items9 msg9;   /* List 2: 1-phase */
    Items13 msg13; /* List 2: 3-phase */
    Items14 msg14; /* List 3: 1-phase */ 
    Items18 msg18; /* List 3: 3-phase */
  };
} HanMsg;

HanMsg msg;


// Create Objects
ESP8266WebServer server (80);
SoftwareSerial mySerial(2, 14); // RX, TX
WiFiClient client;

// Webpage function
String getPage() {
  String page = "";
  page += "<!DOCTYPE html>";
  page += "<html lang=en-EN>";
  page += "<head>";
  page += "<meta name=\"viewport\" content =\"width=device-width, initial-scale=1.0, maximum-scale=4.0, minimum-scale=.5, user-scalable=1\">";
  page += "<meta http-equiv=\"refresh\" content=\"1200\">";
  page += "<title>ESP8266 Demo</title>";
  page += "<style> body { background-color: #fffff; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }</style>";
  page += "</head>";
  page += "<body>";
  page += "<table border=\"1\" width=\"350\" cellpadding=\"2\">";
  page += "<tr><td><b>ESP8266</b></td><td align=\"right\"><b> WEBSERVER</b></td></tr>";
  page += "<tr><td>NodeMCU v3</td><td align=\"right\"> dev board</td></tr>";

  // Output 0
  page += "<tr><td><form action='/' method='POST'>";
  page += "<b>Water heater (2000W)</b>";
  page += "<br>Curr value: ";
  page += Do0;
  page += "</td>";
  page += "<td><INPUT type='radio' name='DO0' value='1'>On";
  page += "&nbsp";
  page += "<INPUT type='radio' name='DO0' value='0'>Off";
  page += "<br>";
  page += "<INPUT type='submit' value='Update / Send'>";
  page += "</form></td></tr>";

  // Output 1
  page += "<tr><td><form action='/' method='POST'>";
  page += "<b>Panel heater (1200W)</b>";
  page += "<br>Curr value: ";
  page += Do1;
  page += "</td>";
  page += "<td><INPUT type='radio' name='DO1' value='1'>On";
  page += "&nbsp";
  page += "<INPUT type='radio' name='DO1' value='0'>Off";
  page += "<br>";
  page += "<INPUT type='submit' value='Update / Send'>";
  page += "</form></td></tr>";

  // User defined threshold value for notification
  page += "<tr><td><form action='/' method='POST'>";
  page += "<b>Notification limit (W)</b>";
  page += "<br>Set value (1 - 99999): ";
  page += "<br>Current: ";
  page += thresHold;
  page += "</td>";
  page += "<td>Enter: ";
  page += "<INPUT type='number' name='KW_THR' min='0' max='99999' value='' size='5' maxlength='5'>";
  page += "<br>";
  page += "<INPUT type='submit' value='Update / Send'>";
  page += "</form></td></tr>";

  // Display values from meter
  page += "<tr><td><b>LAST READ LIST TYPE</b></td><td align=\"right\"><b><font color=\"red\">";
  page += listType;
  page += "</font></b></td></tr>";
  page += "<tr><td><b>DATE</b></td><td align=\"right\">";
  page += msgDate;
  page += "</td></tr>";
  page += "<tr><td><b>TIME</b></td><td align=\"right\">";
  page += msgTime;
  page += "</td></tr>";
  page += "<tr><td><b>LIST VERSION</b></td><td align=\"right\">";
  page += listVersion;
  page += "</td></tr>";
  page += "<tr><td><b>METER ID</b></td><td align=\"right\">";
  page += meterId;
  page += "</td></tr>";
  page += "<tr><td><b>METER MODEL</b></td><td align=\"right\">";
  page += meterModel;
  page += "</td></tr>";
  page += "<tr><td><b>ACT POWER+ (W)</b></td><td align=\"right\">";
  page += actPowerPlus;
  page += "</td></tr>";
  page += "<tr><td><b>ACT POWER- (W)</b></td><td align=\"right\">";
  page += actPowerMinus;
  page += "</td></tr>";
  page += "<tr><td><b>REA POWER+ (VAR)</b></td><td align=\"right\">";
  page += reactPowerPlus;
  page += "</td></tr>";
  page += "<tr><td><b>REA POWER- (VAR)</b></td><td align=\"right\">";
  page += reactPowerMinus;
  page += "</td></tr>";
  page += "<tr><td><b>CURR L1 (A * 1000)</b></td><td align=\"right\">";
  page += currentL1;
  page += "</td></tr>";
  page += "<tr><td><b>CURR L2 (A * 1000)</b></td><td align=\"right\">";
  page += currentL2;
  page += "</td></tr>";
  page += "<tr><td><b>CURR L3 (A * 1000)</b></td><td align=\"right\">";
  page += currentL3;
  page += "</td></tr>";
  page += "<tr><td><b>VOLT L1 (V * 10)</b></td><td align=\"right\">";
  page += voltageL1;
  page += "</td></tr>";
  page += "<tr><td><b>VOLT L2 (V * 10)</b></td><td align=\"right\">";
  page += voltageL2;
  page += "</td></tr>";
  page += "<tr><td><b>VOLT L3 (V * 10)</b></td><td align=\"right\">";
  page += voltageL3;
  page += "</td></tr>";
  page += "<tr><td><b>ACT ENERGY+ (Wh)</b></td><td align=\"right\">";
  page += activeEnergyPlus;
  page += "</td></tr>";
  page += "<tr><td><b>ACT ENERGY- (Wh)</b></td><td align=\"right\">";
  page += activeEnergyMinus;
  page += "</td></tr>";
  page += "<tr><td><b>REA ENERGY+ (VARh)</b></td><td align=\"right\">";
  page += reactiveEnergyPlus;
  page += "</td></tr>";
  page += "<tr><td><b>REA ENERGY- (VARh)</b></td><td align=\"right\">";
  page += reactiveEnergyMinus;
  page += "</td></tr>";
  page += "<tr><td><b>MSG OK</b></td><td align=\"right\">";
  page += messagesOk;
  page += "</td></tr>";
  page += "<tr><td><b>MSG FAULTY</b></td><td align=\"right\">";
  page += messagesFaulty;
  page += "</td></tr>";
  page += "</table>";
  page += "</body>";
  page += "</html>";
  return page;
}


// Check if user has submitted data from webpage
void handleRoot(){
  if (( server.hasArg("DO0")) || ( server.hasArg("DO1"))) {
    handleSubmit_1();
  }
  if ( server.hasArg("KW_THR")) {
    handleSubmit_2();
  }
  checkInputs();
  server.send (200, "text/html", getPage());
}


// Handle relay command from webpage
void handleSubmit_1() {
  // Update the GPIOs
  if ( server.arg("DO0") == "1" ) {
    digitalWrite(OUTPIN0, 1);
  }
  if ( server.arg("DO0") == "0" ) {
    digitalWrite(OUTPIN0, 0);
  }
  if ( server.arg("DO1") == "1" ) {
    digitalWrite(OUTPIN1, 1);
  }
  if ( server.arg("DO1") == "0" ) {
    digitalWrite(OUTPIN1, 0);
  }
}


/* Handle user input active power threshold. Set
value to max if a proper number is not entered.*/
void handleSubmit_2() {
  thresHold = server.arg("KW_THR");
  // check if input string is too long or too short
  if ((thresHold.length() > 5) || (thresHold.length() < 1)) {
    thresHold = "99999";
    thrValue = 99999;
    return;
  }
  thrValue = thresHold.toInt();
  // Check if threshold is a number > 0
  if (thrValue < 1) {
    thresHold = "99999";
    thrValue = 99999;
  }
}


// Check status of the input pins
void checkInputs() {
  valInPin0 = digitalRead(INPIN0);
  if (valInPin0 == 1) {
    Do0 = "<font color=\"red\"><b>On</b></font>";
  } else if (valInPin0 == 0) {
    Do0 = "<font color=\"black\"><b>Off</b></font>";
  } else {
    Do0 = "<font color=\"yellow\"><b>Err</b></font>";
    mySerial.println("Error reading pin D1");
  }
  valInPin1 = digitalRead(INPIN1);
  if (valInPin1 == 1) {
    Do1 = "<font color=\"red\"><b>On</b></font>";
  } else if (valInPin1 == 0) {
    Do1 = "<font color=\"black\"><b>Off</b></font>";
  } else {
    Do1 = "<font color=\"yellow\"><b>Err</b></font>";
    mySerial.println("Error reading pin D3");
  }
}


void setup() {
  /* UART0 (Serial) is by default 8N1. This port is used for 
  reading HAN messages. Transmission from the meter is 8E1 */
  Serial.begin(2400, SERIAL_8E1);

  // Serial.swap() remaps the default TX and RX pins
  Serial.swap();

  // mySerial is used for printing to the monitor
  mySerial.begin(57600); 

  // Assign and initialize digital output 0
  pinMode(OUTPIN0, OUTPUT);
  digitalWrite(OUTPIN0, 0);

  // Assign digital input 0
  pinMode(INPIN0, INPUT);

  // Assign and initialize digital output 1
  pinMode(OUTPIN1, OUTPUT);
  digitalWrite(OUTPIN1, 0);

  // Assign digital input 1
  pinMode(INPIN1, INPUT);

  mySerial.println(); mySerial.println(); mySerial.println();

  // ALLOW TIME TO SWITCH FROM UPLOAD PORT TO MONITOR PORT IN ARDUINO IDE
  delay(10000);

  for(uint8_t t = 4; t > 0; t--) {
    mySerial.printf("[SETUP] BOOT WAIT %d...\r\n", t);
    mySerial.flush();
    delay(1000);
  }

  WiFi.mode(WIFI_STA); // Station mode
  
  WiFi.begin (ssid, password);
  // Wait for connection
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 ); mySerial.print ( "." );
  }

  /* OTA setup code. The first three lines set values that are also
  defaults, so they can be commented out.*/
  // ArduinoOTA.setPort(8266);
  // ArduinoOTA.setHostname("myesp8266");
  // ArduinoOTA.setPassword((const char *)"123"); // No authentication
  ArduinoOTA.onStart([]() {
    mySerial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    mySerial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    mySerial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    mySerial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) mySerial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) mySerial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) mySerial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) mySerial.println("Receive Failed");
    else if (error == OTA_END_ERROR) mySerial.println("End Failed");
  });
  ArduinoOTA.begin();
  
  // When WiFi connection is OK
  mySerial.println (""); 
  mySerial.print ("Connected to "); mySerial.println (ssid);
  mySerial.print ("Server IP address: "); mySerial.println (WiFi.localIP());

  // Call handleRoot function when a client requests URI "/"
  server.on("/", handleRoot);

  server.begin();
  mySerial.println ("HTTP server started");
}


void loop() {
  ArduinoOTA.handle();
  
  // Listen for HTTP requests from clients
  server.handleClient();
  // Check if data is available in buffer and read characters
  readData();
  if (newData == true) {
    // Validate data, identify message and print HEX numbers
    validateAndPrint();
    newData = false;
  }
}


// Read hardware buffer and fill array buffer
void readData() {
  static byte tempA = 0x00;
  static byte rc;
  while ((Serial.available() > 0) && (newData == false)) {
    rc = Serial.read();
    buf[ndx] = rc;
    
    if ((rc == 0x7E) && (tempA == 0x7E)) {
      newData = true;
      buf_len = ndx + 1;
      tempA = 0x00;
      fixArray();
      return;
      
    } else {
      ndx++;
      tempA = rc;
    }
  }
}


// Move last character in array (0x7E) to front
void fixArray() {
  byte tempB = buf[ndx];
  for (int i = ndx; i > 0; i--) {
    buf[i] = buf[i - 1];
  }
  buf[0] = tempB;
}


// Reset buffer index
void Clear() {
  ndx = 0;
}


// Validate telegram and print HEX characters,
void validateAndPrint() {
  // Test some key values to verify that we have a valid message
  if ((buf[17] == 0x09)
    && ((int)((buf[1] & 7) << 8 | buf[2]) == (ndx - 1))
    && (buf[0] == 0x7E) && (buf[ndx] == 0x7E)) {

    messagesOk++;

    // Print to serial monitor
    if(debug) {
      // Print message length info
      mySerial.print("Received telegram: ");
      mySerial.print(ndx + 1);
      mySerial.print(" bytes        ");
      mySerial.print("Data frame: ");
      mySerial.print((int)((buf[1] & 7) << 8 | buf[2]));
      mySerial.println(" bytes");
      // Print the HEX numbers
      for (int i = 0; i <= ndx; i++) {
        // Print leading zero on numbers < 0xF
        if (buf[i] < 16) {
          mySerial.print("0");
        }
        mySerial.print(buf[i], HEX);
        // Print space between chars
        mySerial.print(" ");
        // Restrict print to 20 bytes per line
        if ((i == 19) || (i == 39) || (i == 59) || (i == 79) || (i == 99) || (i == 119) || (i == 139)) {
          mySerial.println();
        }
      }
      mySerial.println(); mySerial.println();
    }
    // Call parsing of message
    decodeMessage(buf, buf_len, &msg);
    updateWeb(buf, buf_len, &msg);
  } else {
    if (debug) {
      mySerial.println("Not a valid telegram.");
    }
    messagesFaulty++;
  }
  newData = false;
  Clear();
}


// Decode message from HEX telegram
int decodeMessage(unsigned char *buf, int buf_len, HanMsg *msg) {

  /* warning killer */
  // buf_len = buf_len;

  memset(msg, 0, sizeof(HanMsg)); /* clear/init data */

  msg->year = buf[19]<<8 | buf[20];
  msg->month = buf[21];
  msg->day = buf[22];
  msg->hour = buf[24];
  msg->min = buf[25];
  msg->sec = buf[26];
  sprintf(msg->tm, "%d-%02d-%02d %02d:%02d:%02d", msg->year, msg->month, msg->day, msg->hour, msg->min, msg->sec);

  // Determine number of items in telegram
  int offset = 17 + 2 + buf[18];
  if (buf[offset]== 0x02) {
    msg->num_items = buf[offset+1];
  }
  offset+=2;

  // List1 for 1 and 3 phase meters - 1 item
  if (msg->num_items == 1) { /* Num Records: 1 */
    if (buf[offset]==0x06) { /* Item 1 */
      msg->msg1.act_pow_pos = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
  }
  // List2 for 1 phase meter - 9 items
  else if (msg->num_items==9) { /* Num records: 9 */
    unsigned int num_bytes=0;
    if (buf[offset]==0x09) { /* Item 1 */
      num_bytes = buf[offset+1];
      strncpy(msg->msg9.obis_list_version, (const char *) buf+offset+2, num_bytes);
      msg->msg9.obis_list_version[num_bytes] = '\0';
    }
    offset+=2+num_bytes;
    if (buf[offset]==0x09) { /* Item 2 */
      num_bytes = buf[offset+1];
      strncpy(msg->msg9.gs1, (const char *) buf+offset+2, num_bytes);
      msg->msg9.gs1[num_bytes] = '\0';
    }
    offset+=2+num_bytes;
    if (buf[offset]==0x09) { /* Item 3 */
      num_bytes = buf[offset+1];
      strncpy(msg->msg9.meter_model, (const char *) buf+offset+2, num_bytes);
      msg->msg9.meter_model[num_bytes] = '\0';
    }
    offset+=2+num_bytes;
    if (buf[offset]==0x06) { /* Item 4 */
      msg->msg9.act_pow_pos = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 5 */
      msg->msg9.act_pow_neg = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 6 */
      msg->msg9.react_pow_pos = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 7 */
      msg->msg9.react_pow_neg = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 8 */
      msg->msg9.curr_L1 = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 9 */
      msg->msg9.volt_L1 = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
  }
  // List2 for 3 phase meter - 13 items
  else if (msg->num_items==13) { /* Num records: 13 */
    unsigned int num_bytes = 0;
    if (buf[offset]==0x09) { /* Item 1 */
      num_bytes = buf[offset+1];
      strncpy(msg->msg13.obis_list_version, (const char *) buf+offset+2, num_bytes);
      msg->msg13.obis_list_version[num_bytes] = '\0';
    }
    offset+=2+num_bytes;
    if (buf[offset]==0x09) { /* Item 2 */
      num_bytes = buf[offset+1];
      strncpy(msg->msg13.gs1, (const char *) buf+offset+2, num_bytes);
      msg->msg13.gs1[num_bytes] = '\0';
    }
    offset+=2+num_bytes;
    if (buf[offset]==0x09) { /* Item 3 */
      num_bytes = buf[offset+1];
      strncpy(msg->msg13.meter_model, (const char *) buf+offset+2, num_bytes);
      msg->msg13.meter_model[num_bytes] = '\0';
    }
    offset+=2+num_bytes;
    if (buf[offset]==0x06) { /* Item 4 */
      msg->msg13.act_pow_pos = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 5 */
      msg->msg13.act_pow_neg = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 6 */
      msg->msg13.react_pow_pos = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 7 */
      msg->msg13.react_pow_neg = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 8 */
      msg->msg13.curr_L1 = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 9 */
      msg->msg13.curr_L2 = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 10 */
       msg->msg13.curr_L3 = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 11 */
      msg->msg13.volt_L1 = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 12 */
      msg->msg13.volt_L2 = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 13 */
      msg->msg13.volt_L3 = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
  }
  // List3 for 1 phase meter - 14 items
  else if (msg->num_items==14) { /* Num records: 14 */
    unsigned int num_bytes = 0;
    if (buf[offset]==0x09) { /* Item 1 */
      num_bytes = buf[offset+1];
      strncpy(msg->msg14.obis_list_version, (const char *) buf+offset+2, num_bytes);
      msg->msg14.obis_list_version[num_bytes] = '\0';
    }
    offset+=2+num_bytes;
    if (buf[offset]==0x09) { /* Item 2 */
      num_bytes = buf[offset+1];
      strncpy(msg->msg14.gs1, (const char *) buf+offset+2, num_bytes);
      msg->msg14.gs1[num_bytes] = '\0';
    }
    offset+=2+num_bytes;
    if (buf[offset]==0x09) { /* Item 3 */
      num_bytes = buf[offset+1];
      strncpy(msg->msg14.meter_model, (const char *) buf+offset+2, num_bytes);
      msg->msg14.meter_model[num_bytes] = '\0';
    }
    offset+=2+num_bytes;
    if (buf[offset]==0x06) { /* Item 4 */
      msg->msg14.act_pow_pos = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 5 */
      msg->msg14.act_pow_neg = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 6 */
      msg->msg14.react_pow_pos = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 7 */
      msg->msg14.react_pow_neg = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 8 */
      msg->msg14.curr_L1 = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 9 */
      msg->msg14.volt_L1 = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }      
    offset+=1+4;
    if (buf[offset]==0x09) { /* Item 10 */
      num_bytes = buf[offset+1];
      int year = buf[offset+2]<<8 | buf[offset+3];
      int month = buf[offset+4];
      int day = buf[offset+5];
      int hour = buf[offset+7];
      int min = buf[offset+8];
      int sec = buf[offset+9];
      sprintf(msg->msg14.date_time, "%d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, min, sec);
    }
    offset+=2+num_bytes;
    if (buf[offset]==0x06) { /* Item 11 */
      msg->msg14.act_energy_pos = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 12 */
      msg->msg14.act_energy_neg = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }      
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 13 */
      msg->msg14.react_energy_pos = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }      
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 14 */
      msg->msg14.react_energy_neg = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }      
  }
  // List3 for 3 phase meter - 18 items
  else if (msg->num_items==18) { /* Num records: 18 */
    unsigned int num_bytes = 0;
    if (buf[offset]==0x09) { /* Item 1 */
      num_bytes = buf[offset+1];
      strncpy(msg->msg18.obis_list_version, (const char *) buf+offset+2, num_bytes);
      msg->msg18.obis_list_version[num_bytes] = '\0';
    }
    offset+=2+num_bytes;
    if (buf[offset]==0x09) { /* Item 2 */
      num_bytes = buf[offset+1];
      strncpy(msg->msg18.gs1, (const char *) buf+offset+2, num_bytes);
      msg->msg18.gs1[num_bytes] = '\0';
    }
    offset+=2+num_bytes;
    if (buf[offset]==0x09) { /* Item 3 */
      num_bytes = buf[offset+1];
      strncpy(msg->msg18.meter_model, (const char *) buf+offset+2, num_bytes);
      msg->msg18.meter_model[num_bytes] = '\0';
    }
    offset+=2+num_bytes;
    if (buf[offset]==0x06) { /* Item 4 */
      msg->msg18.act_pow_pos = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 5 */
      msg->msg18.act_pow_neg = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 6 */
      msg->msg18.react_pow_pos = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 7 */
      msg->msg18.react_pow_neg = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 8 */
      msg->msg18.curr_L1 = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 9 */
      msg->msg18.curr_L2 = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }      
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 10 */
      msg->msg18.curr_L3 = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }      
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 11 */
      msg->msg18.volt_L1 = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }      
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 12 */
      msg->msg18.volt_L2 = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }      
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 13 */
      msg->msg18.volt_L3 = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }      
    offset+=1+4;
    if (buf[offset]==0x09) { /* Item 14 */
      num_bytes = buf[offset+1];
      int year = buf[offset+2]<<8 | buf[offset+3];
      int month = buf[offset+4];
      int day = buf[offset+5];
      int hour = buf[offset+7];
      int min = buf[offset+8];
      int sec = buf[offset+9];
      sprintf(msg->msg18.date_time, "%d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, min, sec);
    }
    offset+=2+num_bytes;
    if (buf[offset]==0x06) { /* Item 15 */
      msg->msg18.act_energy_pa = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 16 */
      msg->msg18.act_energy_ma = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }      
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 17 */
      msg->msg18.act_energy_pr = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }      
    offset+=1+4;
    if (buf[offset]==0x06) { /* Item 18 */
      msg->msg18.act_energy_mr = buf[offset+1]<<24 | buf[offset+2]<<16 | buf[offset+3]<<8 | buf[offset+4];
    }      
  } else {
    return 1; /* Error - Unknown msg size */
  }
}


// Update the values which are presented on the web page as strings
int updateWeb(unsigned char *buf, int buf_len, HanMsg *msg) {

  // Format date and time fields to fit the webpage
  // Insert leading zero(s) in date/time as needed
  String a = ""; if (buf[21] < 10) {a = "0";}
  String b = ""; if (buf[22] < 10) {b = "0";}
  String c = ""; if (buf[24] < 10) {c = "0";}
  String d = ""; if (buf[25] < 10) {d = "0";}
  String e = ""; if (buf[26] < 10) {e = "0";}
  
  msgDate = String(msg->year) + "-" + a + String(msg->month) + "-" + b
  + String(msg->day);
  msgTime = c + String(msg->hour) + ":" + d + String(msg->min) + ":" + e
  + String(msg->sec);

  // Check which list type that was last read and update web fields
  switch (msg->num_items) {
    
  // List1 for 1 and 3 phase meters - 1 item
  case 1:
    listType = "List 1, 1&3-phase";
    actPowerPlus = String(msg->msg1.act_pow_pos);
    break;
    
  // List2 for 1 phase meter - 9 items
  case 9:
    listType = "List 2, 1-phase";
    listVersion = String(msg->msg9.obis_list_version);
    meterId = String(msg->msg9.gs1);
    meterModel = String(msg->msg9.meter_model);
    actPowerPlus = String(msg->msg9.act_pow_pos);
    actPowerMinus = String(msg->msg9.act_pow_neg);
    reactPowerPlus = String(msg->msg9.react_pow_pos);
    reactPowerMinus = String(msg->msg9.react_pow_neg);
    currentL1 = String(msg->msg9.curr_L1);
    voltageL1 = String(msg->msg9.volt_L1);
    break;
    
  // List2 for 3 phase meter - 13 items
  case 13:
    listType = "List 2, 3-phase";
    listVersion = String(msg->msg13.obis_list_version);
    meterId = String(msg->msg13.gs1);
    meterModel = String(msg->msg13.meter_model);
    actPowerPlus = String(msg->msg13.act_pow_pos);
    actPowerMinus = String(msg->msg13.act_pow_neg);
    reactPowerPlus = String(msg->msg13.react_pow_pos);
    reactPowerMinus = String(msg->msg13.react_pow_neg);
    currentL1 = String(msg->msg13.curr_L1);
    currentL2 = String(msg->msg13.curr_L2);
    currentL3 = String(msg->msg13.curr_L3);
    voltageL1 = String(msg->msg13.volt_L1);
    voltageL2 = String(msg->msg13.volt_L2);
    voltageL3 = String(msg->msg13.volt_L3);
    break;
    
  // List3 for 1 phase meter - 14 items
  case 14:
    listType = "List 3, 1-phase";
    listVersion = String(msg->msg14.obis_list_version);
    meterId = String(msg->msg14.gs1);
    meterModel = String(msg->msg14.meter_model);
    actPowerPlus = String(msg->msg14.act_pow_pos);
    actPowerMinus = String(msg->msg14.act_pow_neg);
    reactPowerPlus = String(msg->msg14.react_pow_pos);
    reactPowerMinus = String(msg->msg14.react_pow_neg);
    currentL1 = String(msg->msg14.curr_L1);
    voltageL1 = String(msg->msg14.volt_L1);
    // msg->msg14.date_time not used
    activeEnergyPlus = String(msg->msg14.act_energy_pos);
    activeEnergyMinus = String(msg->msg14.act_energy_neg);
    reactiveEnergyPlus = String(msg->msg14.react_energy_pos);
    reactiveEnergyMinus = String(msg->msg14.react_energy_neg);
    break;
    
  // List3 for 3 phase meter - 18 items
  case 18:
    listType = "List 3, 3-phase";
    listVersion = String(msg->msg18.obis_list_version);
    meterId = String(msg->msg18.gs1);
    meterModel = String(msg->msg18.meter_model);
    actPowerPlus = String(msg->msg18.act_pow_pos);
    actPowerMinus = String(msg->msg18.act_pow_neg);
    reactPowerPlus = String(msg->msg18.react_pow_pos);
    reactPowerMinus = String(msg->msg18.react_pow_neg);
    currentL1 = String(msg->msg18.curr_L1);
    currentL2 = String(msg->msg18.curr_L2);
    currentL3 = String(msg->msg18.curr_L3);
    voltageL1 = String(msg->msg18.volt_L1);
    voltageL2 = String(msg->msg18.volt_L2);
    voltageL3 = String(msg->msg18.volt_L3);
    // msg->msg18.date_time not used
    activeEnergyPlus = String(msg->msg18.act_energy_pa);
    activeEnergyMinus = String(msg->msg18.act_energy_ma);
    reactiveEnergyPlus = String(msg->msg18.act_energy_pr);
    reactiveEnergyMinus = String(msg->msg18.act_energy_mr);
    break;
  }
  
  /* Call to send notification if active power is over the chosen limit. This 
  is checked for every occurence of List2, 1 phase or 3 phase meter*/
  if (((msg->num_items == 9) || (msg->num_items == 13)) &&  (msg->msg13.act_pow_pos > thrValue)) {
    ifttt_trigger(MakerIFTTT_Key, MakerIFTTT_Event);
  }
}


// Send notification
String ifttt_trigger(String MakerIFTTT_Key, String MakerIFTTT_Event) {
  String name = "";
  // close any connection before send a new request.
  client.stop();
  // connect to the Maker event server
  if (client.connect(hostDomain, hostPort)) {
    // The following line is optional.
    String PostData = "{\"value1\" : \"Grense overskredet!\"}";
    if (debug) {
      mySerial.println("connected to server… Getting name…");
    }
    // send the HTTP PUT request:
    String request = "POST /trigger/";
    request += MakerIFTTT_Event;
    request += "/with/key/";
    request += MakerIFTTT_Key;
    request += " HTTP/1.1";
    if ( debug) {
      mySerial.println(request);
    }
    client.println(request);
    client.println("Host: " + String(hostDomain));
    client.println("User-Agent: Energia/1.1");
    client.println("Connection: close");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(PostData.length());
    client.println();
    client.println(PostData);
    client.println();
  } else {
    // Connection did not succeed:
    if (debug) {
      mySerial.println("Connection failed");
    }
    return "FAIL";
  }
  // Capture response from the ifttt server. Allow 4s to complete
  long timeOut = 4000;
  long refTime = millis();
  while((millis() - refTime) < timeOut) {
    while (client.available()) {
      char c = client.read();
      if (debug) {
        mySerial.write(c);
      }
    }
  }
  if (debug) {
    mySerial.println();
    mySerial.println("Request Complete!");
  }
  return "SUCCESS";
}

