/**
 * index.js - Main control page functionality
 * Requires: common.js
 */

// Control state
let myClientId = null;
let hasControl = false;

// Joystick state
let joystick, stick, isDragging, centerX, centerY;

// Initialize when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    initJoystick();
    connectWebSocket('/ws', handleMessage, handleConnect);
});

/**
 * Handle WebSocket connection established
 */
function handleConnect() {
    updateStatus('status', 'Connected', 'success');
}

/**
 * Handle incoming WebSocket messages
 */
function handleMessage(data) {
    // Try to parse as JSON first
    const json = parseTelemetry(data);
    
    if (json) {
        handleJSONMessage(json);
    } else {
        // Handle text messages
        handleTextMessage(data);
    }
}

/**
 * Handle JSON telemetry messages
 */
function handleJSONMessage(data) {
    // Welcome message with client ID
    if (data.type === 'welcome') {
        myClientId = data.clientId;
        logMessage('My client ID: ' + myClientId);
        return;
    }
    
    // Control status updates
    if (data.type === 'control') {
        updateControlStatus(data.controllingClientId);
        return;
    }
    
    // Log messages
    if (data.type === 'log') {
        addLogToConsole(data.message);
        return;
    }
    
    // Encoder telemetry data
    if (data.left && data.right) {
        updateEncoderDisplay(data);
    }
}

/**
 * Handle text-based messages
 */
function handleTextMessage(data) {
    logMessage('Received: ' + data);
}

/**
 * Update encoder data display
 */
function updateEncoderDisplay(data) {
    // Left encoder
    document.getElementById('left-count').textContent = data.left.count;
    document.getElementById('left-revs').textContent = formatNumber(data.left.revolutions, 2);
    document.getElementById('left-dist').textContent = formatNumber(data.left.distance, 2) + ' cm';
    document.getElementById('left-vel').textContent = formatNumber(data.left.velocity, 2) + ' cm/s';
    document.getElementById('left-rpm').textContent = formatNumber(data.left.rpm, 1);
    
    // Right encoder
    document.getElementById('right-count').textContent = data.right.count;
    document.getElementById('right-revs').textContent = formatNumber(data.right.revolutions, 2);
    document.getElementById('right-dist').textContent = formatNumber(data.right.distance, 2) + ' cm';
    document.getElementById('right-vel').textContent = formatNumber(data.right.velocity, 2) + ' cm/s';
    document.getElementById('right-rpm').textContent = formatNumber(data.right.rpm, 1);
    
    // Battery voltage with color coding
    if (data.battery !== undefined) {
        const batteryElement = document.getElementById('battery-voltage');
        batteryElement.textContent = formatNumber(data.battery, 2) + ' V';
        
        // Color code based on voltage (3S LiPo: 9V-12.6V)
        if (data.battery >= 11.1) {
            batteryElement.style.color = '#4CAF50'; // Green
        } else if (data.battery >= 10.2) {
            batteryElement.style.color = '#ff9800'; // Orange
        } else {
            batteryElement.style.color = '#f44336'; // Red
        }
    }
    
    // Motor PWM values
    if (data.motorLeft !== undefined) {
        document.getElementById('motor-left').textContent = data.motorLeft;
    }
    if (data.motorRight !== undefined) {
        document.getElementById('motor-right').textContent = data.motorRight;
    }
}

/**
 * Update control status based on controlling client
 */
function updateControlStatus(controllingClientId) {
    if (controllingClientId === 0) {
        hasControl = false;
    } else if (myClientId !== null && controllingClientId === myClientId) {
        hasControl = true;
    } else {
        hasControl = false;
    }
    updateControlUI();
}

/**
 * Update control UI based on hasControl state
 */
function updateControlUI() {
    const controlText = document.getElementById('control-text');
    const controlBtn = document.getElementById('control-btn');
    const joystickElement = document.getElementById('joystick');
    
    if (hasControl) {
        controlText.textContent = 'You have control';
        controlText.style.color = '#4CAF50';
        controlBtn.textContent = 'Release Control';
        controlBtn.style.background = '#f44336';
        joystickElement.style.opacity = '1';
    } else {
        controlText.textContent = 'Someone else has control';
        controlText.style.color = '#ff9800';
        controlBtn.textContent = 'Request Control';
        controlBtn.style.background = '#4CAF50';
        joystickElement.style.opacity = '0.3';
    }
}

/**
 * Toggle control (request or release)
 */
function toggleControl() {
    if (!checkConnection()) return;
    
    if (hasControl) {
        sendWebSocketMessage('RELEASE_CONTROL');
        hasControl = false;
    } else {
        sendWebSocketMessage('REQUEST_CONTROL');
        hasControl = true;
    }
    updateControlUI();
}

/**
 * Reset encoder counts
 */
function resetEncoders() {
    if (!checkConnection()) return;
    sendWebSocketMessage('RESET');
    logMessage('Encoder reset requested');
}

/**
 * Initialize joystick controls
 */
function initJoystick() {
    joystick = document.getElementById('joystick');
    stick = document.getElementById('stick');
    
    if (!joystick || !stick) {
        console.error('Joystick elements not found');
        return;
    }
    
    isDragging = false;
    centerX = 100;
    centerY = 100;
    
    // Touch events
    joystick.addEventListener('touchstart', (e) => {
        if (hasControl) {
            isDragging = true;
            e.preventDefault();
        }
    });
    
    joystick.addEventListener('touchmove', (e) => {
        if (isDragging && hasControl) {
            const rect = joystick.getBoundingClientRect();
            const x = e.touches[0].clientX - rect.left;
            const y = e.touches[0].clientY - rect.top;
            updateJoystickPosition(x, y);
            e.preventDefault();
        }
    });
    
    joystick.addEventListener('touchend', (e) => {
        if (hasControl && isDragging) {
            isDragging = false;
            resetJoystickPosition();
            e.preventDefault();
        }
    });
    
    // Mouse events (for desktop)
    joystick.addEventListener('mousedown', (e) => {
        if (hasControl) {
            isDragging = true;
            e.preventDefault();
        }
    });
    
    document.addEventListener('mousemove', (e) => {
        if (isDragging && hasControl) {
            const rect = joystick.getBoundingClientRect();
            const x = e.clientX - rect.left;
            const y = e.clientY - rect.top;
            updateJoystickPosition(x, y);
        }
    });
    
    document.addEventListener('mouseup', () => {
        if (hasControl && isDragging) {
            isDragging = false;
            resetJoystickPosition();
        }
    });
}

/**
 * Update joystick position and send control commands
 */
function updateJoystickPosition(x, y) {
    // Calculate offset from center
    let dx = x - centerX;
    let dy = y - centerY;
    
    // Limit to circle radius (80px)
    const maxRadius = 80;
    const distance = Math.sqrt(dx * dx + dy * dy);
    
    if (distance > maxRadius) {
        const angle = Math.atan2(dy, dx);
        dx = Math.cos(angle) * maxRadius;
        dy = Math.sin(angle) * maxRadius;
    }
    
    // Update stick position
    stick.style.left = (centerX + dx - 20) + 'px';
    stick.style.top = (centerY + dy - 20) + 'px';
    
    // Convert to forward (-1 to 1) and turn (-1 to 1)
    const forward = -dy / maxRadius;  // Negative because y increases downward
    const turn = dx / maxRadius;
    
    // Send control command
    sendJoystickCommand(forward, turn);
}

/**
 * Reset joystick to center position
 */
function resetJoystickPosition() {
    stick.style.left = '80px';
    stick.style.top = '80px';
    sendJoystickCommand(0, 0);
}

/**
 * Send joystick control command
 */
function sendJoystickCommand(forward, turn) {
    if (!checkConnection()) return;
    
    // Round to 2 decimal places
    forward = Math.round(forward * 100) / 100;
    turn = Math.round(turn * 100) / 100;
    
    const message = `CONTROL:${forward},${turn}`;
    sendWebSocketMessage(message);
}

/**
 * Add log message to console (if console element exists)
 */
function addLogToConsole(message) {
    const consoleElement = document.getElementById('console');
    if (consoleElement) {
        const timestamp = getTimestamp();
        consoleElement.textContent += `[${timestamp}] ${message}\n`;
        consoleElement.scrollTop = consoleElement.scrollHeight;
    }
}
