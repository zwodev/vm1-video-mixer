
#include <SPI.h>
#include <SPISlave.h>


// Wiring:
// Master RX  GP0 <-> GP11  Slave TX
// Master CS  GP1 <-> GP9   Slave CS
// Master CK  GP2 <-> GP10  Slave CK
// Master TX  GP3 <-> GP8   Slave RX

#define PIN_RX 16
#define PIN_CS 17
#define PIN_SCLK 18
#define PIN_TX 19

SPISettings spisettings(1000000, MSBFIRST, SPI_MODE0);
// SPISettings spisettings(20000000, MSBFIRST, SPI_MODE0);


volatile bool recvBuffReady = false;
char recvBuff[42] = "";
int recvIdx = 0;
void recvCallback(uint8_t *data, size_t len) {
  Serial.println("received something!");
  memcpy(recvBuff + recvIdx, data, len);
  recvIdx += len;
  if (recvIdx == sizeof(recvBuff)) {
    recvBuffReady = true;
    recvIdx = 0;
  }
}

int sendcbs = 0;
// Note that the buffer needs to be long lived, the SPISlave doesn't copy it.  So no local stack variables, only globals or heap(malloc/new) allocations.
char sendBuff[42];
void sentCallback() {
  memset(sendBuff, 0, sizeof(sendBuff));
  sprintf(sendBuff, "Slave to Master Xmission %d", sendcbs++);
  SPISlave.setData((uint8_t*)sendBuff, sizeof(sendBuff));
}

// Note that we use SPISlave1 here **not** because we're running on
// Core 1, but because SPI0 is being used already.  You can use
// SPISlave or SPISlave1 on any core.
void setup() {
  Serial.begin(115200);
  SPISlave.setRX(PIN_RX);
  SPISlave.setCS(PIN_CS);
  SPISlave.setSCK(PIN_SCLK);
  // SPISlave1.setTX(PIN_TX);
  // Ensure we start with something to send...
  // sentCallback();
  // Hook our callbacks into the slave
  // SPISlave1.onDataRecv(recvCallback);
  // SPISlave1.onDataSent(sentCallback);
  SPISlave.begin(spisettings);
  delay(3000);
  Serial.println("S-INFO: SPISlave started");
}

void loop() {
  Serial.println("hello");
  delay(2000);
  if (recvBuffReady) {
    Serial.printf("S-RECV: '%s'\n", recvBuff);
    recvBuffReady = false;
  }
}