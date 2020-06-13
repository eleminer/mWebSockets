#include <WebSocketServer.h>
using namespace net;

#if PLATFORM_ARCH == PLATFORM_ARCHITECTURE_SAMD21
# define _SERIAL SerialUSB
#else
# define _SERIAL Serial
#endif

#if NETWORK_CONTROLLER == NETWORK_CONTROLLER_WIFI
constexpr char kSSID[]{ "SKYNET" };
constexpr char kPassword[]{ "***" };
#else
byte mac[]{ 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 46, 180);
#endif

const uint16_t kPort = 9001;
WebSocketServer server(kPort);

void setup() {
  _SERIAL.begin(115200);
  while (!_SERIAL);

#if NETWORK_CONTROLLER == NETWORK_CONTROLLER_WIFI
  //_SERIAL.setDebugOutput(true);
  _SERIAL.printf("\nConnecting to %s ", kSSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(kSSID, kPassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); _SERIAL.print(F("."));
  }

  _SERIAL.println(" connected");

  WiFi.printDiag(_SERIAL);

  _SERIAL.print(F("Server running at "));
  _SERIAL.print(WiFi.localIP()); _SERIAL.print(F(":"));
  _SERIAL.println(kPort);
#else
  _SERIAL.println(F("Initializing ... "));

#  if NETWORK_CONTROLLER == ETHERNET_CONTROLLER_W5X00
  // Ethernet.init(53);
#    if PLATFORM_ARCH == PLATFORM_ARCHITECTURE_STM32
  Ethernet.init(PA4);
#    endif
#  endif

  Ethernet.begin(mac, ip);

  _SERIAL.print(F("Server running at "));
  _SERIAL.print(Ethernet.localIP()); _SERIAL.print(F(":"));
  _SERIAL.println(kPort);
#endif

  server.onConnection([](WebSocket &ws) {
    ws.onMessage([](WebSocket &ws, const WebSocket::DataType &dataType,
                   const char *message,
                   uint16_t length) { ws.send(dataType, message, length); });
  });

  server.begin();
}

void loop() { server.listen(); }