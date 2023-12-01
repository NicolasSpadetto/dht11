#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <EthernetUdp.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <TimeLib.h>


#define DHTPIN 2
#define DHTTYPE DHT11
#define DESCANSO 60000 //em milisec, 1 min


//conexão mqtt
const int hive_port = 1883;
const char hive_server[] = "broker.hivemq.com";
//const char hive_user[] = "ardNic";
//const char hive_senha[] = "TrabalhoComDados2";
const char idDeCliente[] = "ardNic";
const int com_time = 180; //tempo em segundos de conexão para o servidor e cliente
const char topico[] = "ardNic/DHT11/leituras";
const int fuso = -3;


//conexão com relógio

IPAddress ntpBR(200, 160, 7, 186);
const int p_local = 1883;



DHT dht(DHTPIN, DHTTYPE); //inicializando sensor

byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE}; //endereço da máquina
IPAddress ip(192, 168, 100, 77);

//conexão normal para o servidor
EthernetClient cliente;
PubSubClient hBroker(cliente);

//conexão udp para o relógio
EthernetUDP Udp;

const int NTP_PACKET_SIZE = 48; // time stamp está nos primeiros 48 bits
byte buffer[NTP_PACKET_SIZE]; // buffer para pedir e receber dados do servidor ntp


//sensor DHT11
bool checkSUM(float dado) // retorna 1 caso checkSUM foi correto
{
  if (isnan(dado)) return 0;
  else return 1;
}

float getTempUmid(bool temp0umid1) //ponha 0 se quer a temperatura e 1 se quiser umidade
{ 
  float umid, temp;

  bool temp_cfrm, umid_cfrm;
  temp_cfrm = 0;
  umid_cfrm = 0;
  
  if(temp0umid1) //true para umidade 
  {
    while (!umid_cfrm)
    {
      delay(2000); //delay recomendado pelo fabricante para não sobrecarregar o sensor 
      
      umid = dht.readHumidity();// pega umidade
      umid_cfrm = checkSUM(umid);// testa
    }
    return umid;
  }
  else //false para temperatura
  {
    while (!temp_cfrm)
    {
      delay(2000);
      temp = dht.readTemperature();
      temp_cfrm = checkSUM(temp);
    }
    return temp;
  }
  
}

//relógio NTP

void envPacNTP(IPAddress &addr) 
{
  //limpa buffer
  memset(buffer, 0, NTP_PACKET_SIZE);

  //monta o frame para pedir o tempo
  buffer[0] = 0b11100011;   
  buffer[1] = 0;     
  buffer[2] = 6;     
  buffer[3] = 0xEC;  
  
  buffer[12]  = 49;
  buffer[13]  = 0x4E;
  buffer[14]  = 49;
  buffer[15]  = 52;

  Udp.beginPacket(addr, 123); // porta onde ocorre a transmissão dos dados
  Udp.write(buffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

time_t NTPTempo()
{
  while (Udp.parsePacket() > 0) ; //limpa fila
  envPacNTP(ntpBR);
  uint32_t espera = millis();
  while ((millis() - espera) < 1500) 
  {
    int tamanho = Udp.parsePacket();
    if (tamanho >= NTP_PACKET_SIZE) 
    {
      Udp.read(buffer, NTP_PACKET_SIZE);  //lê fila

      unsigned long secs_1900;
      
      secs_1900 =  (unsigned long)buffer[40] << 24;
      secs_1900 |= (unsigned long)buffer[41] << 16;
      secs_1900 |= (unsigned long)buffer[42] << 8;
      secs_1900 |= (unsigned long)buffer[43];
      return secs_1900 - 2208988800UL + fuso * SECS_PER_HOUR;
    }
  }
  Serial.println("Erro ao tentar pegar o horário");
  return 0; // return 0 if unable to get the time
}

//reconectar ao broker caso comunicação acabe
void reconectar()
{
  if (hBroker.state() != 0)
  {
    while (!hBroker.connect(idDeCliente));
  }
  hBroker.loop();
}



String msgBuild()
{
  time_t tmp = now();

  float temp, umid;
  
  //medições
  temp = getTempUmid(0);
  umid = getTempUmid(1);
  
  String mensagem = "";
  mensagem.reserve(40); // 8 dig para temperatura, 8 para umidade, 8 para data, 5 para hora, 9 para símbolos de separação = multiplo de 8 mais proximo (40)

  mensagem += '{';
  if (day(tmp) < 10) 
  {
    mensagem += '0';
    mensagem += day(tmp);
  }
  else mensagem += day(tmp);

  mensagem += "/";

  if (month(tmp) < 10) 
  {
    mensagem += '0';
    mensagem += month(tmp);
  }
  else mensagem += month(tmp);

  mensagem += "/";

  mensagem += (year(tmp) - 2000);

  mensagem += "-";

  if (hour(tmp) < 10) 
  {
    mensagem += '0';
    mensagem += hour(tmp);
  }
  else mensagem += hour(tmp);

  mensagem += ":";

  if (minute(tmp) < 10) 
  {
    mensagem += '0';
    mensagem += minute(tmp);
  }
  else mensagem += minute(tmp);

  mensagem += "} ";
  mensagem += '{';
  mensagem += temp;
  mensagem += "°C} ";
  mensagem += '{';
  mensagem += umid;
  mensagem += "%}";

  Serial.println(mensagem);

  return mensagem;
}

void msgEnv() // chama as funções para enviar a mensagem
{
  
 
  while (!(hBroker.publish(topico, msgBuild().c_str(), 1))) 
  {
    Serial.println("Envio Falhou! Tentando novamente em 3s...");
    delay(3000);
  }
}

void setup() 
{
  Serial.begin(9600);
  dht.begin();

  while (!Serial); //aguarda a porta serial conectar
  
  Ethernet.begin(mac, ip);  //inicia a conexão
  delay(2000); //delay para tudo se ajeitar

  hBroker.setServer(hive_server, hive_port);//escolhe o servidor

  hBroker.setKeepAlive(com_time);//muda o tempo de conexão
  
  reconectar();

  Udp.begin(p_local);

  setSyncProvider(NTPTempo); // passa o endereço da função q será usada para resincronizar
  setSyncInterval(DESCANSO / 1000);//1 min entre os re-syncs
}

void loop() 
{
  reconectar();
  msgEnv();
  delay(DESCANSO);
  
}
