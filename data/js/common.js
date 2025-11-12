/**
 * common.js - Shared utilities for index.html and calibration.html
 */

// WebSocket connection management
let ws = null;
let reconnectTimeout = null;

/**
 * Connect to WebSocket server
 * @param {string} path - WebSocket path (default: '/ws')
 * @param {function} onMessage - Message handler callback
 * @param {function} onConnect - Connection established callback (optional)
 */
function connectWebSocket(path = '/ws', onMessage, onConnect = null) {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${window.location.host}${path}`;
    
    console.log('Connecting to WebSocket:', wsUrl);
    
    ws = new WebSocket(wsUrl);
    
    ws.onopen = () => {
        console.log('WebSocket Connected');
        if (reconnectTimeout) {
            clearTimeout(reconnectTimeout);
            reconnectTimeout = null;
        }
        if (onConnect) {
            onConnect();
        }
    };
    
    ws.onmessage = (event) => {
        if (onMessage) {
            onMessage(event.data);
        }
    };
    
    ws.onerror = (error) => {
        console.error('WebSocket error:', error);
    };
    
    ws.onclose = () => {
        console.log('WebSocket Disconnected');
        ws = null;
        // Attempt to reconnect after 1 second
        reconnectTimeout = setTimeout(() => {
            connectWebSocket(path, onMessage, onConnect);
        }, 1000);
    };
    
    return ws;
}

/**
 * Send message via WebSocket
 * @param {string} message - Message to send
 * @returns {boolean} - True if sent successfully
 */
function sendWebSocketMessage(message) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(message);
        return true;
    } else {
        console.warn('WebSocket not connected, cannot send:', message);
        return false;
    }
}

/**
 * Check if WebSocket is connected
 * @returns {boolean}
 */
function isWebSocketConnected() {
    return ws && ws.readyState === WebSocket.OPEN;
}

/**
 * Update status display element
 * @param {string} elementId - ID of status element
 * @param {string} message - Status message
 * @param {string} type - Type: 'info', 'success', 'warning', 'error'
 */
function updateStatus(elementId, message, type = 'info') {
    const element = document.getElementById(elementId);
    if (!element) return;
    
    element.textContent = message;
    
    // Remove existing type classes
    element.classList.remove('status-info', 'status-success', 'status-warning', 'status-error');
    
    // Add new type class
    element.classList.add(`status-${type}`);
}

/**
 * Format number with fixed decimal places
 * @param {number} value - Number to format
 * @param {number} decimals - Number of decimal places
 * @returns {string}
 */
function formatNumber(value, decimals = 2) {
    if (isNaN(value)) return 'N/A';
    return Number(value).toFixed(decimals);
}

/**
 * Constrain value between min and max
 * @param {number} value - Value to constrain
 * @param {number} min - Minimum value
 * @param {number} max - Maximum value
 * @returns {number}
 */
function constrain(value, min, max) {
    return Math.min(Math.max(value, min), max);
}

/**
 * Map value from one range to another
 * @param {number} value - Input value
 * @param {number} inMin - Input range minimum
 * @param {number} inMax - Input range maximum
 * @param {number} outMin - Output range minimum
 * @param {number} outMax - Output range maximum
 * @returns {number}
 */
function mapRange(value, inMin, inMax, outMin, outMax) {
    return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

/**
 * Debounce function - prevents rapid repeated calls
 * @param {function} func - Function to debounce
 * @param {number} wait - Wait time in milliseconds
 * @returns {function}
 */
function debounce(func, wait) {
    let timeout;
    return function executedFunction(...args) {
        const later = () => {
            clearTimeout(timeout);
            func(...args);
        };
        clearTimeout(timeout);
        timeout = setTimeout(later, wait);
    };
}

/**
 * Export data as CSV file
 * @param {Array} data - Array of objects to export
 * @param {string} filename - Output filename
 */
function exportToCSV(data, filename = 'data.csv') {
    if (!data || data.length === 0) {
        alert('No data to export');
        return;
    }
    
    // Get headers from first object
    const headers = Object.keys(data[0]);
    
    // Build CSV content
    let csv = headers.join(',') + '\n';
    
    data.forEach(row => {
        csv += headers.map(header => {
            const value = row[header];
            // Escape values containing commas or quotes
            if (typeof value === 'string' && (value.includes(',') || value.includes('"'))) {
                return '"' + value.replace(/"/g, '""') + '"';
            }
            return value;
        }).join(',') + '\n';
    });
    
    // Create download link
    const blob = new Blob([csv], { type: 'text/csv' });
    const url = window.URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    a.click();
    window.URL.revokeObjectURL(url);
}

/**
 * Export data as JSON file
 * @param {Object|Array} data - Data to export
 * @param {string} filename - Output filename
 */
function exportToJSON(data, filename = 'data.json') {
    if (!data) {
        alert('No data to export');
        return;
    }
    
    const json = JSON.stringify(data, null, 2);
    const blob = new Blob([json], { type: 'application/json' });
    const url = window.URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    a.click();
    window.URL.revokeObjectURL(url);
}

/**
 * Show alert if WebSocket is not connected
 * @returns {boolean} - True if connected, false if not
 */
function checkConnection() {
    if (!isWebSocketConnected()) {
        alert('WebSocket not connected! Please wait for connection.');
        return false;
    }
    return true;
}

/**
 * Parse JSON telemetry data safely
 * @param {string} data - Raw data string
 * @returns {Object|null} - Parsed object or null if not JSON
 */
function parseTelemetry(data) {
    try {
        return JSON.parse(data);
    } catch (e) {
        return null;
    }
}

/**
 * Get current timestamp for logging
 * @returns {string} - Formatted timestamp
 */
function getTimestamp() {
    const now = new Date();
    return now.toISOString().split('T')[1].split('.')[0]; // HH:MM:SS
}

/**
 * Log message with timestamp
 * @param {string} message - Message to log
 */
function logMessage(message) {
    console.log(`[${getTimestamp()}] ${message}`);
}
