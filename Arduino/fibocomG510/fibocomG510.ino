//Created by Arek Merta
//uczymy.edu.pl
#include <SoftwareSerial.h>
//*****************************************
//Software serial at D12 (RX) and 8 (TX)
//Make sure the A-N jumper is shorted at A
SoftwareSerial ss(12, 8);
//*****************************************
//Send AT command to chip
bool sendATcommand(SoftwareSerial *ss, char* cmd, char* pars, char*postEcho, char*bufRet, int bufRetSize);
//Send AT command repeat-times
bool sendATcommandS(SoftwareSerial *ss, char* cmd, char* pars, char*postEcho, char*bufRet, int bufRetSize, byte repeat, int delays);
//Toggle chip on/off
void chipToggle();
//Turn chip on/off
bool chipOn(bool turnOn);
//check if chip is present
bool chipPresent();
//Display chip capabilities
bool displayModuleCaps();
//*****************************************
//Buffers for send and receive AT messages
const int bufSize = 50;
char bufRet[bufSize];
char bufNow[bufSize];
char retOK[]    = {"OK"};
char retERROR[] = {"ERROR"};
//*****************************************
//State machine
//A SMS has been sent
bool smsSent = false;
//Capabilities of the chip have been read
bool capsDisplayed = false;
//*****************************************
//How many WD (watchdog) ticks
#define WD_RETRIES 200
//Delay (in ms) between watchdog ticks
#define WD_DELAY 50
//Debug: set to true to see debugMe messages
#define DEBUG true
//Debug helper macros
#define debugTron(descr_, val_) \
  if(DEBUG){ \
    Serial.print("     [DEBUG]"); \
    Serial.print(descr_); \
    Serial.print(": {"); \
    Serial.print(val_); \
    Serial.println('}'); \
  }else {}
#define debugStep(descr_) \
  if(DEBUG){ \
    Serial.print("     [DEBUG]"); \
    Serial.println(descr_); \
  }else {}

/*****************************************
  /*
   Send AT command: given number of times with a
   delay between retries

   ss: pointer to a software serial interface
   cmd: AT command (with AT+)
   pars: command parameters
   postEcho: something to be sent if asked (after '>')
   bufRet: buffer for a return value
   bufRetSize: size of bufRet (max 255)
   repeat: number of times tha command will be issued
   delays: delay between each iteration

   return: true if OK returned from AT command; false otherwise
*/
bool sendATcommandS(SoftwareSerial *ss,
                    char *cmd,
                    char *pars,
                    char *postEcho,
                    char *bufRet,
                    int bufRetSize,
                    byte repeat,
                    int delays) {
  //Repeat AT message 'repeat' times
  for (int z = 0; z < repeat; z++) {
    debugTron("Sending command for the (time)", z);
    if ( sendATcommand(ss, cmd, pars, postEcho, bufRet, bufRetSize) ) {
      return true;
    }
    delay(delays);
  }
  return false;
}
//*****************************************
/*
   Send an AT command
   ss: pointer to a software serial interface
   cmd: AT command (with AT+)
   pars: command parameters
   postEcho: something to be sent if asked (after '>')
   bufRet: buffer for a return value
   bufRetSize: size of bufRet (max 255)
   return: true if OK returned from AT command; false otherwise
*/
bool sendATcommand(SoftwareSerial *ss,
                   char *cmd,
                   char *pars,
                   char *postEcho,
                   char *bufRet,
                   int bufRetSize) {
  byte cmdSize = strlen(cmd);
  byte bufPos     = 0;
  char cNow;
  bool bEchoReceived = false;

  debugTron("Sending cmd", cmd);
  debugTron("Sending pars", pars);
  debugTron("Sending post-echo", postEcho);

  //Send the command
  if ( pars ) {
    //Command has parameters
    ss->print( cmd );
    ss->println( pars );
  } else {
    //No parameters set
    ss->println( cmd );
  }
  //This variable holds number of bytes received
  bufPos = 0;
  byte watchdog = WD_RETRIES;
  while (true) {
    //Wait for something to be returned, but with a watchdog
    if ( !ss->available() ) {
      watchdog -= 1;
      if ( watchdog <= 0 ) {
        //Watchdog is now WD_RETRIES x WD_DELAY
        debugStep("Watchdog!");
        return false;
      }
      delay(WD_DELAY);
      continue;
    }
    //Refresh watchdog
    watchdog = WD_RETRIES;
    //Read a character from g510
    cNow = ss->read();
    //ignore \0 and <CR>
    if (cNow == 0 || cNow == 13) continue;

    //AT command wants input
    if ( (char)cNow == '>' && postEcho) {
      debugStep("Sending postEcho on input request");
      ss->println(postEcho);
      continue;
    }

    //Do on <LF>
    if (cNow == 10) {
      //Add string termination
      bufNow[ bufPos ] = '\0';

      debugTron("Got line", bufNow);

      //Check if a line contains echo'ed command
      if (!strncmp(bufNow, cmd, cmdSize)) {
        bEchoReceived = true;
        debugStep("Echo received");
      } else if (!strncmp(bufNow, retERROR, sizeof(retERROR))) {
        //Line contains ERROR
        debugTron("Error received", bufNow);
        return false;
      } else if (!strncmp(bufNow, retOK, sizeof(retOK)) && bEchoReceived ) {
        //Line contains OK
        debugTron("Success received", bufNow);
        return true;
      } else if ( (bufPos > cmdSize) && !strncmp(bufNow + 1, cmd + 3, cmdSize - 3) ) {
        //Line contains value to return
        strncpy(bufRet, bufNow + cmdSize, bufPos - cmdSize);
        debugTron("Return", bufRet);
      }
      //Restart buffer
      bufPos = 0;
    } else {
      //Store in buffer
      bufNow[ bufPos++ ] = cNow;
    }
  }
  //No OK-ERROR with echo received
  debugStep("End - no response, echo");
  return false;
}
//*****************************************
/*
  Toggle chip power on/off; used by chipOn
*/
void chipToggle() {
  debugStep("Toggling chip now");
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  digitalWrite(13, LOW);
  //It takes 3s to switch off and 800ms to switch on;
  //3000 should be enough for both
  delay(3000);
  digitalWrite(13, HIGH);
}
//*****************************************
/*
  Turn chip on/off
  on: if true, turn on, off otherwise
  return: true if chip on now, false otherwise
*/
bool chipOn(bool turnOn) {
  //Check if chip responds
  if ( !chipPresent() ) {
    //No response, chip is off
    if ( turnOn ) {
      //Turn it on
      debugStep("Turning the chip on...");
      chipToggle();
      //Give it some time to settle
      delay(1000);
    } else {
      debugStep("Chip is off - nothing to do.");
      return false;
    }
  } else {
    //Chip is on, request to off?
    if ( !turnOn ) {
      //Turn it off
      debugStep("Turning the chip off...");
      chipToggle();
      //Give it some time to settle
      delay(1000);
    } else {
      debugStep("Chip is on - nothing to do.");
      return true;
    }
  }
  //Make sure the change happened
  return chipPresent();
}

//*****************************************
/*
  Check if chip is on by sending AT command
  return: true if chip on now, false otherwise
*/
bool chipPresent() {
  //Try 5 times with 50ms intervals
  if ( sendATcommandS(&ss, "AT", NULL, NULL, bufRet, 20, 5, 50) ) {
    debugStep("Chip found, ok");
    return true;
  }
  debugStep("Chip NOT found, fail");
  return false;
}
//*****************************************
/*
   This function is to display some
   basic capabilities of the G510 chip
   return: true if all pars
*/
bool displayModuleCaps() {
  if ( !chipPresent() ) {
    Serial.println("Could not find the GSM module... exiting");
    return false;
  } else {
    Serial.println("Module found, ok");
  }

  if ( sendATcommand(&ss, "AT+CGMI", NULL, NULL, bufRet, bufSize) ) {
    Serial.print("Manufacturer: ");
    Serial.println(bufRet);
  } else {
    Serial.println("Could NOT read manufacturer, fail");
    return false;
  }

  if ( sendATcommand(&ss, "AT+CGMM", NULL, NULL, bufRet, bufSize)) {
    Serial.print("Technologies: ");
    Serial.println(bufRet);
  } else {
    Serial.println("Could NOT read technologies, fail");
    return false;
  }

  if ( sendATcommand(&ss, "AT+CGMR", NULL, NULL, bufRet, bufSize)) {
    Serial.print("Revision    : ");
    Serial.println(bufRet);
  } else {
    Serial.println("Could NOT read revision, fail");
    return false;
  }

  if ( sendATcommand(&ss, "AT+CGSN", NULL, NULL, bufRet, bufSize) ) {
    Serial.print("IMEI        : ");
    Serial.println(bufRet);
  } else {
    Serial.println("Could NOT read IMEI, fail");
    return false;
  }
  return true;
}
//*****************************************
/*
   Check if in text mode; try 3 times to succeed
   return: true if a SMS text mode has been sent; false otherwise
*/
bool setTextSMSMode() {
  //Let's try 3 times
  for (int i = 0; i < 3; i++) {
    //Check if the chip is capable to send messages in text mode
    if ( sendATcommand(&ss, "AT+CMGF", "?", NULL, bufRet, bufSize) ) {
      Serial.print("SMS format  : ");
      //Should be 0 or one, 0 means PDU
      byte smsMode = bufRet[0] - '0';
      Serial.println( smsMode == 0 ? "PDU" : "Text" );
      //We need text mode, not PDU
      if ( smsMode == 0 ) {
        if ( !sendATcommandS(&ss, "AT+CMGF", "=1", NULL, bufRet, bufSize, 3, 50) ) {
          debugStep("Could not set the text mode for SMS...");
          //Try again...
        }
      } else {
        debugStep("Mode set to text, you can SMS now");
        return true;
      }
    }
  }
  debugStep("Could not set the text mode for SMS, exiting.");
  return true;
}
//*****************************************
/*
   Send an sms
   cellNo: mobile number
   message: message to be sent
   return: true if SMS sent
*/
bool sms(const char* cellNo, const char* message) {
  //Allocate buffers for mobile number and message - rememeber to delete[]
  byte cellNoLen = strlen(cellNo);
  char* bufCellNoMy = new char[cellNoLen + 4];

  byte messageLen = strlen(message);
  char substChar = 26;
  char* bufMessageMy = new char[messageLen + 2];

  bool bSMSsent = false;

  //Check if chip is present
  if ( !chipPresent()) {
    Serial.println("Could not find GSM module... exiting");
  }else
  //Set the text mode mode
  if ( !setTextSMSMode() ) {
    Serial.println("Could not set Text SMS module... exiting");
  }else
  //Check if SMS center number is available
  if ( !sendATcommand( &ss, "AT+CSCA", "?", NULL, bufRet, bufSize ) ) {
    Serial.println("SMS centre not set... exiting");
  } else {
    Serial.print("SMS centre  : ");
    Serial.println(bufRet);
    //Mobile number must be between quotes
    sprintf(bufCellNoMy, "=\"%s\"\0", cellNo );
  
    //Message must end with \x1a<CR><LF>
    sprintf(bufMessageMy, "%s%c\0", message, substChar );

    //Send SMS and rememeber status
    bSMSsent = sendATcommand( &ss, "AT+CMGS", bufCellNoMy, bufMessageMy, bufRet, bufSize );
    if ( bSMSsent ) {
      Serial.println("SMS sent.");
    } else {
      Serial.println("Could not send SMS.");
    }
  }
  //Cleanup
  delete [] bufMessageMy;
  delete [] bufCellNoMy;

  return bSMSsent;
}
//*****************************************
void setup() {
  //Setup serial port to communicate with G510
  ss.begin(9600);
  //Set serial port for Arduino
  Serial.begin(9600);
  Serial.println("-------------------------------------");
  Serial.println("Waiting for chip to turn on (it may take a while)...");
}
//*****************************************
void loop() {
  Serial.println("=====================================");
  //If caps read not successful - repeat
  if ( !capsDisplayed ) {
    capsDisplayed = displayModuleCaps();
  }
  //If SMS not send - repeat
  if ( capsDisplayed && !smsSent ) {
    //Use your mobile number instead of ********
    smsSent = sms("+48********", "Hello from uczymy.edu.pl");
  }
  //Give module 5s to get a grasp
  delay(5000);
}
