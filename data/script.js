// Page-specific state
let hasControl = false;

// Register callbacks with WSManager
WSManager.on('onStatusChange', function(status, connected) {
    document.getElementById('status').textContent = status;
    document.getElementById('status').className = connected ? 'connected' : 'disconnected';
});

WSManager.on('onControlChange', function(hasCtrl) {
    hasControl = hasCtrl;
    updateControlUI();
});

WSManager.on('onEncoderData', function(data) {
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
});

WSManager.on('onBatteryData', function(voltage) {
    const batteryVoltage = voltage.toFixed(2);
    const batteryElement = document.getElementById('battery-voltage');
    batteryElement.textContent = batteryVoltage + ' V';
    
    // Color code based on voltage (assuming 3S LiPo: 9V-12.6V)
    if (voltage >= 11.1) {
        batteryElement.style.color = '#4CAF50'; // Green - Good
    } else if (voltage >= 10.2) {
        batteryElement.style.color = '#ff9800'; // Orange - Low
    } else {
        batteryElement.style.color = '#f44336'; // Red - Critical
    }
});

WSManager.on('onMotorData', function(left, right) {
    if (left !== undefined) {
        document.getElementById('motor-left').textContent = left;
    }
    if (right !== undefined) {
        document.getElementById('motor-right').textContent = right;
    }
});

function updateControlUI() {
    const controlText = document.getElementById('control-text');
    const controlBtn = document.getElementById('control-btn');
    const joystick = document.getElementById('joystick');
    
    if (hasControl) {
        controlText.textContent = 'You have control';
        controlText.style.color = '#4CAF50';
        controlBtn.textContent = 'Release Control';
        controlBtn.style.display = 'inline-block';
        controlBtn.classList.remove('button');
        controlBtn.classList.add('button', 'danger');
        joystick.classList.remove('disabled');
    } else {
        controlText.textContent = 'Someone else has control';
        controlText.style.color = '#ff9800';
        controlBtn.textContent = 'Request Control';
        controlBtn.style.display = 'inline-block';
        controlBtn.classList.remove('danger');
        controlBtn.classList.add('button');
        joystick.classList.add('disabled');
    }
}

function toggleControl() {
    if (hasControl) {
        WSManager.send('RELEASE_CONTROL');
    } else {
        WSManager.send('REQUEST_CONTROL');
    }
}

function resetEncoders() {
    if (!WSManager.send('RESET')) {
        console.error('WebSocket not connected');
    }
}

