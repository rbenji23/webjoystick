/* application.cpp - very simple Webduino example */
#include "application.h"
#include "WebServer.h"
//#include "jquery.h"
#include "joystick.h"
#include "webpages.h"
#include "MDNS/MDNS.h"

/* all URLs on this server will start with /led because of how we
 * define the PREFIX value.  We also will listen on port 80, the
 * standard HTTP service port */
#define PREFIX ""
WebServer webserver(PREFIX, 80);

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(MANUAL);
STARTUP(WiFi.selectAntenna(ANT_EXTERNAL));

auto& console = Serial;

CANChannel can(CAN_D1_D2,256,32);
CANMessage RPDO1Message;

MDNS mdns;

/* LED built into the board is on pin D7 */
int led = D7;
int leftMotorCmd = 0;
int rightMotorCmd = 0;

void Process50msecEvents()
{
  RPDO1Message.data[0] = (leftMotorCmd & 0x000000ff);
  RPDO1Message.data[1] = (leftMotorCmd & 0x0000ff00) >> 8;
  RPDO1Message.data[2] = (leftMotorCmd & 0x00ff0000) >> 16;
  RPDO1Message.data[3] = (leftMotorCmd & 0xff000000) >> 24;
  RPDO1Message.data[4] = (rightMotorCmd & 0x000000ff);
  RPDO1Message.data[5] = (rightMotorCmd & 0x0000ff00) >> 8;
  RPDO1Message.data[6] = (rightMotorCmd & 0x00ff0000) >> 16;
  RPDO1Message.data[7] = (rightMotorCmd & 0xff000000) >> 24;

  can.transmit(RPDO1Message);
}

void Timeout()
{
  leftMotorCmd = 0;
  rightMotorCmd = 0;
}

Timer timer_50msec(50, Process50msecEvents);
Timer timer_timeout(5000, Timeout);

void indexCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
  console.printlnf("Single Joystick Page Called");

  server.httpSuccess();

  if (type != WebServer::HEAD)
  {
    server.printP(indexPage);
  }
}

void singlejsCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
  console.printlnf("Single Joystick Page Called");

  server.httpSuccess();

  if (type != WebServer::HEAD)
  {
    server.printP(singlejsPage);
  }
}

void dualjsCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
  console.printlnf("Dual Joystick Page Called");

  server.httpSuccess();

  if (type != WebServer::HEAD)
  {
    server.printP(dualjsPage);
  }
}

// void jqueryjsCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
// {
//   console.printlnf("jquery.js Called");
//   /* this line sends the standard "we're all OK" headers back to the
//      browser */
//   server.httpSuccess();
//
//   /* if we're handling a GET or POST, we can output our data here.
//      For a HEAD request, we just stop after outputting headers. */
//   if (type != WebServer::HEAD)
//   {
//     /* this is a special form of print that outputs from PROGMEM */
//     server.printP(jqueryjs1);
//     //server.printP(joystickjs2);
//   }
// }

void joystickjsCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
  console.printlnf("virtualjoystick.js Called");

  server.httpSuccess();

  if (type != WebServer::HEAD)
  {
    server.printP(joystickjs1);
    server.printP(joystickjs2);
  }
}

#define NAMELEN 32
#define VALUELEN 32

void motorCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  console.printlnf("MotorCmd Called");

  URLPARAM_RESULT rc;
  char name[NAMELEN];
  char value[VALUELEN];

  server.httpSuccess();

  if (type != WebServer::HEAD)
  {
    if (strlen(url_tail))
    {
      while (strlen(url_tail))
      {
        rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
        if (String(name).equalsIgnoreCase("LEFT")){
          leftMotorCmd = String(value).toInt();
        }
        else if (String(name).equalsIgnoreCase("RIGHT")){
          rightMotorCmd = String(value).toInt();
        }
      }
    }
  }
  console.printlnf("LeftSpeed=%d",leftMotorCmd);
  console.printlnf("RightSpeed=%d",rightMotorCmd);
}

void setup()
{
  console.begin(9600);

  WiFi.on();
  WiFi.connect();
  waitUntil(WiFi.ready);

  //Start CANbus
  can.begin(250000);

  //Configure RPDO1Message
  RPDO1Message.id = 0x201;
  RPDO1Message.len = 8;

  //Start Timers that send CAN messages
  timer_50msec.start();
  timer_timeout.start();

  if(mdns.setHostname("photon"))
  {
    mdns.begin();
  }

  // set the output for the LED to out
  // Unused in this project but kept for debug if needed
  pinMode(led, OUTPUT);

  //Add Pages to server
  webserver.setDefaultCommand(&indexCmd);
  webserver.addCommand("index.html",&indexCmd);
  webserver.addCommand("single.html",&singlejsCmd);
  webserver.addCommand("dual.html",&dualjsCmd);
  webserver.addCommand("cmd", &motorCmd);
  webserver.addCommand("jquery.js",&joystickjsCmd);
  webserver.addCommand("virtualjoystick.js",&joystickjsCmd);

  /* start the server to wait for connections */
  webserver.begin();

}

void loop()
{
  //Process MDNS
  mdns.processQueries();

  // process incoming connections one at a time forever
  webserver.processConnection();
}
