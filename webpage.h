#ifndef WEBPAGE_H
#define WEBPAGE_H

const char webpage_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="cs">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>SmartGreenhouse 🌿</title>
  <style>
    :root {
      --bg-color: #0d1117;
      --glass-bg: rgba(255, 255, 255, 0.05);
      --glass-border: rgba(255, 255, 255, 0.1);
      --text-main: #f0f6fc;
      --text-dim: #8b949e;
      --accent-temp: #ff7b72;
      --accent-humid: #79c0ff;
      --accent-soil: #d2a8ff;
      --accent-water: #a5d6ff;
      --success: #238636;
      --danger: #da3633;
    }
    
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Segoe UI', system-ui, sans-serif; }
    
    body {
      background-color: var(--bg-color);
      background-image: radial-gradient(circle at top right, rgba(46, 160, 67, 0.15), transparent 40%),
                        radial-gradient(circle at bottom left, rgba(88, 166, 255, 0.1), transparent 40%);
      color: var(--text-main);
      min-height: 100vh;
      padding: 2rem;
      display: flex;
      flex-direction: column;
      align-items: center;
    }

    .header { text-align: center; margin-bottom: 3rem; }
    .header h1 { font-size: 2.5rem; font-weight: 300; letter-spacing: 1px; }
    .header p { color: var(--text-dim); margin-top: 0.5rem; }

    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 1.5rem; width: 100%; max-width: 1000px; }

    .card {
      background: var(--glass-bg);
      backdrop-filter: blur(12px);
      -webkit-backdrop-filter: blur(12px);
      border: 1px solid var(--glass-border);
      border-radius: 20px;
      padding: 2rem;
      display: flex;
      flex-direction: column;
      align-items: center;
      transition: transform 0.3s ease, box-shadow 0.3s ease;
      position: relative;
      overflow: hidden;
    }
    .card:hover { transform: translateY(-5px); box-shadow: 0 10px 30px rgba(0,0,0,0.5); }
    
    .card::before {
      content: '';
      position: absolute; top: 0; left: 0; right: 0; height: 4px;
      background: var(--card-accent);
      opacity: 0.8;
    }

    .card.temp { --card-accent: var(--accent-temp); }
    .card.humid { --card-accent: var(--accent-humid); }
    .card.soil { --card-accent: var(--accent-soil); }
    .card.water { --card-accent: var(--accent-water); }

    .card-icon { font-size: 2.5rem; margin-bottom: 1rem; }
    .card-title { font-size: 1.1rem; color: var(--text-dim); text-transform: uppercase; letter-spacing: 1px; margin-bottom: 0.5rem; }
    .card-value { font-size: 3.5rem; font-weight: 600; display: flex; align-items: baseline; gap: 5px; }
    .card-unit { font-size: 1.5rem; font-weight: 400; color: var(--text-dim); }

    .controls-panel {
      width: 100%; max-width: 1000px;
      margin-top: 2rem;
      background: var(--glass-bg);
      backdrop-filter: blur(12px);
      border: 1px solid var(--glass-border);
      border-radius: 20px;
      padding: 2rem;
    }
    .controls-header { margin-bottom: 1.5rem; font-size: 1.5rem; font-weight: 300; }
    
    .btn-group { display: flex; gap: 1rem; flex-wrap: wrap; }
    
    button {
      flex: 1; min-width: 200px;
      padding: 1rem 2rem;
      border: none; border-radius: 12px;
      font-size: 1.1rem; font-weight: 600;
      cursor: pointer;
      display: flex; align-items: center; justify-content: center; gap: 10px;
      transition: all 0.2s ease;
      position: relative; overflow: hidden;
    }
    
    .btn-pump { background-color: rgba(35, 134, 54, 0.2); border: 1px solid var(--success); color: #fff; }
    .btn-pump:hover { background-color: rgba(35, 134, 54, 0.4); }
    .btn-pump.active { background-color: var(--success); }
    
    .btn-window { background-color: rgba(88, 166, 255, 0.1); border: 1px solid rgba(88, 166, 255, 0.5); color: #fff; }
    .btn-window:hover { background-color: rgba(88, 166, 255, 0.2); }

    .badge {
      padding: 5px 12px; border-radius: 20px; font-size: 0.85rem; font-weight: 600;
      background: rgba(255,255,255,0.1); margin-top: 10px;
    }
    .badge.ok { background: rgba(35, 134, 54, 0.2); color: #3fb950; border: 1px solid #3fb950;}
    .badge.warn { background: rgba(218, 54, 51, 0.2); color: #ff7b72; border: 1px solid #ff7b72;}

    @keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.5; } 100% { opacity: 1; } }
    .pulsing { animation: pulse 2s infinite; }

  </style>
</head>
<body>

  <div class="header">
    <h1>SmartGreenhouse 🌿</h1>
    <p>Osobní chytrý skleník řízený přes Wemos D1</p>
  </div>

  <div class="grid">
    <div class="card temp">
      <div class="card-icon">🌡️</div>
      <div class="card-title">Vzduch - Teplota</div>
      <div class="card-value"><span id="val-temp">--</span><span class="card-unit">°C</span></div>
      <div class="badge" id="state-window">Okna Zavřena</div>
    </div>

    <div class="card humid">
      <div class="card-icon">💧</div>
      <div class="card-title">Vzduch - Vlhkost</div>
      <div class="card-value"><span id="val-humid">--</span><span class="card-unit">%</span></div>
    </div>

    <div class="card soil">
      <div class="card-icon">🌱</div>
      <div class="card-title">Půda - Vlhkost</div>
      <div class="card-value"><span id="val-soil">--</span><span class="card-unit">%</span></div>
      <div class="badge" id="state-pump">Čerpadlo Vypnuto</div>
    </div>

    <div class="card water">
      <div class="card-icon">🚰</div>
      <div class="card-title">Zásobník Vody</div>
      <div class="card-value"><span id="val-water-lvl" style="font-size:2rem; font-weight:400;">Načítám...</span></div>
      <div class="badge" id="state-water-badge">Stav neznámý</div>
    </div>
  </div>

  <div class="controls-panel">
    <h2 class="controls-header">Manuální Řízení 🎛️</h2>
    <div class="btn-group">
      <button class="btn-pump" id="btn-water" onclick="togglePump()">
        💦 Zalejt Květiny 
      </button>
      <button class="btn-window" id="btn-window-toggle" onclick="toggleWindow()">
        💨 Přepnout Okno (Větrání)
      </button>
      <button class="btn-window" onclick="toggleAutoMode()" id="btn-auto-mode" style="border-color:#d2a8ff; color:#d2a8ff;">
        🤖 Automatický Režim: ZAPNUT
      </button>
    </div>
  </div>

  <script>
    let isPumpOn = false;
    let isWindowOpen = false;
    let autoMode = true;

    // Fetch data from ESP8266 periodically
    async function updateData() {
      try {
        const response = await fetch('/api/data');
        const data = await response.json();
        
        document.getElementById('val-temp').innerText = data.temperature;
        document.getElementById('val-humid').innerText = data.humidity;
        document.getElementById('val-soil').innerText = data.soilHumidity;
        
        // Plovák state update (data.waterLevelOk is boolean)
        const waterValEl = document.getElementById('val-water-lvl');
        const waterBadgeEl = document.getElementById('state-water-badge');
        if(data.waterLevelOk) {
          waterValEl.innerHTML = "Dostatek 🌊";
          waterValEl.style.color = "var(--text-main)";
          waterBadgeEl.className = "badge ok";
          waterBadgeEl.innerText = "Bezpečné k čerpání";
        } else {
          waterValEl.innerHTML = "Prázdná! ⚠️";
          waterValEl.style.color = "var(--danger)";
          waterBadgeEl.className = "badge warn pulsing";
          waterBadgeEl.innerText = "Doplňte vodu do nádrže!";
        }

        // Pump state update
        const pumpBadgeEl = document.getElementById('state-pump');
        isPumpOn = data.pumpState;
        if(isPumpOn) {
            pumpBadgeEl.innerText = "Aktuálně Zalévám! 💦";
            pumpBadgeEl.className = "badge ok pulsing";
        } else {
            pumpBadgeEl.innerText = "Čerpadlo Vypnuto";
            pumpBadgeEl.className = "badge";
        }

        // Window state update
        const windowBadge = document.getElementById('state-window');
        isWindowOpen = data.windowState;
        if(isWindowOpen) {
            windowBadge.innerText = "Okno Otevřeno 💨";
            windowBadge.className = "badge ok";
        } else {
            windowBadge.innerText = "Okno Zavřeno";
            windowBadge.className = "badge";
        }

        autoMode = data.autoMode;
        const autoBtn = document.getElementById('btn-auto-mode');
        if(autoMode) {
            autoBtn.innerText = "🤖 Automatický Režim: ZAPNUT";
            autoBtn.style.backgroundColor = "rgba(210, 168, 255, 0.1)";
        } else {
            autoBtn.innerText = "⚙️ Automatický Režim: VYPNUT";
            autoBtn.style.backgroundColor = "transparent";
        }

      } catch (e) {
        console.error("Error fetching data:", e);
      }
    }

    async function togglePump() {
      await fetch('/api/pump/toggle', { method: 'POST' });
      updateData();
    }

    async function toggleWindow() {
      await fetch('/api/window/toggle', { method: 'POST' });
      updateData();
    }

    async function toggleAutoMode() {
      await fetch('/api/automode/toggle', { method: 'POST' });
      updateData();
    }

    // Polling every 2 seconds
    setInterval(updateData, 2000);
    updateData(); // Initial call
  </script>
</body>
</html>
)=====";

#endif
