/*
   NODE 3 – ADVANCED UART RECEIVER + GAUGES + LIVE GRAPH
   ------------------------------------------------------
   - UART RX on Serial2 (RX=26, TX=25)
   - Expects packets: TEMP|External:68.7F|Internal:105.0F
   - Parses External and Internal temperatures
   - Converts to Celsius on ESP32
   - Serves a web dashboard with:
       * Two circular gauges (External / Internal)
       * Live line graph (last ~60 seconds)
       * Last update age
   - Uses AJAX (fetch /data) for smooth updates
*/

#include <WiFi.h>
#include <WebServer.h>

// Temperature state
String externalF = "NaN";
String internalF = "NaN";
String externalC = "NaN";
String internalC = "NaN";
unsigned long lastUpdateMs = 0;

// UART buffer
String buffer = "";

// WiFi credentials
const char* ssid     = "God";
const char* password = "Nicholai2";

WebServer server(80);

// -----------------------------
// UART READING AND PARSING
// -----------------------------
void processPacket(String packet) {
  Serial.println("Received Packet: " + packet);

  // Expected: TEMP|External:68.7F|Internal:105.0F
  int extStart = packet.indexOf("External:") + 9;
  int extEnd   = packet.indexOf("F", extStart);

  int intStart = packet.indexOf("Internal:") + 9;
  int intEnd   = packet.indexOf("F", intStart);

  if (extStart > 8 && extEnd > extStart) {
    externalF = packet.substring(extStart, extEnd);
    externalC = String((externalF.toFloat() - 32.0) * 5.0 / 9.0, 2);
  }

  if (intStart > 8 && intEnd > intStart) {
    internalF = packet.substring(intStart, intEnd);
    internalC = String((internalF.toFloat() - 32.0) * 5.0 / 9.0, 2);
  }

  lastUpdateMs = millis();

  Serial.println("Parsed External (F): " + externalF);
  Serial.println("Parsed Internal (F): " + internalF);
  Serial.println();
}

void readUART() {
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n') {
      if (buffer.length() > 0) {
        processPacket(buffer);
      }
      buffer = "";
    } else {
      buffer += c;
    }
  }
}

// -----------------------------
// HTML PAGE WITH GAUGES + GRAPH
// -----------------------------
String htmlPage() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<title>ESP32 Temperature Dashboard</title>";

  // CSS
  html += "<style>";
  html += "body{margin:0;padding:0;font-family:Arial,Helvetica,sans-serif;";
  html += "background:linear-gradient(135deg,#001a4d,#000814);";
  html += "color:#ffffff;text-align:center;overflow-x:hidden;transition:background 0.6s;}";

  // Party mode background
  html += "body.party-mode{animation:partyBg 5s linear infinite;}";
  html += "@keyframes partyBg{";
  html += "0%{background:linear-gradient(135deg,#001a4d,#000814);}";
  html += "25%{background:linear-gradient(135deg,#4a148c,#0d47a1);}";
  html += "50%{background:linear-gradient(135deg,#b71c1c,#880e4f);}";
  html += "75%{background:linear-gradient(135deg,#1b5e20,#0d47a1);}";
  html += "100%{background:linear-gradient(135deg,#001a4d,#000814);}";
  html += "}";

  html += ".title{margin-top:20px;font-size:40px;font-weight:bold;text-shadow:0 0 10px rgba(0,0,0,0.7);}";

  // Party button
  html += ".btn{margin-top:10px;padding:8px 18px;border-radius:999px;border:none;";
  html += "cursor:pointer;font-size:14px;font-weight:bold;}";
  html += ".btn-party{background:linear-gradient(90deg,#ff4081,#7c4dff,#40c4ff);color:#ffffff;";
  html += "box-shadow:0 0 10px rgba(0,0,0,0.5);}";
  html += ".btn-party.on{box-shadow:0 0 18px rgba(255,255,255,0.9);}";

  html += ".gauges{display:flex;justify-content:center;gap:40px;margin-top:25px;flex-wrap:wrap;}";

  // Gauge layout: dial on top, text underneath
  html += ".gauge{width:220px;display:flex;flex-direction:column;align-items:center;gap:8px;}";
  html += ".gauge-dial{width:200px;height:200px;border-radius:50%;background:#002b80;";
  html += "box-shadow:0 0 20px #0040ff;position:relative;display:flex;align-items:center;";
  html += "justify-content:center;}";
  html += ".gauge-label{font-size:18px;}";
  html += ".gauge-value{font-size:32px;font-weight:bold;}";
  html += ".gauge-sub{font-size:16px;opacity:0.9;}";

  // Needle – pivot from center of dial
  html += ".needle{position:absolute;width:6px;height:90px;background:#ffdf00;";
  html += "top:50%;left:50%;transform-origin:50% 100%;";
  html += "transform:translate(-50%,-100%) rotate(-120deg);";
  html += "border-radius:3px;box-shadow:0 0 8px #ffdf00;}";
  html += ".center-dot{position:absolute;width:16px;height:16px;background:#ffffff;";
  html += "border-radius:50%;top:50%;left:50%;transform:translate(-50%,-50%);}";

  // Graph container
  html += ".graph-container{margin:35px auto;width:90%;max-width:900px;background:#00163a;";
  html += "border-radius:20px;padding:20px;box-shadow:0 0 20px #002a80;}";
  html += "#graph{width:100%;height:300px;background:#000d26;border-radius:12px;cursor:crosshair;}";
  html += ".footer{margin-top:15px;font-size:14px;opacity:0.9;}";

  // Warm glow overlay (for heating up)
  html += ".temp-glow{position:fixed;top:0;left:0;width:100%;height:100%;";
  html += "pointer-events:none;z-index:4;display:none;";
  html += "background:radial-gradient(circle at 50% 100%,rgba(255,183,77,0.45) 0,";
  html += "rgba(255,111,0,0.22) 35%,rgba(0,0,0,0) 75%);";
  html += "mix-blend-mode:screen;animation:warmPulse 1.5s ease-in-out infinite;}";
  html += ".temp-glow.show{display:block;}";

  html += "@keyframes warmPulse{";
  html += "0%{opacity:0.4;}";
  html += "50%{opacity:0.9;}";
  html += "100%{opacity:0.4;}";
  html += "}";

  // Snow overlay
  html += ".temp-snow{position:fixed;top:0;left:0;width:100%;height:100%;";
  html += "pointer-events:none;z-index:5;display:none;overflow:hidden;}";
  html += ".temp-snow.show{display:block;}";

  html += ".snow-piece{position:absolute;width:10px;height:10px;border-radius:50%;";
  html += "background:radial-gradient(circle,#ffffff 0,#e0f7ff 40%,rgba(224,247,255,0) 70%);";
  html += "box-shadow:0 0 10px rgba(224,247,255,0.9);";
  html += "opacity:0.9;animation:snowFall 4s linear infinite;}";

  html += "@keyframes snowFall{";
  html += "0%{transform:translateY(-10vh) translateX(0);}";
  html += "100%{transform:translateY(110vh) translateX(25px);}";
  html += "}";

  // Party confetti
  html += ".party-confetti{position:fixed;top:0;left:0;width:100%;height:100%;";
  html += "pointer-events:none;z-index:7;display:none;overflow:hidden;}";
  html += ".party-confetti.show{display:block;}";

  html += ".confetti-piece{position:absolute;width:9px;height:18px;border-radius:2px;";
  html += "opacity:0.95;animation:confettiFall 3.5s linear infinite;}";

  html += "@keyframes confettiFall{";
  html += "0%{transform:translateY(-10vh) rotate(0deg);}";
  html += "100%{transform:translateY(110vh) rotate(360deg);}";
  html += "}";

  // Graph tooltip
  html += ".graph-tooltip{position:fixed;min-width:110px;";
  html += "background:rgba(0,0,40,0.95);border-radius:8px;padding:6px 10px;";
  html += "font-size:12px;color:#ffffff;pointer-events:none;z-index:10;";
  html += "display:none;box-shadow:0 0 10px rgba(0,0,0,0.6);text-align:left;}";
  html += "</style>";

  // JavaScript
  html += "<script>";
  html += "let extData=[], intData=[], labels=[];";
  html += "let history=[], lastEffectTime=0;";
  html += "const JUMP_DEG=5;";            // temp jump threshold (°F)
  html += "const WINDOW_MS=5000;";        // 5 second window
  html += "const EFFECT_COOLDOWN=8000;";  // at least 8s between effects
  html += "const EFFECT_DURATION=5000;";  // effect visible for 5s";
  html += "let partyOn=false;";
  html += "let hoverIndex=null;";

  // Particle creation helpers
  html += "function createSnowPieces(){";
  html += "const container=document.getElementById('snowEffect');";
  html += "if(!container)return;";
  html += "const count=45;";
  html += "for(let i=0;i<count;i++){";
  html += "const d=document.createElement('div');";
  html += "d.className='snow-piece';";
  html += "d.style.left=(Math.random()*100)+'%';";
  html += "d.style.animationDuration=(3+Math.random()*2)+'s';";
  html += "d.style.animationDelay=(-Math.random()*3)+'s';";
  html += "container.appendChild(d);";
  html += "}";
  html += "}";

  html += "function createPartyConfetti(){";
  html += "const container=document.getElementById('partyConfetti');";
  html += "if(!container)return;";
  html += "const colors=['#ff4081','#ffeb3b','#40c4ff','#69f0ae','#7c4dff'];";
  html += "const count=60;";
  html += "for(let i=0;i<count;i++){";
  html += "const d=document.createElement('div');";
  html += "d.className='confetti-piece';";
  html += "d.style.left=(Math.random()*100)+'%';";
  html += "d.style.backgroundColor=colors[Math.floor(Math.random()*colors.length)];";
  html += "d.style.animationDuration=(2.5+Math.random()*2)+'s';";
  html += "d.style.animationDelay=(-Math.random()*3)+'s';";
  html += "container.appendChild(d);";
  html += "}";
  html += "}";

  html += "function initParticles(){";
  html += "createSnowPieces();";
  html += "createPartyConfetti();";
  html += "}";

  // Warm glow (heating) + snow (cooling)
  html += "function triggerWarmGlow(){";
  html += "let now=Date.now();";
  html += "if(now-lastEffectTime<EFFECT_COOLDOWN)return;";
  html += "lastEffectTime=now;";
  html += "let glow=document.getElementById('warmGlow');";
  html += "let snow=document.getElementById('snowEffect');";
  html += "if(!glow||!snow)return;";
  html += "snow.classList.remove('show');";
  html += "glow.classList.add('show');";
  html += "setTimeout(function(){glow.classList.remove('show');},EFFECT_DURATION);";
  html += "}";

  html += "function triggerSnow(){";
  html += "let now=Date.now();";
  html += "if(now-lastEffectTime<EFFECT_COOLDOWN)return;";
  html += "lastEffectTime=now;";
  html += "let glow=document.getElementById('warmGlow');";
  html += "let snow=document.getElementById('snowEffect');";
  html += "if(!glow||!snow)return;";
  html += "glow.classList.remove('show');";
  html += "snow.classList.add('show');";
  html += "setTimeout(function(){snow.classList.remove('show');},EFFECT_DURATION);";
  html += "}";

  // Party mode toggle
  html += "function toggleParty(){";
  html += "partyOn=!partyOn;";
  html += "const b=document.body;";
  html += "const btn=document.getElementById('partyBtn');";
  html += "const pc=document.getElementById('partyConfetti');";
  html += "if(partyOn){";
  html += "b.classList.add('party-mode');";
  html += "pc.classList.add('show');";
  html += "btn.classList.add('on');";
  html += "btn.innerText='Party Mode: ON';";
  html += "}else{";
  html += "b.classList.remove('party-mode');";
  html += "pc.classList.remove('show');";
  html += "btn.classList.remove('on');";
  html += "btn.innerText='Party Mode';";
  html += "}";
  html += "}";

  html += "function fetchData(){";
  html += "fetch('/data').then(r=>r.json()).then(d=>{";
  html += "document.getElementById('extF').innerText = d.extF + ' °F';";
  html += "document.getElementById('intF').innerText = d.intF + ' °F';";
  html += "document.getElementById('extC').innerText = d.extC + ' °C';";
  html += "document.getElementById('intC').innerText = d.intC + ' °C';";
  html += "document.getElementById('age').innerText = 'Last Update: ' + d.age + ' sec ago';";

  // Gauge update
  html += "let extF = parseFloat(d.extF);";
  html += "let intF = parseFloat(d.intF);";
  html += "updateGauge('extNeedle', extF);";
  html += "updateGauge('intNeedle', intF);";

  // Graph + jump detection
  html += "if(!isNaN(extF) && !isNaN(intF)){";
  html += "let now=Date.now();";
  html += "history.push({t:now,ext:extF,int:intF});";
  html += "while(history.length && now-history[0].t>WINDOW_MS){history.shift();}";
  html += "if(history.length>1){";
  html += "let old=history[0];";
  html += "let dExt=extF-old.ext;";
  html += "let dInt=intF-old.int;";
  html += "if(dExt>=JUMP_DEG || dInt>=JUMP_DEG){triggerWarmGlow();}";
  html += "else if(dExt<=-JUMP_DEG || dInt<=-JUMP_DEG){triggerSnow();}";
  html += "}";

  html += "extData.push(extF); intData.push(intF);";
  html += "if(labels.length==0) labels.push(0); else labels.push(labels[labels.length-1]+1);";
  html += "if(extData.length>60){extData.shift();intData.shift();labels.shift();}";
  html += "drawGraph();";
  html += "}";
  html += "});";
  html += "}";

  // Gauge: map 40–120°F to -120 to +120 degrees
  html += "function updateGauge(id,value){";
  html += "let min=40, max=120;";
  html += "if(isNaN(value)) return;";
  html += "if(value<min) value=min; if(value>max) value=max;";
  html += "let frac=(value-min)/(max-min);";
  html += "let angle=-120+frac*240;";
  html += "document.getElementById(id).style.transform='translate(-50%,-100%) rotate('+angle+'deg)';";
  html += "}";

  // Graph drawing
  html += "function drawGraph(){";
  html += "let c=document.getElementById('graph');";
  html += "let ctx=c.getContext('2d');";
  html += "let w=c.width, h=c.height;";
  html += "ctx.clearRect(0,0,w,h);";
  html += "if(extData.length<2) return;";

  // determine min/max
  html += "let all=extData.concat(intData);";
  html += "let min=Math.min.apply(null,all);";
  html += "let max=Math.max.apply(null,all);";
  html += "if(max-min<5){max+=2.5;min-=2.5;}";

  // axes
  html += "ctx.strokeStyle='#335599';ctx.lineWidth=1;";
  html += "ctx.beginPath();ctx.moveTo(40,10);ctx.lineTo(40,h-30);ctx.lineTo(w-10,h-30);ctx.stroke();";

  // y labels
  html += "ctx.fillStyle='#88aaff';ctx.font='12px Arial';";
  html += "ctx.fillText(max.toFixed(1)+' F',5,20);";
  html += "ctx.fillText(min.toFixed(1)+' F',5,h-35);";

  // helper to map value to canvas
  html += "function mapY(v){return (h-30)-((v-min)/(max-min))*(h-40);}";

  // draw series
  html += "let n=extData.length;";
  html += "let step=(w-60)/(n-1);";

  // external line
  html += "ctx.strokeStyle='#4dd0ff';ctx.lineWidth=2;ctx.beginPath();";
  html += "for(let i=0;i<n;i++){let x=40+i*step;let y=mapY(extData[i]);";
  html += "if(i==0)ctx.moveTo(x,y);else ctx.lineTo(x,y);}ctx.stroke();";

  // internal line
  html += "ctx.strokeStyle='#ffb347';ctx.lineWidth=2;ctx.beginPath();";
  html += "for(let i=0;i<n;i++){let x=40+i*step;let y=mapY(intData[i]);";
  html += "if(i==0)ctx.moveTo(x,y);else ctx.lineTo(x,y);}ctx.stroke();";

  // legend
  html += "ctx.fillStyle='#4dd0ff';ctx.fillRect(w-150,15,12,4);";
  html += "ctx.fillStyle='#ffffff';ctx.fillText('External',w-135,22);";
  html += "ctx.fillStyle='#ffb347';ctx.fillRect(w-150,35,12,4);";
  html += "ctx.fillStyle='#ffffff';ctx.fillText('Internal',w-135,42);";

  // hover vertical line
  html += "if(hoverIndex!==null && hoverIndex>=0 && hoverIndex<n){";
  html += "let x=40+hoverIndex*step;";
  html += "ctx.strokeStyle='rgba(255,255,255,0.3)';";
  html += "ctx.setLineDash([4,4]);";
  html += "ctx.beginPath();";
  html += "ctx.moveTo(x,10);";
  html += "ctx.lineTo(x,h-30);";
  html += "ctx.stroke();";
  html += "ctx.setLineDash([]);";
  html += "}";
  html += "}";

  // Graph hover handling
  html += "function setupGraphHover(){";
  html += "const c=document.getElementById('graph');";
  html += "const tooltip=document.getElementById('graphTooltip');";
  html += "if(!c||!tooltip)return;";

  html += "c.addEventListener('mousemove',function(e){";
  html += "if(extData.length<2)return;";
  html += "const rect=c.getBoundingClientRect();";
  html += "const scaleX=c.width/rect.width;";
  html += "let xCanvas=(e.clientX-rect.left)*scaleX;";
  html += "let n=extData.length;";
  html += "let step=(c.width-60)/(n-1);";
  html += "let idx=Math.round((xCanvas-40)/step);";
  html += "if(idx<0)idx=0;";
  html += "if(idx>=n)idx=n-1;";
  html += "hoverIndex=idx;";
  html += "drawGraph();";
  html += "let ext=extData[idx];";
  html += "let inte=intData[idx];";
  html += "if(isNaN(ext)||isNaN(inte))return;";
  html += "tooltip.innerHTML='Sample '+idx+'<br>External: '+ext.toFixed(1)+' °F<br>Internal: '+inte.toFixed(1)+' °F';";
  html += "tooltip.style.left=(e.clientX+12)+'px';";
  html += "tooltip.style.top=(e.clientY-10)+'px';";
  html += "tooltip.style.display='block';";
  html += "});";

  html += "c.addEventListener('mouseleave',function(){";
  html += "hoverIndex=null;";
  html += "drawGraph();";
  html += "const tooltip=document.getElementById(\"graphTooltip\");";
  html += "if(tooltip)tooltip.style.display='none';";
  html += "});";
  html += "}";

  html += "window.onload=function(){";
  html += "initParticles();";
  html += "setupGraphHover();";
  html += "fetchData();";
  html += "setInterval(fetchData,1000);";
  html += "};";

  html += "</script>";

  html += "</head><body>";

  // Warm glow, snow, and party confetti overlays
  html += "<div id='warmGlow' class='temp-glow'></div>";
  html += "<div id='snowEffect' class='temp-snow'></div>";
  html += "<div id='partyConfetti' class='party-confetti'></div>";
  html += "<div id='graphTooltip' class='graph-tooltip'></div>";

  html += "<div class='title'>ESP32 Temperature Dashboard</div>";
  html += "<div><button id='partyBtn' class='btn btn-party' onclick='toggleParty()'>Party Mode</button></div>";

  html += "<div class='gauges'>";

  // External gauge
  html += "<div class='gauge'>";
  html += "<div class='gauge-dial'>";
  html += "<div class='needle' id='extNeedle'></div>";
  html += "<div class='center-dot'></div>";
  html += "</div>";
  html += "<div class='gauge-label'>External Temperature</div>";
  html += "<div class='gauge-value' id='extF'>-- °F</div>";
  html += "<div class='gauge-sub' id='extC'>-- °C</div>";
  html += "</div>";

  // Internal gauge
  html += "<div class='gauge'>";
  html += "<div class='gauge-dial'>";
  html += "<div class='needle' id='intNeedle'></div>";
  html += "<div class='center-dot'></div>";
  html += "</div>";
  html += "<div class='gauge-label'>Internal Temperature</div>";
  html += "<div class='gauge-value' id='intF'>-- °F</div>";
  html += "<div class='gauge-sub' id='intC'>-- °C</div>";
  html += "</div>";

  html += "</div>"; // gauges

  html += "<div class='graph-container'>";
  html += "<canvas id='graph' width='800' height='300'></canvas>";
  html += "</div>";

  html += "<div class='footer' id='age'>Last Update: waiting for data...</div>";

  html += "</body></html>";
  return html;
}

void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

// JSON endpoint for AJAX
void handleDataJson() {
  unsigned long ageSec = 0;
  if (lastUpdateMs == 0) ageSec = 9999;
  else ageSec = (millis() - lastUpdateMs) / 1000;

  String json = "{";
  json += "\"extF\":\"" + externalF + "\",";
  json += "\"intF\":\"" + internalF + "\",";
  json += "\"extC\":\"" + externalC + "\",";
  json += "\"intC\":\"" + internalC + "\",";
  json += "\"age\":" + String(ageSec);
  json += "}";

  server.send(200, "application/json", json);
}

// -----------------------------
// SETUP & LOOP
// -----------------------------
void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 26, 25);  // RX=26, TX=25

  Serial.println("\nNode 3 – Gauges + Graph Dashboard\n");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleDataJson);
  server.begin();
}

void loop() {
  readUART();
  server.handleClient();
}
