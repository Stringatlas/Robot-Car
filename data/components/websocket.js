// Shared WebSocket manager - maintains connection across page navigation
// Uses sessionStorage to persist console history

const WSManager = (function() {
    let ws = null;
    let myClientId = null;
    let hasControl = false;
    let reconnectTimeout = null;
    const CONSOLE_STORAGE_KEY = 'robot_car_console';
    const MAX_STORED_LOGS = 100;
    
    // Callbacks that pages can register
    const callbacks = {
        onControlChange: [],
        onEncoderData: [],
        onBatteryData: [],
        onMotorData: [],
        onStatusChange: [],
        onRawMessage: []
    };
    
    function connect() {
        const wsUrl = 'ws://' + window.location.hostname + '/ws';
        ws = new WebSocket(wsUrl);
        
        ws.onopen = function() {
            console.log('WebSocket connected');
            notifyStatusChange('Connected', true);
            loadConsoleHistory();
        };
        
        ws.onmessage = function(event) {
            const rawData = event.data;
            
            // Try JSON first
            try {
                const data = JSON.parse(rawData);
                
                if (data.type === 'welcome') {
                    myClientId = data.clientId;
                    console.log('My client ID:', myClientId);
                }
                else if (data.type === 'control') {
                    updateControlStatus(data.controllingClientId);
                }
                else if (data.type === 'log') {
                    addLogToConsole(data.message);
                    saveLogToStorage(data.message);
                }
                else if (data.left && data.right) {
                    notifyEncoderData(data);
                    if (data.battery !== undefined) {
                        notifyBatteryData(data.battery);
                    }
                    if (data.motorLeft !== undefined || data.motorRight !== undefined) {
                        notifyMotorData(data.motorLeft, data.motorRight);
                    }
                }
            } catch(e) {
                // Not JSON, handle string messages
                notifyRawMessage(rawData);
            }
        };
        
        ws.onerror = function(error) {
            console.error('WebSocket error:', error);
            notifyStatusChange('Error', false);
        };
        
        ws.onclose = function() {
            console.log('WebSocket disconnected, reconnecting...');
            notifyStatusChange('Disconnected', false);
            hasControl = false;
            notifyControlChange(false);
            reconnectTimeout = setTimeout(connect, 2000);
        };
    }
    
    function updateControlStatus(controllingClientId) {
        if (controllingClientId === 0) {
            hasControl = false;
        } else if (myClientId !== null && controllingClientId === myClientId) {
            hasControl = true;
        } else {
            hasControl = false;
        }
        notifyControlChange(hasControl);
    }
    
    function send(message) {
        if (ws && ws.readyState === WebSocket.OPEN) {
            ws.send(message);
            return true;
        }
        return false;
    }
    
    // Console history management
    function saveLogToStorage(message) {
        try {
            let logs = JSON.parse(sessionStorage.getItem(CONSOLE_STORAGE_KEY) || '[]');
            logs.push({
                message: message,
                timestamp: new Date().toISOString()
            });
            // Keep only last N logs
            if (logs.length > MAX_STORED_LOGS) {
                logs = logs.slice(-MAX_STORED_LOGS);
            }
            sessionStorage.setItem(CONSOLE_STORAGE_KEY, JSON.stringify(logs));
        } catch(e) {
            console.error('Error saving to sessionStorage:', e);
        }
    }
    
    function loadConsoleHistory() {
        try {
            const logs = JSON.parse(sessionStorage.getItem(CONSOLE_STORAGE_KEY) || '[]');
            logs.forEach(log => {
                addLogToConsole(log.message);
            });
        } catch(e) {
            console.error('Error loading from sessionStorage:', e);
        }
    }
    
    function clearConsoleHistory() {
        sessionStorage.removeItem(CONSOLE_STORAGE_KEY);
    }
    
    // Callback registration
    function on(event, callback) {
        if (callbacks[event]) {
            callbacks[event].push(callback);
        }
    }
    
    // Notify callbacks
    function notifyControlChange(hasCtrl) {
        callbacks.onControlChange.forEach(cb => cb(hasCtrl));
    }
    
    function notifyEncoderData(data) {
        callbacks.onEncoderData.forEach(cb => cb(data));
    }
    
    function notifyBatteryData(voltage) {
        callbacks.onBatteryData.forEach(cb => cb(voltage));
    }
    
    function notifyMotorData(left, right) {
        callbacks.onMotorData.forEach(cb => cb(left, right));
    }
    
    function notifyStatusChange(status, connected) {
        callbacks.onStatusChange.forEach(cb => cb(status, connected));
    }
    
    function notifyRawMessage(message) {
        callbacks.onRawMessage.forEach(cb => cb(message));
    }
    
    // Public API
    return {
        connect: connect,
        send: send,
        getControlState: () => hasControl,
        getClientId: () => myClientId,
        on: on,
        clearHistory: clearConsoleHistory
    };
})();

// Auto-connect when script loads
WSManager.connect();
