#ifndef WIFI_VAULT_SERVER_H
#define WIFI_VAULT_SERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include "BarrierServo.h"
#include "Config.h"
#include "DoorSensor.h"
#include "DoorStatusLed.h"
#include "ObjectMotionSensor.h"
#include "PersistentLog.h"

class WiFiVaultServer {
private:
  DoorSensor &door;
  BarrierServo &barrier;
  DoorStatusLed &doorLed;
  ObjectMotionSensor &objectSensor;
  PersistentLog &persistentLog;
  WebServer server;

  String statusJson() {
    String json = "{";
    json += "\"door\":\"";
    json += door.isOpen() ? "UNLOCKED" : "LOCKED";
    json += "\",\"object\":\"";
    json += objectSensor.isStolen() ? "STOLEN" : "SAFE";
    json += "\",\"barrier\":\"";
    json += barrier.isUnlocked() ? "UNLOCK" : "LOCK";
    json += "\",\"mpu\":\"";
    json += objectSensor.isReady() ? "OK" : "ERROR";
    json += "\",\"rgbMode\":\"";
    json += doorLed.isManualMode() ? "MANUAL" : "AUTO";
    json += "\",\"armed\":";
    json += objectSensor.isArmed() ? "true" : "false";
    json += ",\"connected\":true";
    json += ",\"email_enabled\":true";
    json += ",\"email_status\":\"WiFi alert forwarded to local Flask app\"";
    json += ",\"commands\":";
    json += persistentLog.commandsJson();
    json += ",\"events\":";
    json += persistentLog.eventsJson();
    json += "}";
    return json;
  }

  void sendHtml() {
    server.send(200, "text/html", R"rawliteral(
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Cloudlock</title>
  <style>
    :root{color-scheme:light;--bg:#eef4f4;--panel:rgba(255,255,255,.92);--text:#15171a;--muted:#6d747c;--line:rgba(177,193,207,.82);--red:#d93a32;--green:#1b9c5a;--dark:#25292e}
    *{box-sizing:border-box}
    body{min-height:100vh;margin:0;background:linear-gradient(118deg,rgba(8,72,69,.34) 0%,rgba(8,72,69,0) 32%),linear-gradient(74deg,rgba(10,45,58,.18) 4%,rgba(24,168,111,.34) 32%,rgba(27,139,171,.28) 52%,rgba(74,57,150,.30) 76%,rgba(13,24,44,.16) 100%),linear-gradient(160deg,#dcebea 0%,#cde6dc 27%,#cbdfe9 57%,#d9d3ee 100%);background-attachment:fixed;color:var(--text);font-family:system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif}
    main{width:min(920px,calc(100% - 32px));margin:0 auto;padding:42px 0}
    header{display:flex;justify-content:space-between;gap:20px;align-items:flex-start;margin-bottom:26px}
    h1,h2,p{margin:0} h1{font-size:34px;line-height:1} p{color:var(--muted);margin-top:8px}
    .connection{min-width:132px;padding:10px 14px;border:1px solid var(--line);border-radius:8px;background:var(--panel);color:var(--green);text-align:center;font-size:14px;font-weight:800}
    .status-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:14px}
    .status-panel,.control-section,.details,.logs article,.rgb-section{border:1px solid var(--line);border-radius:8px;background:var(--panel);box-shadow:0 18px 44px rgba(28,55,72,.08);backdrop-filter:blur(12px)}
    .status-panel{min-height:176px;padding:24px;display:flex;flex-direction:column;justify-content:space-between}
    .label{color:var(--muted);font-size:13px;font-weight:800;letter-spacing:.04em;text-transform:uppercase}
    .status-panel strong{display:block;font-size:40px;line-height:1;letter-spacing:0}
    .hint{color:var(--muted);font-size:15px}
    .status-panel.safe strong,.status-panel.open strong{color:var(--green)}
    .status-panel.stolen strong,.status-panel.locked strong{color:var(--red)}
    .control-section,.rgb-section{margin-top:14px;min-height:130px;padding:22px 24px;display:grid;grid-template-columns:1fr auto;align-items:center;gap:24px}
    .control-section h2,.rgb-section h2{margin-top:8px;font-size:22px;letter-spacing:0}
    .switch{position:relative;width:300px;height:58px;display:grid;grid-template-columns:1fr 1fr;padding:5px;border:1px solid var(--line);border-radius:8px;background:#eef1f4}
    .switch button{position:relative;z-index:2;border:0;background:transparent;color:var(--muted);font:inherit;font-size:14px;font-weight:900;cursor:pointer}.switch button.active{color:#fff}
    #switchThumb{position:absolute;top:5px;left:5px;z-index:1;width:calc(50% - 5px);height:calc(100% - 10px);border-radius:6px;background:var(--red);transition:transform 180ms ease,background 180ms ease}.switch.unlock #switchThumb{transform:translateX(100%);background:var(--green)}
    .command-status{grid-column:1/-1;color:var(--muted);font-size:14px}.command-status.ok{color:var(--green)}.command-status.error{color:var(--red)}
    .details{margin-top:14px;display:grid;grid-template-columns:repeat(4,minmax(0,1fr))}.details div{padding:18px 20px;border-right:1px solid var(--line)}.details div:last-child{border-right:0}.details span,.details strong{display:block}.details span{color:var(--muted);font-size:13px;font-weight:800;text-transform:uppercase}.details strong{margin-top:6px;font-size:18px}
    .logs{margin-top:14px;display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:14px}.logs article{min-height:210px;padding:20px}.log-title{margin-bottom:14px}.log-list{list-style:none;padding:0;margin:0;display:grid;gap:8px}.log-list li{min-height:38px;display:flex;align-items:center;justify-content:space-between;gap:10px;padding:9px 10px;border:1px solid var(--line);border-radius:6px;color:var(--dark);font-size:14px;font-weight:700}.log-list .empty{color:var(--muted);font-weight:600}
    .delete-event,.auto-btn{border:1px solid var(--line);border-radius:6px;background:#fff;font:inherit;font-size:12px;font-weight:900;cursor:pointer;padding:5px 8px}.delete-event{color:var(--red)}.auto-btn{color:var(--text);font-size:14px;padding:13px 16px;border-radius:8px}
    .rgb-tools{display:flex;align-items:center;gap:12px}
    input[type=color]{width:72px;height:46px;border:1px solid var(--line);border-radius:8px;background:#fff;padding:4px;cursor:pointer}
    @media(max-width:720px){header,.control-section,.rgb-section{display:flex;flex-direction:column;align-items:stretch}.status-grid,.details,.logs{grid-template-columns:1fr}.details div{border-right:0;border-bottom:1px solid var(--line)}.details div:last-child{border-bottom:0}.switch{width:100%}.rgb-tools{align-items:stretch}.auto-btn,input[type=color]{width:100%}}
  </style>
</head>
<body>
  <main>
    <header>
      <div><h1>Cloudlock</h1><p>Smart Vault control panel</p></div>
      <div id="connection" class="connection">Connected</div>
    </header>
    <section class="status-grid">
      <article class="status-panel">
        <span class="label">Vault door</span>
        <strong id="doorValue">UNKNOWN</strong>
        <span id="doorHint" class="hint">Waiting for MC38</span>
      </article>
      <article class="status-panel">
        <span class="label">Object state</span>
        <strong id="objectValue">UNKNOWN</strong>
        <span id="objectHint" class="hint">Waiting for MPU</span>
      </article>
    </section>
    <section class="control-section">
      <div><span class="label">Barrier control</span><h2>MG90S servo lock</h2></div>
      <div class="switch" role="group" aria-label="Barrier control">
        <button id="lockButton" type="button">LOCK</button>
        <button id="unlockButton" type="button">UNLOCK</button>
        <span id="switchThumb"></span>
      </div>
      <span id="commandStatus" class="command-status">Ready</span>
    </section>
    <section class="rgb-section">
      <div><span class="label">RGB</span><h2>Pick a LED color</h2></div>
      <div class="rgb-tools">
        <input id="rgbPicker" type="color" value="#0066ff">
        <button id="rgbAuto" class="auto-btn">AUTO DOOR</button>
      </div>
    </section>
    <section class="details">
      <div><span>MPU</span><strong id="mpuValue">UNKNOWN</strong></div>
      <div><span>Armed</span><strong id="armedValue">NO</strong></div>
      <div><span>Updated</span><strong id="updatedValue">-</strong></div>
      <div><span>Email</span><strong id="emailValue">OFF</strong></div>
    </section>
    <section class="logs">
      <article><div class="log-title"><span class="label">Last 10 web commands</span></div><ul id="commandLog" class="log-list"></ul></article>
      <article><div class="log-title"><span class="label">Last 10 vault events</span></div><ul id="eventLog" class="log-list"></ul></article>
    </section>
  </main>
  <script>
    const doorValue=document.querySelector("#doorValue"),doorHint=document.querySelector("#doorHint"),objectValue=document.querySelector("#objectValue"),objectHint=document.querySelector("#objectHint"),connection=document.querySelector("#connection");
    const mpuValue=document.querySelector("#mpuValue"),armedValue=document.querySelector("#armedValue"),updatedValue=document.querySelector("#updatedValue"),emailValue=document.querySelector("#emailValue");
    const lockButton=document.querySelector("#lockButton"),unlockButton=document.querySelector("#unlockButton"),switchBox=document.querySelector(".switch"),commandStatus=document.querySelector("#commandStatus"),commandLog=document.querySelector("#commandLog"),eventLog=document.querySelector("#eventLog");
    const rgbPicker=document.querySelector("#rgbPicker"),rgbAuto=document.querySelector("#rgbAuto");
    function setPanelState(e,s){e.classList.remove("safe","stolen","locked","open");e.classList.add(s)}
    function setBarrier(v){const u=v==="UNLOCK";switchBox.classList.toggle("unlock",u);lockButton.classList.toggle("active",!u);unlockButton.classList.toggle("active",u)}
    async function sendBarrier(command){lockButton.disabled=true;unlockButton.disabled=true;commandStatus.textContent=`Sending ${command}...`;commandStatus.className="command-status";try{const r=await fetch("/api/barrier?cmd="+command,{method:"POST"});const d=await r.json();if(!r.ok||!d.ok)throw new Error(d.error||"Command failed");setBarrier(command);commandStatus.textContent=`${command} command sent`;commandStatus.className="command-status ok";refreshStatus()}catch(e){commandStatus.textContent=e.message;commandStatus.className="command-status error"}finally{lockButton.disabled=false;unlockButton.disabled=false}}
    async function setRgb(v){await fetch("/api/rgb?color="+encodeURIComponent(v),{method:"POST"});commandStatus.textContent="RGB color sent";commandStatus.className="command-status ok"}
    async function rgbDoorMode(){await fetch("/api/rgb/auto",{method:"POST"});commandStatus.textContent="RGB auto door mode";commandStatus.className="command-status ok";refreshStatus()}
    async function deleteEvent(index){try{const r=await fetch("/api/events/delete?index="+index,{method:"POST"});const d=await r.json();if(!r.ok||!d.ok)throw new Error(d.error||"Delete failed");refreshStatus()}catch(e){commandStatus.textContent=e.message;commandStatus.className="command-status error"}}
    function renderCommandLog(commands){commandLog.innerHTML="";if(!commands||commands.length===0){commandLog.innerHTML='<li class="empty">No web commands stored</li>';return}commands.forEach(c=>{const i=document.createElement("li");i.textContent=c;commandLog.appendChild(i)})}
    function renderEventLog(events){eventLog.innerHTML="";if(!events||events.length===0){eventLog.innerHTML='<li class="empty">No vault events stored</li>';return}events.forEach((ev,index)=>{const i=document.createElement("li"),t=document.createElement("span"),b=document.createElement("button");t.textContent=ev;b.type="button";b.className="delete-event";b.textContent="Delete";b.onclick=()=>deleteEvent(index);i.appendChild(t);i.appendChild(b);eventLog.appendChild(i)})}
    async function refreshStatus(){
      const r=await fetch("/api/status"); const s=await r.json();
      connection.textContent="Connected";connection.className="connection";
      doorValue.textContent=s.door;doorHint.textContent=s.door==="LOCKED"?"MC38 contact is closed":"MC38 contact is open";setPanelState(doorValue.closest(".status-panel"),s.door==="LOCKED"?"locked":"open");
      objectValue.textContent=s.object;objectHint.textContent=s.object==="STOLEN"?"Alarm condition active":"MPU position is safe";setPanelState(objectValue.closest(".status-panel"),s.object==="STOLEN"?"stolen":"safe");
      mpuValue.textContent=s.mpu;armedValue.textContent=s.armed?"YES":"NO";updatedValue.textContent=new Date().toLocaleTimeString();emailValue.textContent=s.email_enabled?"ON":"OFF";emailValue.title=s.email_status||"";
      setBarrier(s.barrier);
      renderCommandLog(s.commands);renderEventLog(s.events);
    }
    lockButton.onclick=()=>sendBarrier("LOCK"); unlockButton.onclick=()=>sendBarrier("UNLOCK");
    rgbPicker.oninput=()=>setRgb(rgbPicker.value); rgbAuto.onclick=()=>rgbDoorMode();
    refreshStatus(); setInterval(refreshStatus,700);
  </script>
</body>
</html>
)rawliteral");
  }

  void sendStatus() {
    server.send(200, "application/json", statusJson());
  }

  void handleBarrierCommand() {
    String command = server.arg("cmd");
    command.toUpperCase();

    if (command == "LOCK") {
      persistentLog.addCommand("WIFI_LOCK");
      barrier.setUnlocked(false);
      server.send(200, "application/json", "{\"ok\":true,\"command\":\"LOCK\"}");
    } else if (command == "UNLOCK") {
      persistentLog.addCommand("WIFI_UNLOCK");
      barrier.setUnlocked(true);
      server.send(200, "application/json", "{\"ok\":true,\"command\":\"UNLOCK\"}");
    } else {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid command\"}");
    }
  }

  uint8_t hexPairToByte(const String &hex, int startIndex) {
    return strtoul(hex.substring(startIndex, startIndex + 2).c_str(), nullptr, 16);
  }

  void handleRgbCommand() {
    String color = server.arg("color");
    color.trim();

    if (color.length() != 7 || color[0] != '#') {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid color\"}");
      return;
    }

    uint8_t red = hexPairToByte(color, 1);
    uint8_t green = hexPairToByte(color, 3);
    uint8_t blue = hexPairToByte(color, 5);

    doorLed.setManualColor(red, green, blue);
    persistentLog.addCommand("WIFI_RGB_COLOR");
    server.send(200, "application/json", "{\"ok\":true}");
  }

  void handleRgbAuto() {
    doorLed.useDoorMode(door.isOpen());
    persistentLog.addCommand("WIFI_RGB_AUTO");
    server.send(200, "application/json", "{\"ok\":true}");
  }

  void handleDeleteEvent() {
    int index = server.arg("index").toInt();
    if (index < 0 || index > 9) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid event index\"}");
      return;
    }

    persistentLog.deleteEvent(index);
    persistentLog.addCommand("WIFI_DELETE_EVENT");
    server.send(200, "application/json", "{\"ok\":true}");
  }

  const char *wifiStatusText(wl_status_t status) {
    switch (status) {
      case WL_IDLE_STATUS:
        return "IDLE";
      case WL_NO_SSID_AVAIL:
        return "NO_SSID_AVAIL";
      case WL_SCAN_COMPLETED:
        return "SCAN_COMPLETED";
      case WL_CONNECTED:
        return "CONNECTED";
      case WL_CONNECT_FAILED:
        return "CONNECT_FAILED";
      case WL_CONNECTION_LOST:
        return "CONNECTION_LOST";
      case WL_DISCONNECTED:
        return "DISCONNECTED";
      default:
        return "UNKNOWN";
    }
  }

  void scanForConfiguredNetwork() {
    Serial.println("Scan WiFi pentru hotspot...");
    int networks = WiFi.scanNetworks();
    bool found = false;

    if (networks <= 0) {
      Serial.println("Nu am gasit nicio retea WiFi la scanare.");
      return;
    }

    for (int i = 0; i < networks; i++) {
      String ssid = WiFi.SSID(i);
      Serial.print("  Retea gasita: ");
      Serial.print(ssid);
      Serial.print(" / RSSI=");
      Serial.print(WiFi.RSSI(i));
      Serial.print(" dBm / channel=");
      Serial.println(WiFi.channel(i));

      if (ssid == WIFI_STA_SSID) {
        found = true;
        Serial.print("Hotspot gasit: ");
        Serial.print(ssid);
        Serial.print(" / RSSI=");
        Serial.print(WiFi.RSSI(i));
        Serial.print(" dBm / channel=");
        Serial.println(WiFi.channel(i));
      }
    }

    if (!found) {
      Serial.print("Hotspotul configurat NU a fost gasit la scanare: ");
      Serial.println(WIFI_STA_SSID);
      Serial.println("Pe iPhone activeaza Personal Hotspot si Maximize Compatibility.");
    }

    WiFi.scanDelete();
  }

public:
  WiFiVaultServer(DoorSensor &doorSensor, BarrierServo &servo, DoorStatusLed &led, ObjectMotionSensor &mpu, PersistentLog &log)
      : door(doorSensor), barrier(servo), doorLed(led), objectSensor(mpu), persistentLog(log), server(80) {
  }

  void begin() {
    WiFi.persistent(false);
    WiFi.setSleep(false);
    WiFi.disconnect(true);
    delay(500);

    WiFi.mode(WIFI_STA);
    scanForConfiguredNetwork();

    WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASSWORD);
    Serial.print("Conectare la WiFi SSID: ");
    Serial.println(WIFI_STA_SSID);

    unsigned long startMs = millis();
    wl_status_t lastStatus = WL_IDLE_STATUS;
    while (WiFi.status() != WL_CONNECTED && millis() - startMs < 30000) {
      delay(500);
      Serial.print(".");
      wl_status_t currentStatus = WiFi.status();
      if (currentStatus != lastStatus) {
        lastStatus = currentStatus;
        Serial.print(" ");
        Serial.print(wifiStatusText(currentStatus));
        Serial.print(" ");
      }
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("WiFi conectat. IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("Gateway: ");
      Serial.println(WiFi.gatewayIP());
      Serial.print("RSSI: ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");
    } else {
      Serial.print("WiFi STA esuat. Status final: ");
      Serial.println(wifiStatusText(WiFi.status()));
      Serial.println("Serverul web WiFi nu va fi accesibil pana cand ESP32 se conecteaza la hotspot.");
    }

    server.on("/", HTTP_GET, [this]() { sendHtml(); });
    server.on("/api/status", HTTP_GET, [this]() { sendStatus(); });
    server.on("/api/barrier", HTTP_POST, [this]() { handleBarrierCommand(); });
    server.on("/api/rgb", HTTP_POST, [this]() { handleRgbCommand(); });
    server.on("/api/rgb/auto", HTTP_POST, [this]() { handleRgbAuto(); });
    server.on("/api/events/delete", HTTP_POST, [this]() { handleDeleteEvent(); });
    server.begin();
    Serial.println("WiFi web server pornit pe portul 80.");
  }

  void handleClient() {
    server.handleClient();
  }
};

#endif
