#include <WebSocketServer.h>
using namespace net;

#if PLATFORM_ARCH == PLATFORM_ARCHITECTURE_SAMD21
#  define _SERIAL SerialUSB
#else
#  define _SERIAL Serial
#endif

#if NETWORK_CONTROLLER == NETWORK_CONTROLLER_WIFI
constexpr char kSSID[]{ "SKYNET" };
constexpr char kPassword[]{ "***" };
#else
byte mac[]{ 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip{ 192, 168, 46, 180 };
#endif

constexpr uint16_t kPort{ 3000 };
WebSocketServer wss{ kPort };

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
   //Ethernet.init(SS);

  Ethernet.begin(mac, ip);
#  endif

  _SERIAL.print(F("Server running at "));
  _SERIAL.print(Ethernet.localIP());
  _SERIAL.print(F(":"));
  _SERIAL.println(kPort);
#endif

  wss.onConnection([](WebSocket &ws) {
    ws.onMessage([](WebSocket &ws, const WebSocket::DataType &dataType,
                   const char *message, uint16_t length) {
      switch (dataType) {
      case WebSocket::DataType::TEXT:
        _SERIAL.print(F("Received: "));
        _SERIAL.println(message);
        break;
      case WebSocket::DataType::BINARY:
        _SERIAL.println(F("Received binary data"));
        break;
      }

      ws.send(dataType, message, length);
    });

    ws.onClose(
      [](WebSocket &ws, const WebSocket::CloseCode &code, const char *reason,
        uint16_t length) { _SERIAL.println(F("Disconnected")); });

    _SERIAL.print(F("New client: "));
    _SERIAL.println(ws.getRemoteIP());

    constexpr char message[]{ "Hello from Arduino server!" };
    ws.send(WebSocket::DataType::TEXT, message, strlen(message));
  });

  wss.begin();
}

void loop() { wss.listen(); }