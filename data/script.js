// WebSocket connection
let ws;
let myClientId = null;
let hasControl = false;

function connectWebSocket() {
    // Use current host for WebSocket connection
    const wsUrl = 'ws://' + window.location.hostname + '/ws';
    ws = new WebSocket(wsUrl);
    
    ws.onopen = function() {
        console.log('WebSocket connected');
        document.getElementById('status').textContent = 'Connected';
        document.getElementById('status').className = 'connected';
    };
    
    ws.onmessage = function(event) {
        try {
            const data = JSON.parse(event.data);
            
            // Handle welcome message with client ID
            if (data.type === 'welcome') {
                myClientId = data.clientId;
                console.log('My client ID:', myClientId);
            }
            // Handle control status updates
            else if (data.type === 'control') {
                updateControlStatus(data.controllingClientId);
            }
            // Handle log messages
            else if (data.type === 'log') {
                addLogToConsole(data.message);
            }
            // Handle encoder data
            else if (data.left && data.right) {
                // Update left encoder
                document.getElementById('left-count').textContent = data.left.count;
                document.getElementById('left-revs').textContent = data.left.revolutions;
                document.getElementById('left-dist').textContent = data.left.distance + ' cm';
                document.getElementById('left-vel').textContent = data.left.velocity + ' cm/s';
                document.getElementById('left-rpm').textContent = data.left.rpm;
                
                // Update right encoder
                document.getElementById('right-count').textContent = data.right.count;
                document.getElementById('right-revs').textContent = data.right.revolutions;
                document.getElementById('right-dist').textContent = data.right.distance + ' cm';
                document.getElementById('right-vel').textContent = data.right.velocity + ' cm/s';
                document.getElementById('right-rpm').textContent = data.right.rpm;
                
                // Update battery voltage if available
                if (data.battery !== undefined) {
                    const batteryVoltage = data.battery.toFixed(2);
                    const batteryElement = document.getElementById('battery-voltage');
                    batteryElement.textContent = batteryVoltage + ' V';
                    
                    // Color code based on voltage (assuming 3S LiPo: 9V-12.6V)
                    if (data.battery >= 11.1) {
                        batteryElement.style.color = '#4CAF50'; // Green - Good
                    } else if (data.battery >= 10.2) {
                        batteryElement.style.color = '#ff9800'; // Orange - Low
                    } else {
                        batteryElement.style.color = '#f44336'; // Red - Critical
                    }
                }
                
                // Update motor PWM values if available
                if (data.motorLeft !== undefined) {
                    document.getElementById('motor-left').textContent = data.motorLeft;
                }
                if (data.motorRight !== undefined) {
                    document.getElementById('motor-right').textContent = data.motorRight;
                }
                

            }
        } catch(e) {
            console.error('Error parsing data:', e);
        }
    };
    
    ws.onerror = function(error) {
        console.error('WebSocket error:', error);
        document.getElementById('status').textContent = 'Error';
        document.getElementById('status').className = 'disconnected';
    };
    
    ws.onclose = function() {
        console.log('WebSocket disconnected, reconnecting...');
        document.getElementById('status').textContent = 'Disconnected';
        document.getElementById('status').className = 'disconnected';
        hasControl = false;
        updateControlUI();
        // Reconnect after 2 seconds
        setTimeout(connectWebSocket, 2000);
    };
}

function updateControlStatus(controllingClientId) {
    // Check if we are the controlling client
    if (controllingClientId === 0) {
        // No one has control
        hasControl = false;
    } else if (myClientId !== null && controllingClientId === myClientId) {
        // We have control
        hasControl = true;
    } else {
        // Someone else has control
        hasControl = false;
    }
    updateControlUI();
}

function updateControlUI() {
    const controlText = document.getElementById('control-text');
    const controlBtn = document.getElementById('control-btn');
    const joystick = document.getElementById('joystick');
    
    if (hasControl) {
        controlText.textContent = 'You have control';
        controlText.style.color = '#4CAF50';
        controlBtn.textContent = 'Release Control';
        controlBtn.style.display = 'inline-block';
        controlBtn.style.background = '#f44336';
        joystick.style.opacity = '1';
    } else {
        controlText.textContent = 'Someone else has control';
        controlText.style.color = '#ff9800';
        controlBtn.textContent = 'Request Control';
        controlBtn.style.display = 'inline-block';
        controlBtn.style.background = '#4CAF50';
        joystick.style.opacity = '0.3';
    }
}

function toggleControl() {
    if (ws && ws.readyState === WebSocket.OPEN) {
        if (hasControl) {
            ws.send('RELEASE_CONTROL');
            hasControl = false;
        } else {
            ws.send('REQUEST_CONTROL');
            hasControl = true;
        }
        updateControlUI();
    }
}

function resetEncoders() {
    // Send reset command via WebSocket
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send('RESET');
    } else {
        console.error('WebSocket not connected');
    }
}

// Joystick control - Initialize after DOM is ready
let joystick, stick, isDragging, centerX, centerY;

function initJoystick() {
    joystick = document.getElementById('joystick');
    stick = document.getElementById('stick');
    isDragging = false;
    centerX = 100;
    centerY = 100;
    
    if (!joystick || !stick) {
        console.error('Joystick elements not found');
        return;
    }
    
    joystick.addEventListener('touchstart', function(e) {
        if (hasControl) {
            isDragging = true; 
            e.preventDefault();
        }
    });
    
    joystick.addEventListener('touchmove', function(e) {
        if(isDragging && hasControl) {
            let rect = joystick.getBoundingClientRect();
            let x = e.touches[0].clientX - rect.left;
            let y = e.touches[0].clientY - rect.top;
            updateJoystick(x, y);
            e.preventDefault();
        }
    });
    
    joystick.addEventListener('touchend', function(e) {
        if (hasControl && isDragging) {
            isDragging = false;
            stick.style.left = '80px'; 
            stick.style.top = '80px';
            sendJoystick(0, 0);
            e.preventDefault();
        }
    });
    
    // Also support mouse for desktop testing
    joystick.addEventListener('mousedown', function(e) {
        if (hasControl) {
            isDragging = true;
            e.preventDefault();
        }
    });
    
    document.addEventListener('mousemove', function(e) {
        if(isDragging && hasControl) {
            let rect = joystick.getBoundingClientRect();
            let x = e.clientX - rect.left;
            let y = e.clientY - rect.top;
            updateJoystick(x, y);
            e.preventDefault();
        }
    });
    
    document.addEventListener('mouseup', function(e) {
        if(isDragging && hasControl) {
            isDragging = false;
            stick.style.left = '80px'; 
            stick.style.top = '80px';
            sendJoystick(0, 0);
            e.preventDefault();
        }
    });
}

function sendJoystick(x, y) {
    // Only send joystick data if we have control
    if (hasControl && ws && ws.readyState === WebSocket.OPEN) {
        ws.send('JOYSTICK:' + x + ',' + y);
        // Update display
        document.getElementById('joy-x').textContent = x;
        document.getElementById('joy-y').textContent = y;
    }
}

function updateJoystick(x, y) {
    let deltaX = x - centerX;
    let deltaY = y - centerY;
    let distance = Math.sqrt(deltaX*deltaX + deltaY*deltaY);
    if(distance > 80) {
        deltaX = deltaX * 80 / distance;
        deltaY = deltaY * 80 / distance;
    }
    stick.style.left = (centerX + deltaX - 20) + 'px';
    stick.style.top = (centerY + deltaY - 20) + 'px';
    
    // Normalize to -1.0 to 1.0 range
    let normalizedX = (deltaX / 80).toFixed(2);
    let normalizedY = (-deltaY / 80).toFixed(2);
    sendJoystick(normalizedX, normalizedY);
}

function addLogToConsole(message) {
    const consoleDiv = document.getElementById('console');
    
    // Clear "Waiting for telemetry..." message on first log
    if (consoleDiv.children.length === 1 && consoleDiv.children[0].textContent === 'Waiting for telemetry...') {
        consoleDiv.innerHTML = '';
    }
    
    // Create log entry
    const logEntry = document.createElement('div');
    logEntry.textContent = message;
    logEntry.style.marginBottom = '2px';
    
    // Color code based on message content
    if (message.includes('ERROR') || message.includes('✗')) {
        logEntry.style.color = '#ff5555';
    } else if (message.includes('⚠️') || message.includes('WARNING')) {
        logEntry.style.color = '#ffaa00';
    } else if (message.includes('✓')) {
        logEntry.style.color = '#00ff00';
    } else if (message.includes('🕹️') || message.includes('🎛️')) {
        logEntry.style.color = '#00aaff';
    } else {
        logEntry.style.color = '#00ff00';
    }
    
    consoleDiv.appendChild(logEntry);
    
    // Auto-scroll to bottom
    consoleDiv.scrollTop = consoleDiv.scrollHeight;
    
    // Limit number of log entries to prevent memory issues
    while (consoleDiv.children.length > 500) {
        consoleDiv.removeChild(consoleDiv.firstChild);
    }
}

function clearConsole() {
    const consoleDiv = document.getElementById('console');
    consoleDiv.innerHTML = '<div style="color: #888;">Console cleared</div>';
}

// Initialize everything on page load
window.addEventListener('DOMContentLoaded', function() {
    initJoystick();
    connectWebSocket();
});
