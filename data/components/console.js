function addLogToConsole(message, logType) {
    let colorClass = '';
    switch (logType) {
        case 'ERROR':
            colorClass = 'console-error';
            console.error(message);
            break;
        case 'WARNING':
            colorClass = 'console-warning';
            console.warn(message);
            break;
        case 'SUCCESS':
            colorClass = 'console-success';
            console.log(message);
            break;
        case 'INFO':
            colorClass = 'console-info';
            console.info(message);
            break;
        case 'DEBUG':
            colorClass = 'console-warning';
            console.warn(message);
            break;
        case 'UPDATE':
        case 'COMMAND':
        default:
            colorClass = 'console-info';
            console.info(message);
            break;
    }

    const consoleDiv = document.getElementById('console');
    if (!consoleDiv) return;

    if (consoleDiv.children.length === 1 && consoleDiv.children[0].classList.contains('text-muted')) {
        consoleDiv.innerHTML = '';
    }

    const timestamp = new Date().toLocaleTimeString();
    const logEntry = document.createElement('div');
    logEntry.className = 'console-line';
    logEntry.innerHTML = `<span class="console-timestamp">[${timestamp}]</span><span class="${colorClass}">${message}</span>`;
    consoleDiv.appendChild(logEntry);

    consoleDiv.scrollTop = consoleDiv.scrollHeight;

    while (consoleDiv.children.length > 500) {
        consoleDiv.removeChild(consoleDiv.firstChild);
    }
}

function clearConsole() {
    const consoleDiv = document.getElementById('console');
    consoleDiv.innerHTML = '<div class="text-muted">Console cleared.</div>';

    if (typeof WSManager !== 'undefined') {
        WSManager.clearHistory();
    }
}
