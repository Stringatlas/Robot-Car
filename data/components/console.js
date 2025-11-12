// Shared console functionality for both index.html and calibration.html

function addLogToConsole(message) {
    const consoleDiv = document.getElementById('console');
    
    // Clear "Waiting for telemetry..." message on first log
    if (consoleDiv.children.length === 1 && consoleDiv.children[0].classList.contains('text-muted')) {
        consoleDiv.innerHTML = '';
    }
    
    // Create log entry with timestamp
    const timestamp = new Date().toLocaleTimeString();
    const logEntry = document.createElement('div');
    logEntry.className = 'console-line';
    
    // Determine message type and color
    let colorClass = '';
    if (message.includes('ERROR') || message.includes('✗')) {
        colorClass = 'console-error';
        console.error(message);
    } else if (message.includes('⚠️') || message.includes('WARNING')) {
        colorClass = 'console-warning';
        console.warn(message);
    } else if (message.includes('✓')) {
        colorClass = 'console-success';
        console.log(message);
    } else if (message.includes('🕹️') || message.includes('🎛️')) {
        colorClass = 'console-info';
        console.info(message);
    } else {
        colorClass = 'console-info';
        console.log(message);
    }
    
    logEntry.innerHTML = `<span class="console-timestamp">[${timestamp}]</span><span class="${colorClass}">${message}</span>`;
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
    consoleDiv.innerHTML = '<div class="text-muted">Console cleared.</div>';
    // Clear history if WSManager is available
    if (typeof WSManager !== 'undefined') {
        WSManager.clearHistory();
    }
}
