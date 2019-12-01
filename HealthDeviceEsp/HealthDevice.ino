#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#define samp_siz 4
#define rise_threshold 5

ESP8266WebServer server;
WebSocketsServer webSocket = WebSocketsServer(81);

uint8_t pin_led = 2;
char *ssid = "wifiname";
char *password = "wifipassword";
bool temperature = true;
float reads[samp_siz], sum;
long int now, ptr;
float last, reader, start;
float first, second, third, before, print_value;
bool rising;
int rise_count;
int n;
long int last_beat;

char webpage[] PROGMEM = R"=====(
<!DOCTYPE html><html lang="en"><head><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>Health Device</title></head><body><style>*{margin:0;padding:0;font-size:1.1em}button{border:none;background-color:green;color:#fff;width:60%;margin:10px}header{width:100%;height:30px;background-color:#000;color:#fff}header p{display:inline-block}</style>
    <header><p>Health Device</p><p style="position: absolute;right: 10px;" onclick="Share()">Share</p></header>
    <p style="font-size: 30px;margin-left:10px"><label id="temp"></label>&deg;C <label id="sym"></label></p>
    <center><button onclick="send('t')">Get Temperature</button></center>
    <hr width="100%" style="height:2px;border:none;color:#000;background-color:#000" />
    <center><button onclick="send('h')">Get Heart Beat</button></center>
    <center style="font-size:23px;color:red;"><p>Heart Rate: </p><p id="bval"></p></center>
    <canvas id="beats" style="position: fixed;bottom: 0;"></canvas>
    <script>
        var temp = 36.7,h,pts = [], canvas = document.getElementById('beats'),ctx = canvas.getContext('2d');
        canvas.width = window.innerWidth;canvas.height = h = 400;
        function Share(){
          var u = "whatsapp://send?text=Health Device Result\nTemperature: "+temp+"\n";
          if(pts.length > 0) u += "HeartBeat: "+pts[pts.length-1]+"\n";
          u = encodeURI(u);
          window.open(u,"_blank");
        }
        function addPoint(x) {
            while (pts.length * 20 > window.innerWidth - 40)
                pts.shift();
            pts.push(2 * x);
            Draw();
        }
        function Draw() {
            document.getElementById('temp').innerText = temp;
            document.getElementById('sym').innerHTML = (temp >= 35.5 && temp <= 38.0)?"&#128515;":"&#129298;";
            ctx.clearRect(0,0,window.innerWidth,h);
            ctx.lineWidth = 5; ctx.strokeStyle = "#f00";
            ctx.beginPath(); ctx.moveTo(0, 0); ctx.lineTo(0, h); ctx.stroke();
            ctx.lineWidth = 1;
            for (i = 0; i <= 200; i += 10) {
                ctx.beginPath();
                ctx.moveTo(0, h - 2 * i);
                ctx.lineTo(10, h - 2 * i);
                ctx.stroke();
                ctx.fillText("" + i, 10, h - 2 * i);
            }
            if(pts.length < 1) return;
            document.getElementById('bval').innerText=pts[pts.length-1];
            ctx.lineWidth = 2;ctx.strokeStyle = "#000";ctx.lineJoin = "round";
            ctx.beginPath();
            ctx.moveTo(30,pts[0]);
            for(var i=1;i<pts.length;++i)
                ctx.lineTo(i*20+30,pts[i]);
            ctx.stroke();
        }
        var Socket;
        function init() {
            Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
            Socket.onmessage = function (eve) {
              console.log(eve.data);
                if(eve.data[0] == "h"){
                  addPoint(parseFloat(eve.data.substr(1)));
                }else{
                  temp = parseFloat(eve.data.replace(/[^\d.]/g,''));
                  document.getElementById('temp').innerText = temp;
                  document.getElementById('sym').innerHTML = (temp >= 35.5 && temp <= 38.0)?"&#128515;":"&#129298;";
                }
            }
        }
        function send(e) { if(Socket.readyState !== WebSocket.OPEN){console.log("Not Connected");return;}Socket.send(e + "");}
        Draw();init();</script></body></html>
)=====";
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  if (type == WStype_TEXT)
  {
    if (payload[0] == 't')
    {
      temperature = true;
      Serial.println("Reading Temperature");
      digitalWrite(D1, LOW);
      digitalWrite(D0, HIGH);
    }
    if (payload[0] == 'h')
    {
      temperature = false;
      Serial.println("Reading Heartbeat");
      digitalWrite(D1, HIGH);
      digitalWrite(D0, LOW);
    }
  }
}

void setup()
{
  pinMode(pin_led, OUTPUT);
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);
  digitalWrite(D0, LOW);
  digitalWrite(D1, LOW);

  WiFi.begin(ssid, password);
  Serial.begin(115200);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", []() {
    server.send_P(200, "text/html", webpage);
  });
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  digitalWrite(D0, HIGH);
}
unsigned long current = 0;

void loop()
{
  webSocket.loop();
  server.handleClient();
  if (temperature)
  {
    if (millis() < current + 3000)
      return;
    int analogValue = analogRead(A0);
    float millivolts = (analogValue / 1024.0) * 330;
    String x = String(millivolts);
    char tm[x.length()+1];
    x.toCharArray(tm,sizeof(tm));
    webSocket.broadcastTXT(tm, sizeof(tm));
    current = millis();
  }
  else{
    for (int i = 0; i < samp_siz; i++) {
      reads[i] = 0;
    }
    sum = 0;
    ptr = 0;
    n = 0;
    start = millis();
    reader = 0.0;
    do{
      reader += analogRead(A0);
      ++n;
      now = millis();
    }while (now < start + 20);
    
    reader /= n;
    sum -= reads[ptr];
    sum += reader;
    reads[ptr] = reader;
    last = sum / samp_siz;
    if (last > before){
      rise_count++;
      if (!rising && rise_count > rise_threshold)
      {
        rising = true;
        first = millis() - last_beat;
        last_beat = millis();
        print_value = 60000. / (0.4 * first + 0.3 * second + 0.3 * third);
        String x = String(print_value);
        x = "h" + x;
        char c[5];
        x.toCharArray(c, 5);
        webSocket.broadcastTXT(c, sizeof(c));
    
        Serial.print(print_value);
        Serial.print('\n');
        third = second;
        second = first;
      }
    }
    else
    {
      rising = false;
      rise_count = 0;
    }
    before = last;
    ptr++;
    ptr %= samp_siz;
    // int e = random(190);
    // String x = String(e);
    // x = "h" + x;
    // char c[5];
    // x.toCharArray(c, 5);
    // webSocket.broadcastTXT(c, sizeof(c));
    // delay(2000);*/
  }
  
}

