#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "ESP32_BOT";     // Your desired network name
const char* password = "12345678";   // Your desired password

// Pin Definitions
const int MOTOR_A_IN1 = 26;
const int MOTOR_A_IN2 = 27;
const int MOTOR_B_IN1 = 14;
const int MOTOR_B_IN2 = 12;
const int MOTOR_A_EN = 25;
const int MOTOR_B_EN = 13;
const int LEFT_SENSOR = 36;    // VP
const int RIGHT_SENSOR = 39;   // VN
const int MODE_SWITCH = 23;

// Constants
const int LINE_THRESHOLD = 2000;
const int MIN_SPEED = 0;
const int MAX_SPEED = 255;

// Variables
int currentSpeed = 150;
bool isLineFollowMode = true;

WebServer server(80);

// HTML for the web interface
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>ESP32 Bot Controller</title>
    <style>
        body {font-family: Arial; text-align: center; margin: 0; padding: 20px; user-select: none;}
        .container {max-width: 600px; margin: 0 auto;}
        .joystick-container {
            width: 300px; height: 300px; 
            background-color: #f0f0f0; 
            border-radius: 50%; 
            margin: 20px auto; 
            position: relative; 
            touch-action: none;
        }
        .joystick {
            width: 100px; height: 100px; 
            background-color: #4CAF50; 
            border-radius: 50%; 
            position: absolute;
            top: 100px; left: 100px;
            cursor: pointer;
        }
        .controls {margin: 20px 0;}
        button {
            padding: 10px 20px;
            margin: 5px;
            font-size: 16px;
            cursor: pointer;
        }
        input[type="range"] {width: 200px;}
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 Bot Controller</h1>
        <div class="controls">
            <button id="modeToggle">Switch Mode</button>
            <p id="currentMode">Current Mode: Line Following</p>
        </div>
        <div class="controls">
            <label for="speed">Speed: </label>
            <input type="range" id="speed" min="0" max="255" value="150">
            <span id="speedValue">150</span>
        </div>
        <div class="joystick-container" id="joystickContainer">
            <div class="joystick" id="joystick"></div>
        </div>
    </div>
    <script>
        let isLineFollowMode = true;
        const joystick = document.getElementById('joystick');
        const container = document.getElementById('joystickContainer');
        const modeToggle = document.getElementById('modeToggle');
        const currentMode = document.getElementById('currentMode');
        const speedControl = document.getElementById('speed');
        const speedValue = document.getElementById('speedValue');
        
        let isDragging = false;
        let currentX = 0;
        let currentY = 0;
        
        function handleStart(e) {
            isDragging = true;
            joystick.style.transition = 'none';
        }
        
        function handleMove(e) {
            if (!isDragging) return;
            
            const rect = container.getBoundingClientRect();
            const centerX = rect.width / 2;
            const centerY = rect.height / 2;
            
            let clientX, clientY;
            if (e.type === 'touchmove') {
                clientX = e.touches[0].clientX;
                clientY = e.touches[0].clientY;
            } else {
                clientX = e.clientX;
                clientY = e.clientY;
            }
            
            let x = clientX - rect.left - centerX;
            let y = clientY - rect.top - centerY;
            
            const distance = Math.sqrt(x * x + y * y);
            const maxDistance = rect.width / 2 - joystick.offsetWidth / 2;
            
            if (distance > maxDistance) {
                x *= maxDistance / distance;
                y *= maxDistance / distance;
            }
            
            currentX = x / maxDistance * 100;
            currentY = -y / maxDistance * 100;
            
            joystick.style.transform = `translate(${x}px, ${y}px)`;
            if (!isLineFollowMode) {
                sendControl(currentX, currentY);
            }
        }
        
        function handleEnd() {
            if (!isDragging) return;
            isDragging = false;
            currentX = 0;
            currentY = 0;
            joystick.style.transition = 'transform 0.2s ease-out';
            joystick.style.transform = 'translate(0px, 0px)';
            if (!isLineFollowMode) {
                sendControl(0, 0);
            }
        }
        
        // Touch Events
        joystick.addEventListener('touchstart', handleStart);
        document.addEventListener('touchmove', handleMove);
        document.addEventListener('touchend', handleEnd);
        
        // Mouse Events
        joystick.addEventListener('mousedown', handleStart);
        document.addEventListener('mousemove', handleMove);
        document.addEventListener('mouseup', handleEnd);
        
        modeToggle.addEventListener('click', () => {
            isLineFollowMode = !isLineFollowMode;
            currentMode.textContent = `Current Mode: ${isLineFollowMode ? 'Line Following' : 'Remote Control'}`;
            fetch('/mode', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({mode: isLineFollowMode})
            });
        });
        
        speedControl.addEventListener('input', () => {
            const speed = speedControl.value;
            speedValue.textContent = speed;
            fetch('/speed', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({speed: parseInt(speed)})
            });
        });
        
        function sendControl(x, y) {
            fetch('/control', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({x, y})
            });
        }
    </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  
  // Setup Motor pins
  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_B_IN1, OUTPUT);
  pinMode(MOTOR_B_IN2, OUTPUT);
  pinMode(MOTOR_A_EN, OUTPUT);
  pinMode(MOTOR_B_EN, OUTPUT);
  
  // Setup input pins
  pinMode(LEFT_SENSOR, INPUT);
  pinMode(RIGHT_SENSOR, INPUT);
  pinMode(MODE_SWITCH, INPUT_PULLUP);
  
  // Initialize PWM
  ledcSetup(0, 5000, 8);  // Channel 0, 5KHz, 8-bit resolution
  ledcSetup(1, 5000, 8);  // Channel 1, 5KHz, 8-bit resolution
  ledcAttachPin(MOTOR_A_EN, 0);
  ledcAttachPin(MOTOR_B_EN, 1);

  // Create Access Point
  WiFi.softAP(ssid, password);
  Serial.println("Access Point Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Setup web server routes
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", INDEX_HTML);
  });
  
  server.on("/control", HTTP_POST, handleControl);
  server.on("/mode", HTTP_POST, handleMode);
  server.on("/speed", HTTP_POST, handleSpeed);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  
  // Read physical mode switch if present
  if (digitalRead(MODE_SWITCH) == LOW) {
    isLineFollowMode = !isLineFollowMode;
    delay(200); // Debounce
  }
  
  if (isLineFollowMode) {
    lineFollowMode();
  }
  
  delay(10);
}

void handleControl() {
  if (server.hasArg("plain")) {
    String message = server.arg("plain");
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (!error) {
      int x = doc["x"];
      int y = doc["y"];
      
      // Map joystick values to motor speeds
      int xMapped = map(x, -100, 100, -currentSpeed, currentSpeed);
      int yMapped = map(y, -100, 100, -currentSpeed, currentSpeed);
      
      // Calculate motor speeds
      int leftMotor = yMapped + xMapped;
      int rightMotor = yMapped - xMapped;
      
      // Constrain speeds
      leftMotor = constrain(leftMotor, -currentSpeed, currentSpeed);
      rightMotor = constrain(rightMotor, -currentSpeed, currentSpeed);
      
      moveMotors(leftMotor, rightMotor);
      server.send(200, "text/plain", "OK");
    }
  }
}

void handleMode() {
  if (server.hasArg("plain")) {
    String message = server.arg("plain");
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (!error) {
      isLineFollowMode = doc["mode"];
      server.send(200, "text/plain", "OK");
    }
  }
}

void handleSpeed() {
  if (server.hasArg("plain")) {
    String message = server.arg("plain");
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (!error) {
      currentSpeed = doc["speed"];
      currentSpeed = constrain(currentSpeed, MIN_SPEED, MAX_SPEED);
      server.send(200, "text/plain", "OK");
    }
  }
}

void lineFollowMode() {
  int leftValue = analogRead(LEFT_SENSOR);
  int rightValue = analogRead(RIGHT_SENSOR);
  
  if (leftValue > LINE_THRESHOLD && rightValue > LINE_THRESHOLD) {
    moveMotors(currentSpeed, currentSpeed);  // Forward
  }
  else if (leftValue > LINE_THRESHOLD) {
    moveMotors(-currentSpeed, currentSpeed); // Turn left
  }
  else if (rightValue > LINE_THRESHOLD) {
    moveMotors(currentSpeed, -currentSpeed); // Turn right
  }
  else {
    moveMotors(0, 0); // Stop
  }
}

void moveMotors(int leftSpeed, int rightSpeed) {
  // Control left motor
  if (leftSpeed >= 0) {
    digitalWrite(MOTOR_A_IN1, HIGH);
    digitalWrite(MOTOR_A_IN2, LOW);
    ledcWrite(0, leftSpeed);
  } else {
    digitalWrite(MOTOR_A_IN1, LOW);
    digitalWrite(MOTOR_A_IN2, HIGH);
    ledcWrite(0, -leftSpeed);
  }
  
  // Control right motor
  if (rightSpeed >= 0) {
    digitalWrite(MOTOR_B_IN1, HIGH);
    digitalWrite(MOTOR_B_IN2, LOW);
    ledcWrite(1, rightSpeed);
  } else {
    digitalWrite(MOTOR_B_IN1, LOW);
    digitalWrite(MOTOR_B_IN2, HIGH);
    ledcWrite(1, -rightSpeed);
  }
}
