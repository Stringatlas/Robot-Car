# ESP32 Robot Car - Project Structure

This project uses a modular architecture for an ESP32-S3 based robot car with wheel encoders, OTA updates, and web interface.

## 📁 File Structure

```
Robot Car/
├── platformio.ini          # PlatformIO configuration
├── README.md              # This file
├── include/
│   └── config.h           # WiFi, OTA, and hardware pin configuration
├── src/
│   ├── main.cpp           # Main program entry point
│   ├── Encoder.h          # Encoder class header
│   ├── Encoder.cpp        # Encoder class implementation
│   ├── WebServer.h        # Web server class header
│   └── WebServer.cpp      # Web server class implementation
└── data/                  # Web interface files (served via LittleFS)
    ├── index.html         # Main encoder dashboard
    ├── script.js          # JavaScript (backup from ESP32Test)
    └── style.css          # CSS styling
```

## 🔧 Configuration

### 1. Update WiFi Credentials
Edit `include/config.h`:
```cpp
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
```

### 2. Configure Encoder Pins
Edit `include/config.h`:
```cpp
#define LEFT_ENCODER_A 4
#define LEFT_ENCODER_B 15
#define RIGHT_ENCODER_A 16
#define RIGHT_ENCODER_B 17
```

### 3. Adjust Encoder Settings
```cpp
#define ENCODER_PPR 960        // Pulses per revolution (240 PPR × 4)
#define WHEEL_DIAMETER 6.5     // Wheel diameter in cm
```

## 🚀 Building & Uploading

### Build the firmware:
```bash
pio run
```

### Upload firmware via USB:
```bash
pio run --target upload
```

### Upload filesystem (web files):
```bash
pio run --target uploadfs
```

### Upload via OTA (after first USB upload):
```bash
pio run --target upload --upload-port <IP_ADDRESS>
```

## 📡 Features

### ✅ Encoder Class
- **Location**: `src/Encoder.h` & `src/Encoder.cpp`
- **Features**:
  - Quadrature decoding (4x resolution)
  - Real-time velocity calculation
  - Distance tracking
  - RPM measurement
  - Thread-safe ISR handling

**Usage Example**:
```cpp
Encoder leftEncoder(PIN_A, PIN_B, PPR, WHEEL_DIA);
leftEncoder.begin();
leftEncoder.update();  // Call regularly

long count = leftEncoder.getCount();
float distance = leftEncoder.getDistance();  // cm
float velocity = leftEncoder.getVelocity();  // cm/s
float rpm = leftEncoder.getRPM();
```

### ✅ Web Server with OTA
- **Location**: `src/WebServer.h` & `src/WebServer.cpp`
- **Features**:
  - Async web server on port 80
  - LittleFS filesystem for HTML/CSS/JS
  - REST API endpoints
  - File upload for OTA web updates

**API Endpoints**:
- `GET /` - Main dashboard
- `GET /api/encoders` - JSON encoder data
- `POST /api/reset` - Reset encoder counts
- `POST /upload` - Upload new web files

### ✅ OTA Updates
- **Hostname**: `ESP32-RobotCar.local`
- **Password**: `admin123` (change in `config.h`)
- Update firmware or filesystem wirelessly via Arduino IDE or PlatformIO

## 🌐 Web Interface

After connecting to WiFi, access the dashboard at:
```
http://<ESP32_IP_ADDRESS>
```

The web interface displays:
- Real-time encoder counts
- Revolutions
- Distance traveled (cm)
- Velocity (cm/s)
- RPM
- Reset button

## 🔌 Hardware Connections

### ESP32-S3-DevKitM-1 Pinout:
```
Left Encoder:
  - Channel A → GPIO 4
  - Channel B → GPIO 15
  - VCC → 3.3V
  - GND → GND

Right Encoder:
  - Channel A → GPIO 16
  - Channel B → GPIO 17
  - VCC → 3.3V
  - GND → GND
```

## 📊 Serial Monitor Output

The system prints encoder data every 500ms:
```
=== ESP32 Robot Car ===
Initializing...

✓ Encoders initialized
✓ WiFi Connected!
IP Address: 192.168.1.100
✓ OTA ready
✓ Web server started

=== System Ready ===

--- Encoder Data ---
Left:  Count: 120 | Distance: 2.45 cm | Velocity: 5.23 cm/s | RPM: 15.2
Right: Count: 118 | Distance: 2.41 cm | Velocity: 5.10 cm/s | RPM: 14.9
```

## 📝 Next Steps

Future additions could include:
- Motor control class (PWM speed control)
- PID controller for velocity/position
- WebSocket for real-time updates
- Odometry calculations (position tracking)
- Motion control (move forward/backward/turn)

## 🐛 Troubleshooting

### ESP32 not detected:
1. Hold BOOT button while plugging in USB
2. Try a different USB cable (must support data)
3. Check: `ls /dev/tty.*` or `pio device list`

### WiFi connection fails:
- Verify SSID/password in `config.h`
- ESP32-S3 only supports 2.4GHz WiFi

### Web page not loading:
- Upload filesystem: `pio run --target uploadfs`
- Check Serial Monitor for IP address
- Verify LittleFS mounted successfully

## 📦 Dependencies

All dependencies are auto-installed via PlatformIO:
- ESP Async WebServer (^1.2.3)
- AsyncTCP (^1.1.1)
- ArduinoOTA (^1.0)

---

**Board**: ESP32-S3-DevKitM-1  
**Platform**: Espressif32  
**Framework**: Arduino
