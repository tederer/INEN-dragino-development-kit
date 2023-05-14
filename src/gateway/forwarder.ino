#include <FileIO.h>
#include <Console.h>
#include <Process.h>
#include <SPI.h>
#include <LoRa.h>

#define VERSION       "fwd_v049"

#define MESSAGE_SIZE  256
#define LOGFILE       "/var/log/forwarder"
#define DATAFILE      "/var/iot/data"

static float rxfreq, txfreq;
static int SF, CR, txsf;
static long BW, preLen;

void log(char* msg) {
  File logfile = FileSystem.open(LOGFILE, FILE_APPEND);
  logfile.println(msg);
  logfile.close();
}

//Get LoRa Radio Configure from LG01
void readGatewayRadioConfig() {

    char tmp[32];
    Process p;

    File logfile = FileSystem.open(LOGFILE, FILE_APPEND);

    //Read frequency from uci ####################
    int j = 0;
    memset(tmp, 0, sizeof(tmp));
    p.begin("uci");
    p.addParameter("get");
    p.addParameter("lorawan.radio.rx_frequency");
    p.run();    // Run the process and wait for its termination
    while (p.available() > 0 && j < 9) {
        tmp[j] = p.read();
        j++;
    }
    rxfreq = atof(tmp);
    
    logfile.print("   rx-freq = ");
    logfile.println(rxfreq);

    //Read txfre from uci ####################
    j = 0;
    memset(tmp, 0, sizeof(tmp));
    p.begin("uci");
    p.addParameter("get");
    p.addParameter("lorawan.radio.tx_frequency");
    p.run();    // Run the process and wait for its termination
    while (p.available() > 0 && j < 10) {
        tmp[j] = p.read();
        j++;
    }
    txfreq = atof(tmp);

    logfile.print("   tx-freq = ");
    logfile.println(txfreq);

    //Read Spread Factor ####################
    j = 0;
    memset(tmp, 0, sizeof(tmp));
    p.begin("uci");
    p.addParameter("get");
    p.addParameter("lorawan.radio.SF");
    p.run();    // Run the process and wait for its termination
    while (p.available() > 0 && j < 3) {
        tmp[j] = p.read();
        j++;
    }

    SF = atoi(tmp) > 0 ? atoi(tmp) : 10;  //default SF10

    logfile.print("   SF = ");
    logfile.println(SF);

    //Read tx Spread Factor ####################
    j = 0;
    memset(tmp, 0, sizeof(tmp));
    p.begin("uci");
    p.addParameter("get");
    p.addParameter("lorawan.radio.TXSF");
    p.run();    // Run the process and wait for its termination
    while (p.available() > 0 && j < 3) {
        tmp[j] = p.read();
        j++;
    }

    txsf = atoi(tmp) > 0 ? atoi(tmp) : 9;  //Txsf default to sf9

    logfile.print("   txsf = ");
    logfile.println(txsf);
    
    //Read Coding Rate  ####################
    j = 0;
    memset(tmp, 0, sizeof(tmp));
    p.begin("uci");
    p.addParameter("get");
    p.addParameter("lorawan.radio.coderate");
    p.run();    // Run the process and wait for its termination
    while (p.available() > 0 && j < 2) {
        tmp[j] = p.read();
        j++;
    }
    CR = atoi(tmp);

    logfile.print("   CR = ");
    logfile.println(CR);
    
    //Read PreambleLength
    j = 0;
    memset(tmp, 0, sizeof(tmp));
    p.begin("uci");
    p.addParameter("get");
    p.addParameter("lorawan.radio.preamble");
    p.run();    // Run the process and wait for its termination
    while (p.available() > 0 && j < 5) {
        tmp[j] = p.read();
        j++;
    }
    preLen = atol(tmp);

    logfile.print("   preLen = ");
    logfile.println(preLen);
    
    //Read BandWidth  #######################

    j = 0;
    memset(tmp, 0, sizeof(tmp));
    p.begin("uci");
    p.addParameter("get");
    p.addParameter("lorawan.radio.BW");
    p.run();    // Run the process and wait for its termination
    while (p.available() > 0 && j < 2) {
        tmp[j] = p.read();
        j++;
    }

    switch (atoi(tmp)) {
        case 0: BW = 7.8E3; break;
        case 1: BW = 10.4E3; break;
        case 2: BW = 15.6E3; break;
        case 3: BW = 20.8E3; break;
        case 4: BW = 31.25E3; break;
        case 5: BW = 41.7E3; break;
        case 6: BW = 62.5E3; break;
        case 7: BW = 125E3; break;
        case 8: BW = 250E3; break;
        case 9: BW = 500E3; break;
        default: BW = 125E3; break;
    }

    logfile.print("   BW = ");
    logfile.println(BW);
    
    logfile.close();
}

void configureLoRaModule() {
    LoRa.setFrequency(rxfreq);
    LoRa.setSpreadingFactor(SF);
    LoRa.setSignalBandwidth(BW);
    LoRa.setCodingRate4(CR);
    //LoRa.setSyncWord(0x34);
    LoRa.setPreambleLength(preLen);
}

void writeVersion() {
  File fw_version = FileSystem.open("/var/avr/fw_version", FILE_WRITE); 
  fw_version.print(VERSION);
  fw_version.close();
}

void setup(){
  Bridge.begin(115200);
  FileSystem.begin();
  writeVersion();
  log(VERSION);
  log("reading radio config");
  readGatewayRadioConfig();
  log("LoRa.begin");
  if (LoRa.begin(rxfreq) == 0) {
    log("ERROR: LoRa.begin failed");
  }
  log("configureLoRaModule");
  configureLoRaModule();
  log("end of setup");
}

void loop(){
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    log("packet received");
    File f = FileSystem.open(LOGFILE, FILE_APPEND);
    f.print("   size       = ");
    f.println(packetSize);
    f.print("   data (dec) = ");

    char message[MESSAGE_SIZE];
    int readBytes = 0;
    int i         = 0;
    while(readBytes < packetSize) {
      byte b = LoRa.read();
      f.print(b);
      f.print(" ");
      if (readBytes >= 4) {
        message[i++] = b;
      }
      readBytes++;
    }
    message[i] = 0;
    f.println();
    f.close();

    File dataFile = FileSystem.open(DATAFILE, FILE_WRITE);
    dataFile.println(message);
    dataFile.close();

    File g = FileSystem.open(LOGFILE, FILE_APPEND);
    g.print("   message    = ");
    g.println(message);
    g.close();
  }
}