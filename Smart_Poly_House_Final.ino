#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <HardwareSerial.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

unsigned long lastLcdUpdate = 0;
int lcdPage = 0;
String espIP = "";

/* ================= WIFI ================= */
// Configure your Wi-Fi credentials locally before flashing.
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

/* ================= ADMIN LOGIN ================= */
const char* USERNAME = "Rafe";
const char* PASSWORD = "CHANGE_THIS_ADMIN_PASSWORD";

/* ================= USER STORAGE ================= */
#define MAX_USERS 5
struct User {
  String username;
  String password;
};
User users[MAX_USERS];
int userCount = 0;

/* ================= PINS ================= */
#define DHTPIN 4
#define DHTTYPE DHT11
#define SOIL_PIN 35
#define LDR_PIN 27
//#define FAN_PIN 25
#define RELAY_PIN 25
#define RELAY_ON  LOW    // Active LOW relay
#define RELAY_OFF HIGH
//#define PUMP_PIN 27
#define LIGHT_RELAY_PIN  32   
#define LIGHT_RELAY_ON   LOW
#define LIGHT_RELAY_OFF  HIGH
#define RAIN_PIN 26 
#define BUZZER_PIN 33
#define LED_GREEN   14
#define LED_YELLOW  12
#define LED_RED     13


/* ================= GSM ================= */
HardwareSerial sim800(1);
String ALERT_NUMBER = "YOUR_PHONE_NUMBER";

/* ================= FLAGS ================= */
//bool fanState=false;
bool relayState = false;
//bool pumpState=false;
bool lightState=false;
bool tempAlertSent=false;
bool humAlertSent=false;
bool rainDetected = false;
bool buzzerAlert = false;
bool manualFanMode = false;
bool lightRelayState = false;
bool manualLightMode = false;
unsigned long bootTimer = 0;
bool wifiConnecting = true;
bool ipShown = false;

int lastSoilState = -1; 

/* ================= OBJECTS ================= */
DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

/* ================= CROP PROFILE ================= */
struct CropProfile {
  String crop;
  String season;
  float minTemp;
  float maxTemp;
  float minHum;
  float maxHum;
  int minSoil;
};

CropProfile profiles[] = {

  // ================= TOMATO =================
  {"Tomato","Summer",   22,35,50,70,1800},
  {"Tomato","Monsoon",  20,30,65,85,1600},
  {"Tomato","Winter",   15,25,50,70,2000},

  // ================= WHEAT =================
  {"Wheat","Summer",    25,40,40,60,2200},   // not ideal
  {"Wheat","Monsoon",   22,32,60,80,2000},   // risky
  {"Wheat","Winter",    10,25,40,60,1900},   // best

  // ================= CUCUMBER =================
  {"Cucumber","Summer", 25,38,60,80,1700},
  {"Cucumber","Monsoon",22,32,70,90,1500},
  {"Cucumber","Winter", 16,25,55,75,2000},

  // ================= POTATO =================
  {"Potato","Summer",   25,40,45,65,2200},   // not ideal
  {"Potato","Monsoon",  20,30,65,85,1800},
  {"Potato","Winter",   10,25,50,70,1900},   // best

  // ================= APPLE =================
  {"Apple","Summer",    18,30,50,70,2000},
  {"Apple","Monsoon",   15,25,70,90,1700},
  {"Apple","Winter",     2,15,60,80,2100},

  // ================= MANGO =================
  {"Mango","Summer",    24,40,50,70,2300},   // best
  {"Mango","Monsoon",   22,32,65,85,1900},
  {"Mango","Winter",    12,25,50,70,2400},

  // ================= GRAPES =================
  {"Grapes","Summer",   22,38,45,65,2300},
  {"Grapes","Monsoon",  20,30,70,90,1800},
  {"Grapes","Winter",   10,25,50,70,2400}
};


String activeCrop="Tomato";
String activeSeason="Summer";
CropProfile currentProfile;

/* ================= HELPERS ================= */
bool userExists(String u){
  for(int i=0;i<userCount;i++)
    if(users[i].username==u) return true;
  return false;
}

bool validateUser(String u,String p){
  if(u==USERNAME && p==PASSWORD) return true;
  for(int i=0;i<userCount;i++)
    if(users[i].username==u && users[i].password==p) return true;
  return false;
}
/*=========Update Lcd=========*/
void updateLCD(float t, float h, int soil, String light, String rain){
  if(millis() - lastLcdUpdate < 2000) return;
  lastLcdUpdate = millis();

  lcd.clear();

  switch(lcdPage){
    case 0:
      lcd.setCursor(0,0);
      lcd.print("Temp:");
      lcd.print(t,1);
      lcd.print("C");

      lcd.setCursor(0,1);
      lcd.print("Hum:");
      lcd.print(h,0);
      lcd.print("%");
      break;

    case 1:
      lcd.setCursor(0,0);
      lcd.print("Soil:");
      lcd.print(soil == LOW ? "Wet" : "Dry");

      lcd.setCursor(0,1);
      lcd.print("Light:");
      lcd.print(light);
      break;

    case 2:
      lcd.setCursor(0,0);
      lcd.print("Rain Status");
      lcd.setCursor(0,1);
      lcd.print(rain);
      break;
  }

  lcdPage++;
  if(lcdPage > 2) lcdPage = 0;
}
/* ================= SMS ================= */
void sendSMS(String msg){
  sim800.println("AT+CMGF=1");
  delay(500);
  sim800.print("AT+CMGS=\"");
  sim800.print(ALERT_NUMBER);
  sim800.println("\"");
  delay(500);
  sim800.print(msg);
  sim800.write(26);
  delay(3000);
}
/*=========Buzzer Function=======*/
void beepBuzzer(int times){
  for(int i = 0; i < times; i++){
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}
/*==========Led Function=========*/
void updateStatusLEDs(float t, float h, int rain){
  bool danger = false;
  bool warning = false;

  // Danger conditions
  if(t > currentProfile.maxTemp ||
     h < currentProfile.minHum ||
     h > currentProfile.maxHum ||
     rain == LOW){
    danger = true;
  }

  // Warning conditions (near threshold)
  if(t > (currentProfile.maxTemp - 2)){
    warning = true;
  }

  // LED logic
  if(danger){
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN, LOW);
  }
  else if(warning){
    digitalWrite(LED_YELLOW, HIGH);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
  }
  else{
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, LOW);
  }
}

/* ================= LOGIN PAGE ================= */
const char login_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="UTF-8">
<title>Smart Poly House | Login</title>
<style>
body{margin:0;height:100vh;background:linear-gradient(135deg,#020617,#0f172a);
display:flex;align-items:center;justify-content:center;font-family:Segoe UI;color:#e5e7eb;}
.card{width:360px;background:#0f172a;padding:35px;border-radius:18px;
box-shadow:0 0 30px rgba(34,197,94,.25);}
h2{text-align:center;color:#22c55e}
input{width:100%;padding:12px;margin:10px 0;border-radius:10px;
border:1px solid #334155;background:#020617;color:#e5e7eb;}
button{width:100%;padding:12px;border-radius:10px;border:none;
font-weight:bold;font-size:16px;margin-top:10px;}
.login{background:#22c55e;color:black;}
.reg{background:#0ea5e9;color:black;}
.footer{text-align:center;margin-top:15px;color:#94a3b8;font-size:13px}
</style>
</head>
<body>
<div class="card">
<h2>🌱 Smart Poly House</h2>
<input id="user" placeholder="Username">
<input id="pass" type="password" placeholder="Password">
<button class="login" onclick="login()">Login</button>
<button class="reg" onclick="location.href='/registerPage'">Register</button>
<div class="footer">Agri-IoT Monitoring System</div>
</div>

<script>
function login(){
 fetch("/login",{method:"POST",
 headers:{'Content-Type':'application/x-www-form-urlencoded'},
 body:`user=${user.value}&pass=${pass.value}`})
 .then(r=>{if(r.ok)location.href="/dashboard";else alert("Invalid Login")});
}
</script>
</body></html>
)rawliteral";

/* ================= REGISTER PAGE ================= */
const char register_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="UTF-8">
<title>Register</title>
<style>
body{margin:0;height:100vh;background:linear-gradient(135deg,#020617,#0f172a);
display:flex;align-items:center;justify-content:center;font-family:Segoe UI;color:#e5e7eb;}
.card{width:360px;background:#0f172a;padding:35px;border-radius:18px;
box-shadow:0 0 30px rgba(14,165,233,.35);}
h2{text-align:center;color:#0ea5e9}
input{width:100%;padding:12px;margin:10px 0;border-radius:10px;
border:1px solid #334155;background:#020617;color:#e5e7eb;}
button{width:100%;padding:12px;border-radius:10px;border:none;
font-weight:bold;font-size:16px;margin-top:10px;}
.reg{background:#0ea5e9;color:black;}
.back{background:#22c55e;color:black;}
</style>
</head>
<body>
<div class="card">
<h2>📝 New User Registration</h2>
<input id="user" placeholder="Username">
<input id="pass" type="password" placeholder="Password">
<button class="reg" onclick="registerUser()">Register</button>
<button class="back" onclick="location.href='/'">Back</button>
</div>

<script>
function registerUser(){
 fetch("/registerUser",{method:"POST",
 headers:{'Content-Type':'application/x-www-form-urlencoded'},
 body:`user=${user.value}&pass=${pass.value}`})
 .then(r=>{
  if(r.status==200){ alert("Registered! Please login."); location.href="/"; }
  else if(r.status==409) alert("User already exists");
  else alert("Registration failed");
 });
}
</script>
</body></html>
)rawliteral";

/* ================= DASHBOARD ================= */
const char dashboard_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html>
<head>
<meta charset="UTF-8">
<title>Smart Poly House Dashboard</title>
<style>
body{margin:0;background:#020617;font-family:Segoe UI;color:#e5e7eb;}
.container{max-width:1200px;margin:auto;padding:25px;}
h1{text-align:center;color:#22c55e}
.sub{text-align:center;color:#94a3b8;margin-bottom:25px}

.controls{
 display:flex;
 justify-content:center;
 gap:15px;
 margin-bottom:25px;
}

select,button{
 padding:12px 16px;
 border-radius:12px;
 border:1px solid #334155;
 background:#0f172a;
 color:#e5e7eb;
}

/* APPLY BUTTON DEFAULT (BLUE) */
#applyBtn{
 background:#0ea5e9;
 color:black;
 font-weight:bold;
 transition:0.3s;
}

/* APPLY BUTTON AFTER PRESS (GREEN) */
#applyBtn.applied{
 background:#22c55e !important;
 color:black !important;
 box-shadow:0 0 15px rgba(34,197,94,.7);
}

.cards{
 display:grid;
 grid-template-columns:repeat(auto-fit,minmax(220px,1fr));
 gap:20px;
}

.card{
 background:linear-gradient(145deg,#0f172a,#020617);
 padding:22px;
 border-radius:18px;
 box-shadow:0 0 25px rgba(34,197,94,.15);
 text-align:center;
}

.card p{
 font-size:30px;
 font-weight:bold;
 margin-top:10px
}

.actions{
 margin-top:30px;
 display:flex;
 justify-content:center;
 gap:15px;
}

.actions button{
 min-width:180px;
 font-size:16px;
 font-weight:bold;
 border:none;
 cursor:pointer;
}

.on{background:#ef4444;color:white;}
.off{background:#22c55e;color:black;}

.logout{
 margin-top:40px;
 text-align:center;
}

.logout button{
 background:#ef4444;
 color:white;
 padding:12px 30px;
 border-radius:14px;
 border:none;
 margin:5px;
}
</style>
</head>

<body>
<div class="container">
<h1>🌱 Smart Poly House</h1>
<div class="sub">Live Monitoring & Intelligent Control</div>

<div class="controls">
 <select id="crop">
  <option>Tomato</option>
  <option>Cucumber</option>
  <option>Potato</option>
  <option>Wheat</option>
  <option>apple</option>
  <option>Mango</option>
  <option>Grapes</option>
 </select>

 <select id="season">
  <option>Summer</option>
  <option>Winter</option>
  <option>Monsoon</option>
 </select>

 <button id="applyBtn" onclick="applyConfig()">Apply</button>
</div>

<div class="cards">
 <div class="card"><h3>Temperature</h3><p id="temp">--</p></div>
 <div class="card"><h3>Humidity</h3><p id="hum">--</p></div>
 <div class="card"><h3>Soil Moisture</h3><p id="soil">--</p></div>
 <div class="card"><h3>Light</h3><p id="light">--</p></div>
 <div class="card"><h3>Rain</h3><p id="rain">--</p></div>
</div>

<div class="actions">
 <button id="fanBtn" class="off" onclick="toggle('fan')">Fan OFF</button>
 <button id="lightBtn" class="off" onclick="toggle('light')">Light OFF</button>
</div>

<div class="logout">
 <button onclick="downloadReport()">Download Report</button>
 <button onclick="logout()">Logout</button>
</div>
</div>

<script>
let state={fan:false,light:false};
const threshold={temp:{min:18,max:32},hum:{min:50,max:70},soil:{min:1800}};

function toggle(dev){
 state[dev]=!state[dev];
 fetch("/control",{method:"POST",
 headers:{'Content-Type':'application/x-www-form-urlencoded'},
 body:`device=${dev}&state=${state[dev]?'on':'off'}`});
 const b=document.getElementById(dev+"Btn");
 b.className=state[dev]?'on':'off';
 b.innerHTML=dev.charAt(0).toUpperCase()+dev.slice(1)+(state[dev]?' ON':' OFF');
}

function update(){
 fetch("/data").then(r=>r.json()).then(d=>{
  setColor(temp,d.temperature,threshold.temp.min,threshold.temp.max," °C");
  setColor(hum,d.humidity,threshold.hum.min,threshold.hum.max," %");
  setColor(soil,d.soil,threshold.soil.min,99999,"");
  light.innerHTML=d.light;
  rain.innerHTML = d.rain;
  rain.style.color = (d.rain=="Rain Detected") ? "#0ea5e9" : "#22c55e";
 });
}

function setColor(el,val,min,max,unit){
 el.innerHTML=val+unit;
 el.style.color=(val>=min && val<=max)?"#22c55e":"#ef4444";
}

/* APPLY BUTTON CLICK */
function applyConfig(){
 fetch("/config",{method:"POST",
 headers:{'Content-Type':'application/x-www-form-urlencoded'},
 body:`crop=${crop.value}&season=${season.value}`})
 .then(()=>{
  let b=document.getElementById("applyBtn");
  b.classList.add("applied");
  b.innerHTML="Applied ✔";
 });
}

/* RESET APPLY BUTTON WHEN SELECTION CHANGES */
crop.onchange = season.onchange = () => {
 let b=document.getElementById("applyBtn");
 b.classList.remove("applied");
 b.innerHTML="Apply";
};

function downloadReport(){
 fetch("/data").then(r=>r.json()).then(d=>{
  let report =
   "SMART POLY HOUSE REPORT\n\n"+
   "Crop: "+crop.value+"\n"+
   "Season: "+season.value+"\n\n"+
   "Temperature: "+d.temperature+" C\n"+
   "Humidity: "+d.humidity+" %\n"+
   "Soil Moisture: "+d.soil+"\n"+
   "Light: "+d.light+"\n\n"+
   "Fan: "+(state.fan?"ON":"OFF")+"\n"+
   "Light: "+(state.light?"ON":"OFF")+"\n"+
   "Service Provided By AdvanceTech India\n"+
   "Made by Rafe";

  let blob=new Blob([report],{type:"text/plain"});
  let a=document.createElement("a");
  a.href=URL.createObjectURL(blob);
  a.download="PolyHouse_Report.txt";
  a.click();
 });
}

function logout(){location.href="/";}
setInterval(update,2000);update();
</script>
</body>
</html>
)rawliteral";

/* ================= HANDLERS ================= */
void handleLogin(){
 if(validateUser(server.arg("user"),server.arg("pass")))
  server.send(200,"text/plain","OK");
 else server.send(401,"text/plain","FAIL");
}

void handleRegisterUser(){
 String u=server.arg("user");
 String p=server.arg("pass");

 if(u==""||p==""){ server.send(400,"text/plain","EMPTY"); return; }
 if(userExists(u)){ server.send(409,"text/plain","EXISTS"); return; }

 if(userCount<MAX_USERS){
  users[userCount++]={u,p};

  sendSMS("NEW USER REGISTERED\nUsername: "+u);

  server.send(200,"text/plain","REGISTERED");
 }else server.send(500,"text/plain","LIMIT");
}
/* SESSION APPLIED LED Blink */
void showSessionLCD(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Session Applied");

  lcd.setCursor(0,1);
  lcd.print(activeCrop + "-" + activeSeason);

  delay(3000);   // show for 3 sec
}
void sessionAppliedLED(){
  for(int i=0;i<3;i++){
    digitalWrite(LED_GREEN,HIGH);
    digitalWrite(LED_YELLOW,HIGH);
    digitalWrite(LED_RED,HIGH);
    delay(200);

    digitalWrite(LED_GREEN,LOW);
    digitalWrite(LED_YELLOW,LOW);
    digitalWrite(LED_RED,LOW);
    delay(200);
  }
}

/* SESSION APPLY SMS */
void handleConfig(){
 activeCrop=server.arg("crop");
 activeSeason=server.arg("season");
 for(auto &p:profiles)
  if(p.crop==activeCrop && p.season==activeSeason) currentProfile=p;

 sendSMS(
  "SESSION APPLIED\nCrop:"+activeCrop+
  "\nSeason:"+activeSeason+
  "\nTemp:"+String(currentProfile.minTemp)+"-"+String(currentProfile.maxTemp)+
  "\nHum:"+String(currentProfile.minHum)+"-"+String(currentProfile.maxHum)+
  "\nSoil Min:"+String(currentProfile.minSoil)
 );
 sessionAppliedLED();
 showSessionLCD();
 server.send(200,"text/plain","OK");
}

/* MANUAL CONTROL SMS */
void handleControl(){
 String d=server.arg("device");
 String s=server.arg("state");
/*
 if(d=="fan"){
  //digitalWrite(FAN_PIN,s=="on");
  digitalWrite(RELAY_PIN, s=="on" ? RELAY_ON : RELAY_OFF);
  if(s=="on" && !fanState){ sendSMS("Fan ON"); fanState=true; }
  if(s=="off" && fanState){ sendSMS("Fan OFF"); fanState=false; }
 }
*/
if(d == "fan"){
  manualFanMode = true;   //  switch to MANUAL mode

  if(s == "on"){
    digitalWrite(RELAY_PIN, RELAY_ON);
    relayState = true;
    sendSMS("Fan MANUAL ON");
  }
  else{
    digitalWrite(RELAY_PIN, RELAY_OFF);
    relayState = false;
    sendSMS("Fan MANUAL OFF");
  }
}


/*
 if(d=="pump"){
  digitalWrite(PUMP_PIN,s=="on");
  if(s=="on" && !pumpState){ sendSMS("Pump ON"); pumpState=true; }
  if(s=="off" && pumpState){ sendSMS("Pump OFF"); pumpState=false; }
 }
*/
 if(d == "light"){
  manualLightMode = true;

  if(s == "on"){
    digitalWrite(LIGHT_RELAY_PIN, LIGHT_RELAY_ON);
    lightRelayState = true;
    sendSMS("Light MANUAL ON");
  }
  else{
    digitalWrite(LIGHT_RELAY_PIN, LIGHT_RELAY_OFF);
    lightRelayState = false;
    sendSMS("Light MANUAL OFF");
  }
}


 server.send(200,"text/plain","OK");
}

/* AUTO + ALERT SMS */
void handleData(){
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int soil = digitalRead(SOIL_PIN);
  
if(soil != lastSoilState){

  if(soil == HIGH)
    sendSMS("Soil became DRY");

  else
    sendSMS("Soil became WET");

  lastSoilState = soil;
}
  int ldr = digitalRead(LDR_PIN);
  int rain = digitalRead(RAIN_PIN);
  updateStatusLEDs(t, h, rain);

  String soilStatus = (soil == LOW) ? "Wet" : "Dry";
  String light = (ldr == HIGH) ? "Dark" : "Bright";

  //  Rain alert
  if(rain == LOW && !rainDetected){
    sendSMS(" RAIN ALERT: Rain detected in polyhouse!");
    beepBuzzer(5);
    rainDetected = true;
  }
  if(rain == HIGH){
    rainDetected = false;
  }

  // Temperature alert
  if(t > currentProfile.maxTemp && !tempAlertSent){
    sendSMS("TEMP ALERT: " + String(t) + " C");
    beepBuzzer(3);
    tempAlertSent = true;
  }
  if(t <= currentProfile.maxTemp) tempAlertSent = false;

  //  Humidity alert
  if((h < currentProfile.minHum || h > currentProfile.maxHum) && !humAlertSent){
    sendSMS("HUM ALERT: " + String(h) + " %");
    beepBuzzer(3);
    humAlertSent = true;
  }
  if(h >= currentProfile.minHum && h <= currentProfile.maxHum) humAlertSent = false;

  // 🌬️ Fan AUTO (relay)
  // 🌬️ Fan AUTO (relay) — ONLY if not manual
if(!manualFanMode){
  if(t > currentProfile.maxTemp && !relayState){
    digitalWrite(RELAY_PIN, RELAY_ON);
    relayState = true;
    sendSMS("Fan AUTO ON");
  }

  if(t <= currentProfile.maxTemp && relayState){
    digitalWrite(RELAY_PIN, RELAY_OFF);
    relayState = false;
    sendSMS("Fan AUTO OFF");
  }
}
if(manualFanMode && t < (currentProfile.maxTemp - 3)){
  manualFanMode = false;   // back to AUTO control
}
  // 💡 Light AUTO
  // 💡 Light AUTO (relay) — only if not manual
if(!manualLightMode){
  if(ldr == LOW && !lightRelayState){
    digitalWrite(LIGHT_RELAY_PIN, LIGHT_RELAY_ON);
    lightRelayState = true;
    sendSMS("Light AUTO Off");
  }

  if(ldr == HIGH && lightRelayState){
    digitalWrite(LIGHT_RELAY_PIN, LIGHT_RELAY_OFF);
    lightRelayState = false;
    sendSMS("Light AUTO ON");
  }
}

// 💡 Light AUTO control (FIXED)
/*
if(!manualLightMode){

  //  DARK → LIGHT ON
  if(ldr == LOW && !lightRelayState){
    digitalWrite(LIGHT_RELAY_PIN, LIGHT_RELAY_ON); // LOW
    lightRelayState = true;
    sendSMS("Light AUTO ON (Dark)");
  }

  // ☀️ BRIGHT → LIGHT OFF
  if(ldr == HIGH && lightRelayState){
    digitalWrite(LIGHT_RELAY_PIN, LIGHT_RELAY_OFF); // HIGH
    lightRelayState = false;
    sendSMS("Light AUTO OFF (Bright)");
  }
}
*/
// 🔁 Return AUTO mode
if(manualLightMode && ldr == HIGH){
  manualLightMode = false;
}


  String rainStatus = (rain == LOW) ? "Rain Detected" : "No Rain";
  updateLCD(t, h, soil, light, rainStatus);

  //  SEND DATA TO WEBPAGE (THIS WAS MISSING)
  String json = "{";
  json += "\"temperature\":" + String(t) + ",";
  json += "\"humidity\":" + String(h) + ",";
  json += "\"soil\":\"" + soilStatus + "\",";
  json += "\"light\":\"" + light + "\",";
  json += "\"rain\":\"" + rainStatus + "\"";
  json += "}";

  server.send(200, "application/json", json);
}
/*==============Autonomous================*/
void runSystem(){

  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int soil = digitalRead(SOIL_PIN);

  if(soil != lastSoilState){
    if(soil == HIGH) sendSMS("Soil became DRY");
    else sendSMS("Soil became WET");
    lastSoilState = soil;
  }

  int ldr = digitalRead(LDR_PIN);
  int rain = digitalRead(RAIN_PIN);

  updateStatusLEDs(t,h,rain);

  String soilStatus = (soil==LOW)?"Wet":"Dry";
  String light = (ldr==HIGH)?"Dark":"Bright";

  if(rain==LOW && !rainDetected){
    sendSMS("RAIN ALERT!");
    beepBuzzer(3);
    rainDetected=true;
  }
  if(rain==HIGH) rainDetected=false;

  // FAN AUTO
  if(!manualFanMode){
    if(t>currentProfile.maxTemp && !relayState){
      digitalWrite(RELAY_PIN,RELAY_ON);
      relayState=true;
    }
    if(t<=currentProfile.maxTemp && relayState){
      digitalWrite(RELAY_PIN,RELAY_OFF);
      relayState=false;
    }
  }

  // LIGHT AUTO
  if(!manualLightMode){
    if(ldr==LOW && !lightRelayState){
      digitalWrite(LIGHT_RELAY_PIN,LIGHT_RELAY_ON);
      lightRelayState=true;
    }
    if(ldr==HIGH && lightRelayState){
      digitalWrite(LIGHT_RELAY_PIN,LIGHT_RELAY_OFF);
      lightRelayState=false;
    }
  }

  String rainStatus=(rain==LOW)?"Rain Detected":"No Rain";
  updateLCD(t,h,soil,light,rainStatus);
}

/*===========lcd Function===========*/
void setup(){
 Serial.begin(115200);
 dht.begin();
 Wire.begin(21, 22);
 lcd.init();
 lcd.backlight();

 /* Screen 1 */
 lcd.clear();
 lcd.setCursor(0,0);
 lcd.print("  Advance Tech. ");
 lcd.setCursor(0,1);
 lcd.print(" India Pvt.Ltd");
 delay(5000);

 /* Screen 2 */
lcd.clear();
lcd.setCursor(0,0);
lcd.print("Smart Polyhouse");
lcd.setCursor(0,1);
lcd.print("Starting...");



 //pinMode(FAN_PIN,OUTPUT);
 //pinMode(PUMP_PIN,OUTPUT);
 pinMode(RELAY_PIN, OUTPUT);
 pinMode(RAIN_PIN, INPUT);
 pinMode(LDR_PIN, INPUT); 
 pinMode(SOIL_PIN, INPUT);
 pinMode(BUZZER_PIN, OUTPUT);
 digitalWrite(BUZZER_PIN, LOW);   // Buzzer OFF
 digitalWrite(RELAY_PIN, RELAY_OFF);// Fan OFF at boot
 pinMode(LED_GREEN, OUTPUT);
 pinMode(LED_YELLOW, OUTPUT);
 pinMode(LED_RED, OUTPUT);
 pinMode(LIGHT_RELAY_PIN, OUTPUT);
 digitalWrite(LIGHT_RELAY_PIN, LIGHT_RELAY_OFF);
 // default state
 digitalWrite(LED_GREEN, HIGH);   // system OK at boot
 digitalWrite(LED_YELLOW, LOW);
 digitalWrite(LED_RED, LOW);

 sim800.begin(9600,SERIAL_8N1,16,17);
 delay(3000);
 sendSMS("Smart Poly House Started");

WiFi.begin(ssid, password);

lcd.clear();
lcd.setCursor(0,0);
lcd.print("Connecting WiFi");

int timeout = 0;

while(WiFi.status() != WL_CONNECTED && timeout < 20){
  delay(400);
  lcd.setCursor(timeout % 16,1);
  lcd.print(".");
  timeout++;
}

//  WIFI FAILED
if(WiFi.status() != WL_CONNECTED){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   WiFi NOT   ");
  lcd.setCursor(0,1);
  lcd.print("  Connected  ");
  delay(3000);

  espIP = "0.0.0.0";
}

//  WIFI SUCCESS
else{
  espIP = WiFi.localIP().toString();

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" WiFi Connected ");

  lcd.setCursor(0,1);
  lcd.print(espIP);

  delay(5000);
}


espIP = WiFi.localIP().toString();
Serial.print("ESP32 IP Address: ");
Serial.println(espIP);

//  SHOW IP ON LCD
lcd.clear();
lcd.setCursor(0,0);
lcd.print("ESP IP Address");
lcd.setCursor(0,1);
lcd.print(espIP);
delay(4000);

 currentProfile=profiles[0];

 server.on("/",[](){server.send_P(200,"text/html",login_html);});
 server.on("/registerPage",[](){server.send_P(200,"text/html",register_html);});
 server.on("/dashboard",[](){server.send_P(200,"text/html",dashboard_html);});

 server.on("/login",HTTP_POST,handleLogin);
 server.on("/registerUser",HTTP_POST,handleRegisterUser);
 server.on("/data",handleData);
 server.on("/control",HTTP_POST,handleControl);
 server.on("/config",HTTP_POST,handleConfig);

 server.begin();
}

void loop(){

  server.handleClient();
  runSystem();

  // ===== WIFI CONNECT CHECK =====
  if(wifiConnecting && WiFi.status()==WL_CONNECTED){
    wifiConnecting=false;
    ipShown=true;
    bootTimer=millis();

    espIP = WiFi.localIP().toString();

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("WiFi Connected");
    lcd.setCursor(0,1);
    lcd.print(espIP);
    
    delay(4000);
  }

  // ===== SHOW IP FOR 5 SEC =====
  if(ipShown && millis()-bootTimer>5000){
    ipShown=false;
    lcd.clear();
  }
}

