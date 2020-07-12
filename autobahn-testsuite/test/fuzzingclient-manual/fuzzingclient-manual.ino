/*
   1. Install docker (and set shared drives)
   2. Run docker pull crossbario/autobahn-testsuite
   3. Download: https://github.com/crossbario/autobahn-testsuite
   4. Modify ./docker/config/ .json files
      set server ip (this device), agent (name) and cases
   5. Run with admin permissions:
      docker run -it --rm -v %cd%\config:/config \
      -v %cd%\reports:/reports crossbario/autobahn-testsuite \
      -p 9001:9001 wstest -m fuzzingserver \
      -s /config/fuzzingserver.json
   6. Write to serial (for each case):
      <192.168.46.9,9001,/runCase?case=1&agent=Mega2560>
   7. Write to serial (to generate report):
      <192.168.46.9,9001,/updateReports?agent=Mega2560>
   8. See client-auto.js for automation
*/

#include <WebSocketClient.h>
using namespace net;

#if PLATFORM_ARCH == PLATFORM_ARCHITECTURE_SAMD21
#  define _SERIAL SerialUSB
#else
#  define _SERIAL Serial
#endif

constexpr uint8_t kCmdBufferSize{ 64 };
char cmdBuffer[kCmdBufferSize]{};
char tempBuffer[kCmdBufferSize]{};

bool newData{ false };

char host[32]{};
char path[48]{};
uint16_t port{ 3000 };

#if NETWORK_CONTROLLER == NETWORK_CONTROLLER_WIFI
const char kSSID[]{ "SKYNET" };
const char kPassword[]{ "***" };
#else
byte mac[]{ 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
// IPAddress ip{ 192, 168, 46, 179 };
#endif

WebSocketClient client;

void setup() {
  _SERIAL.begin(115200);
  while (!_SERIAL)
    ;

#if NETWORK_CONTROLLER == NETWORK_CONTROLLER_WIFI
  //_SERIAL.setDebugOutput(true);
  _SERIAL.printf("\nConnecting to %s ", kSSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(kSSID, kPassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    _SERIAL.print(F("."));
  }

  _SERIAL.println(F(" connected"));

  WiFi.printDiag(_SERIAL);

  _SERIAL.print(F("Device IP: "));
  _SERIAL.println(WiFi.localIP());
#else
  _SERIAL.println(F("Initializing ... "));

#  if NETWORK_CONTROLLER == ETHERNET_CONTROLLER_W5X00
  // Ethernet.init(5); // ESPDUINO-32
  // Ethernet.init(53); // Mega2560
#  endif

  Ethernet.begin(mac); //, ip);

  _SERIAL.print(F("Device IP: "));
  _SERIAL.println(Ethernet.localIP());
#endif

  client.onOpen([](WebSocket &ws) { _SERIAL.println(F("Connected")); });

  client.onClose(
    [](WebSocket &ws, const WebSocket::CloseCode &code, const char *reason,
      uint16_t length) { _SERIAL.println(F("Disconnected\n")); });

  client.onMessage(
    [](WebSocket &ws, const WebSocket::DataType &dataType, const char *message,
      uint16_t length) { ws.send(dataType, message, length); });
}

void loop() {
  static bool recvInProgress{ false };
  static byte idx{ 0 };

  //char c;

  while (_SERIAL.available() > 0 && newData == false) {
    char c{ _SERIAL.read() };
    if (recvInProgress == true) {
      if (c != '>') {
        cmdBuffer[idx++] = c;

        if (idx >= kCmdBufferSize) idx = kCmdBufferSize - 1;
      } else {
        cmdBuffer[idx] = '\0';
        recvInProgress = false;
        idx = 0;
        newData = true;
      }
    } else if (c == '<')
      recvInProgress = true;
  }

  if (newData == true) {
    strcpy(tempBuffer, cmdBuffer);

    char *ptr{ strtok(tempBuffer, ",") };
    strcpy(host, ptr);

    ptr = strtok(nullptr, ",");
    port = atoi(ptr);

    ptr = strtok(nullptr, ",");
    strcpy(path, ptr);

    _SERIAL.print(F("Performing test: "));
    _SERIAL.println(path);

    if (!client.open(host, port, path)) {
      //_SERIAL.println("Connection failed!");
    }

    newData = false;
  }

  client.listen();
}