# Atelier ESP32-S3 — Interface LVGL 7"

Interface tactile pour la gestion d'atelier via Home Assistant.

## Matériel
- Waveshare ESP32-S3 Touch LCD 7" (800×480, tactile capacitif)
- Raspberry Pi 4 + Home Assistant + Mosquitto (MQTT)

## Structure
- `src/main.cpp` — WiFi, MQTT, initialisation écran + LVGL
- `src/ui.cpp` — les 4 onglets (Accueil, Relais, Mesures, Scénarios)
- `include/config.h` — identifiants WiFi/MQTT à renseigner
- `lv_conf.h` — configuration LVGL
- `simulator/` — simulateur PC SDL2 pour tester l'UI sans matériel

## Topics MQTT
| Topic | Direction | Format |
|---|---|---|
| `atelier/relais/set` | ESP32 → HA | `{"relais":1,"state":true}` |
| `atelier/relais/state` | HA → ESP32 | `{"1":true,"2":false,...}` |
| `atelier/courant` | HA → ESP32 | `12.45` (ampères) |
| `atelier/scenario/set` | ESP32 → HA | `"Mode Travail"` |
| `atelier/esp32/status` | ESP32 → HA | `online` (retained) |

## Build embarqué (ESP32-S3)
```bash
pio run -t upload
pio device monitor
```

## Simulateur PC (SDL2) — test sans matériel
```bash
sudo apt install libsdl2-dev liblvgl-dev
cd simulator
cmake -B build && cmake --build build
./build/atelier_sim
```

## À faire côté Home Assistant
1. Installer l'add-on **Mosquitto broker**
2. Créer un utilisateur MQTT `atelier`
3. Exposer le capteur Zigbee de courant sur `atelier/courant`
4. Créer un `input_text`/automation qui publie l'état des 8 relais sur `atelier/relais/state`
5. Créer les automatisations de scénarios qui écoutent `atelier/scenario/set`
