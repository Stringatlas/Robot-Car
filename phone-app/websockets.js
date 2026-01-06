function getWebSocketUrl() {
    const hostname = window.location.hostname;
    
    // Served from ESP32 itself
    if (hostname.match(/^192\.168\.|^10\.|^172\.(1[6-9]|2[0-9]|3[01])\./)) {
        return 'ws://' + hostname + '/ws';
    }
    
    // External host
    const urlParams = new URLSearchParams(window.location.search);
    const paramIP = urlParams.get('ip');
    if (paramIP) {
        localStorage.setItem('esp32_ip', paramIP);
        return 'ws://' + paramIP + '/ws';
    }

    const storedIp = localStorage.getItem('esp32_ip');
    
    if (storedIp) {
        return 'ws://' + storedIp + '/ws';
    }

    const inputIp = prompt('Enter ESP32 IP address:', '192.168.2.2');
    if (inputIp) localStorage.setItem('esp32_ip', inputIp);
    return 'ws://' + inputIp + '/ws';
}

const wsUrl = getWebSocketUrl();
ws = new WebSocket(wsUrl);

function changeEsp32Ip() {
    const currentIp = localStorage.getItem('esp32_ip') || '';
    const newIp = prompt('Enter new ESP32 IP address:', currentIp);
    if (newIp && newIp !== currentIp) {
        localStorage.setItem('esp32_ip', newIp);
        location.reload();
    }
}