let calibrationRunning = false;
let currentMotor = null;
let calibrationData = [];
let canvas = document.getElementById('chart');
let ctx = canvas.getContext('2d');

// PID Autotuner state
let autotuneRunning = false;
let autotuneTimeout = null;
let autotuneData = {
    startTime: 0,
    targetVel: 0,
    motor: 'left',
    duration: 5000,
    aggressiveness: 1.0,
    samples: [],
    steadyStateReached: false,
    maxVel: 0,
    riseTime: 0,
    settlingTime: 0,
    overshoot: 0,
    steadyStateError: 0,
    kp: 0,
    ki: 0,
    kd: 0
};

// Register callbacks with WSManager for calibration-specific data
WSManager.on('onEncoderData', function(data) {
    const leftVel = parseFloat(data.left.velocity);
    const rightVel = parseFloat(data.right.velocity);
    const motorLeftPWM = data.motorLeft !== undefined ? parseFloat(data.motorLeft) : null;
    const motorRightPWM = data.motorRight !== undefined ? parseFloat(data.motorRight) : null;

    if (calibrationRunning) {
        updateCurrentData(leftVel, rightVel);
    }

    if (autotuneRunning) {
        if (motorLeftPWM !== null && motorRightPWM !== null) {
            autotuneData.lastLeftPWM = motorLeftPWM;
            autotuneData.lastRightPWM = motorRightPWM;
            autotuneData.lastAbsPWM = (Math.abs(motorLeftPWM) + Math.abs(motorRightPWM)) / 2.0;
        }
        processAutotuneData(leftVel, rightVel);
    }
});

// Handle calibration-specific string messages
WSManager.on('onRawMessage', function(data) {
    if (data.startsWith('VEL_ERROR:')) {
        const parts = data.substring(10).split(',');
        const leftError = parseFloat(parts[0]);
        const rightError = parseFloat(parts[1]);
        const pidEnabled = parts[2] === 'true';

        document.getElementById('pidStatus').textContent = 
            `PID Status: ${pidEnabled ? 'Enabled' : 'Disabled'} | ` +
            `Left Error: ${leftError.toFixed(2)} cm/s | Right Error: ${rightError.toFixed(2)} cm/s`;

        document.getElementById('velocityStatus').textContent = 
            `Velocity Tracking - Left Error: ${leftError.toFixed(1)} cm/s | Right Error: ${rightError.toFixed(1)} cm/s`;
    } else if (data.startsWith('COMMAND_ACK:')) {
        const parts = data.split(':');
        if (parts.length >= 3 && parts[1] === 'VELOCITY') {
            const ackVel = parseFloat(parts[2]);
            console.log('Server ACKed VELOCITY command:', ackVel);
            updateAutotuneStatus(`Server ACKed velocity ${ackVel.toFixed(1)} cm/s`);
        }
    } else if (data.startsWith('CALIBRATION_POINT:')) {
        const parts = data.substring(18).split(',');
        const pwm = parseInt(parts[0]);
        const leftVel = parseFloat(parts[1]);
        const rightVel = parseFloat(parts[2]);
        addDataPoint(pwm, leftVel, rightVel);
    } else if (data.startsWith('CALIBRATION_COMPLETE')) {
        stopCalibration();
        updateStatus('Calibration complete!');
    } else if (data.startsWith('CALIBRATION_PROGRESS:')) {
        const progress = data.substring(21);
        updateStatus(`Calibrating... ${progress}`);
    }
});

function updateStatus(message) {
    document.getElementById('status').textContent = 'Status: ' + message;
}

// Direct Motor Control Functions
function setMotorPowers() {
    const leftPower = parseFloat(document.getElementById('left-motor-input').value);
    const rightPower = parseFloat(document.getElementById('right-motor-input').value);
    
    if (isNaN(leftPower) || isNaN(rightPower)) {
        alert('Please enter valid numbers for both motors');
        return;
    }
    
    const clampedLeft = Math.max(-1, Math.min(1, leftPower));
    const clampedRight = Math.max(-1, Math.min(1, rightPower));
    
    WSManager.send('MOTORS:' + clampedLeft.toFixed(2) + ',' + clampedRight.toFixed(2));
    console.log('Set motors - Left:', clampedLeft, 'Right:', clampedRight);
}

function stopMotors() {
    WSManager.send('MOTORS:0,0');
    WSManager.send('VELOCITY:0');
    document.getElementById('left-motor-input').value = 0;
    document.getElementById('right-motor-input').value = 0;
}

// Manual Velocity Feedforward Tuning Functions
function updateFFGainDisplay() {
    const value = document.getElementById('ff-gain-slider').value;
    document.getElementById('ff-gain-value').textContent = parseFloat(value).toFixed(1);
}

function updateDeadzoneDisplay() {
    const value = document.getElementById('deadzone-slider').value;
    document.getElementById('deadzone-value').textContent = value;
}

function setFFGain() {
    const gain = parseFloat(document.getElementById('ff-gain-slider').value);
    if (ws && ws.readyState === WebSocket.OPEN) {
        WSManager.send('FF_GAIN:' + gain.toFixed(2));
        console.log('Set FF gain:', gain);
    }
}

function zeroGain() {
    document.getElementById('ff-gain-slider').value = 0;
    updateFFGainDisplay();
    setFFGain();
}

function setDeadzone() {
    const deadzone = parseInt(document.getElementById('deadzone-slider').value);
    if (ws && ws.readyState === WebSocket.OPEN) {
        WSManager.send('DEADZONE:' + deadzone);
        console.log('Set deadzone:', deadzone);
    }
}

function setVelocity() {
    const velocity = parseFloat(document.getElementById('velocity-input').value);
    if (ws && ws.readyState === WebSocket.OPEN) {
        WSManager.send('VELOCITY:' + velocity.toFixed(1));
        console.log('Set velocity:', velocity);
    }
}

function stopVelocity() {
    if (ws && ws.readyState === WebSocket.OPEN) {
        WSManager.send('VELOCITY:0');
    }
    document.getElementById('velocity-input').value = 0;
}

// Polynomial Mapping Functions
function sendVel2PWMPolynomial() {
    const a0 = parseFloat(document.getElementById('vel2pwm_a0').value) || 0;
    const a1 = parseFloat(document.getElementById('vel2pwm_a1').value) || 0;
    const a2 = parseFloat(document.getElementById('vel2pwm_a2').value) || 0;
    const a3 = parseFloat(document.getElementById('vel2pwm_a3').value) || 0;
    
    // Format: POLY_VEL2PWM:degree,a0,a1,a2,a3
    const message = `POLY_VEL2PWM:3,${a0},${a1},${a2},${a3}`;
    
    if (ws && ws.readyState === WebSocket.OPEN) {
        WSManager.send(message);
        console.log('Sent velocity->PWM polynomial:', {a0, a1, a2, a3});
        document.getElementById('polyStatus').textContent = 
            `Velocity→PWM polynomial updated: a₀=${a0.toFixed(4)}, a₁=${a1.toFixed(4)}, a₂=${a2.toFixed(6)}, a₃=${a3.toFixed(8)}`;
    } else {
        alert('WebSocket not connected!');
    }
}

function sendPWM2VelPolynomial() {
    const b0 = parseFloat(document.getElementById('pwm2vel_b0').value) || 0;
    const b1 = parseFloat(document.getElementById('pwm2vel_b1').value) || 0;
    const b2 = parseFloat(document.getElementById('pwm2vel_b2').value) || 0;
    const b3 = parseFloat(document.getElementById('pwm2vel_b3').value) || 0;
    
    // Format: POLY_PWM2VEL:degree,b0,b1,b2,b3
    const message = `POLY_PWM2VEL:3,${b0},${b1},${b2},${b3}`;
    
    if (ws && ws.readyState === WebSocket.OPEN) {
        WSManager.send(message);
        console.log('Sent PWM->velocity polynomial:', {b0, b1, b2, b3});
        document.getElementById('polyStatus').textContent = 
            `PWM→Velocity polynomial updated: b₀=${b0.toFixed(4)}, b₁=${b1.toFixed(4)}, b₂=${b2.toFixed(6)}, b₃=${b3.toFixed(8)}`;
    } else {
        alert('WebSocket not connected!');
    }
}

function togglePolynomialMapping() {
    const enabled = document.getElementById('polyEnable').checked;
    
    if (ws && ws.readyState === WebSocket.OPEN) {
        WSManager.send('POLY_ENABLE:' + (enabled ? 'true' : 'false'));
        console.log('Polynomial mapping:', enabled ? 'enabled' : 'disabled');
        document.getElementById('polyStatus').textContent = enabled ? 
            '✓ Polynomial mapping ENABLED - using cubic regression' : 
            '✗ Polynomial mapping disabled - using linear feedforward';
    } else {
        alert('WebSocket not connected!');
    }
}

// PID Autotuner Functions
function startPIDAutotune() {
    const targetVel = parseFloat(document.getElementById('autotune-velocity').value);
    const duration = parseFloat(document.getElementById('test-duration').value) * 1000; // Convert to ms
    const motor = document.getElementById('autotune-motor').value;
    const aggressiveness = parseFloat(document.getElementById('tune-aggressiveness').value);
    
    // Clear any existing timeout
    if (autotuneTimeout) {
        clearTimeout(autotuneTimeout);
    }
    
    // Reset autotune state
    autotuneData = {
        startTime: Date.now(),
        targetVel: targetVel,
        motor: motor,
        duration: duration,
        aggressiveness: aggressiveness,
        samples: [],
        steadyStateReached: false,
        maxVel: 0,
        riseTime: 0,
        settlingTime: 0,
        overshoot: 0,
        steadyStateError: 0,
        kp: 0,
        ki: 0,
        kd: 0
    };
    
    autotuneRunning = true;
    document.getElementById('startAutotune').disabled = true;
    document.getElementById('stopAutotune').disabled = false;
    document.getElementById('autotuneResults').style.display = 'none';
    updateAutotuneStatus('Starting at 0 velocity...');
    
    // Make sure PID is disabled for clean step response
    if (ws && ws.readyState === WebSocket.OPEN) {
        WSManager.send('PID_ENABLE:false');
        // Start at zero
        WSManager.send('VELOCITY:0');
    }
    
    // After 500ms, apply step input and start timeout
    setTimeout(() => {
        if (autotuneRunning && ws && ws.readyState === WebSocket.OPEN) {
            autotuneData.startTime = Date.now(); // Reset start time
            WSManager.send('VELOCITY:' + targetVel.toFixed(1));
            updateAutotuneStatus('Step applied! Measuring response...');
            
            // Set timeout to force completion after duration + 500ms buffer
            autotuneTimeout = setTimeout(() => {
                if (autotuneRunning) {
                    console.log('Autotune timeout reached, calculating gains...');
                    calculatePIDGains();
                    stopPIDAutotune();
                }
            }, duration + 500);
        }
    }, 500);
    
    console.log('Started PID autotuning (step response):', autotuneData);
}

function stopPIDAutotune() {
    autotuneRunning = false;
    document.getElementById('startAutotune').disabled = false;
    document.getElementById('stopAutotune').disabled = true;
    
    // Clear timeout
    if (autotuneTimeout) {
        clearTimeout(autotuneTimeout);
        autotuneTimeout = null;
    }
    
    if (ws && ws.readyState === WebSocket.OPEN) {
        WSManager.send('VELOCITY:0');
    }
    
    updateAutotuneStatus('Autotuning stopped');
}

function updateAutotuneStatus(message) {
    document.getElementById('autotuneStatus').textContent = 'Status: ' + message;
}

function processAutotuneData(leftVel, rightVel) {
    if (!autotuneRunning) return;
    
    const now = Date.now();
    const elapsed = now - autotuneData.startTime;
    
    // Get velocity for selected motor
    const velocity = autotuneData.motor === 'left' ? leftVel : 
                    autotuneData.motor === 'right' ? rightVel : 
                    (leftVel + rightVel) / 2;

    // Sanity check: if motor PWM is saturated (near 255) the response will be useless
    if (autotuneData.lastAbsPWM !== undefined && autotuneData.lastAbsPWM !== null) {
        // lastAbsPWM is in 0..255
        if (Math.abs(autotuneData.lastAbsPWM) >= 245) {
            // Stop autotune and warn the user
            updateAutotuneStatus('✗ Aborting: motor PWM saturated during test');
            alert('Autotune aborted: motor output saturated. Lower the target velocity or retune feedforward/deadzone first.');
            stopPIDAutotune();
            return;
        }
    }
    
    // Store sample
    autotuneData.samples.push({ 
        time: elapsed, 
        velocity: velocity 
    });
    
    // Log first few samples for debugging
    if (autotuneData.samples.length <= 3) {
        console.log(`Sample ${autotuneData.samples.length}: t=${elapsed}ms, v=${velocity.toFixed(1)} cm/s`);
    }
    
    // Update max velocity
    if (velocity > autotuneData.maxVel) {
        autotuneData.maxVel = velocity;
    }
    
    // Update status (include PWM if available)
    const progress = Math.min(100, (elapsed / autotuneData.duration) * 100);
    let statusText = `Measuring response... ${progress.toFixed(0)}% (${velocity.toFixed(1)} cm/s) - ${autotuneData.samples.length} samples`;
    if (autotuneData.lastAbsPWM !== undefined && autotuneData.lastAbsPWM !== null) {
        statusText += ` | Motor PWM ≈ ${autotuneData.lastAbsPWM.toFixed(0)}`;
    }
    updateAutotuneStatus(statusText);
    
    // Check if test duration completed
    if (elapsed >= autotuneData.duration) {
        console.log('Duration reached, calling calculatePIDGains...');
        calculatePIDGains();
        stopPIDAutotune();
    }
}

function calculatePIDGains() {
    console.log('calculatePIDGains called with', autotuneData.samples.length, 'samples');
    
    if (autotuneData.samples.length < 10) {
        updateAutotuneStatus('Not enough data collected - only ' + autotuneData.samples.length + ' samples');
        alert('Not enough data collected. Only got ' + autotuneData.samples.length + ' samples. Make sure encoder data is being received.');
        return;
    }
    
    const target = autotuneData.targetVel;
    const samples = autotuneData.samples;
    
    // Find rise time (time to reach 90% of target)
    const ninetyPercent = target * 0.9;
    let riseIdx = samples.findIndex(s => s.velocity >= ninetyPercent);
    if (riseIdx === -1) riseIdx = samples.length - 1;
    autotuneData.riseTime = samples[riseIdx].time / 1000; // Convert to seconds
    
    // Find peak overshoot
    autotuneData.overshoot = ((autotuneData.maxVel - target) / target) * 100;
    
    // Calculate steady-state values (last 20% of samples)
    const steadyStartIdx = Math.floor(samples.length * 0.8);
    const steadySamples = samples.slice(steadyStartIdx);
    const steadyStateVel = steadySamples.reduce((sum, s) => sum + s.velocity, 0) / steadySamples.length;
    autotuneData.steadyStateError = target - steadyStateVel;
    
    // Find settling time (time to stay within 5% of target)
    const fivePercent = target * 0.05;
    let settledIdx = samples.length - 1;
    for (let i = riseIdx; i < samples.length; i++) {
        const inBand = Math.abs(samples[i].velocity - target) < fivePercent;
        if (inBand && i < samples.length - 10) {
            // Check if it stays in band for at least 10 samples
            let staysInBand = true;
            for (let j = i; j < Math.min(i + 10, samples.length); j++) {
                if (Math.abs(samples[j].velocity - target) >= fivePercent) {
                    staysInBand = false;
                    break;
                }
            }
            if (staysInBand) {
                settledIdx = i;
                break;
            }
        }
    }
    autotuneData.settlingTime = samples[settledIdx].time / 1000; // Convert to seconds
    
    // Calculate PID gains using Cohen-Coon-inspired method
    // Based on rise time, overshoot, and steady-state error
    const Tr = autotuneData.riseTime;
    const Ts = autotuneData.settlingTime;
    const SSE = Math.abs(autotuneData.steadyStateError);
    const aggressiveness = autotuneData.aggressiveness;
    
    // Kp: Higher if there's steady-state error, moderate rise time
    // Base Kp on error magnitude - more error needs more gain
    const baseKp = SSE > 5 ? 0.8 : SSE > 2 ? 0.5 : 0.3;
    autotuneData.kp = (baseKp / Math.max(Tr, 0.1)) * aggressiveness;
    
    // Ki: Eliminate steady-state error, but not too aggressive
    // Integral time constant should be related to settling time
    autotuneData.ki = (autotuneData.kp / Math.max(Ts * 2, 0.5)) * aggressiveness;
    
    // Kd: Reduce overshoot and improve response
    // More Kd if there's overshoot
    const kdFactor = autotuneData.overshoot > 10 ? 0.15 : 
                    autotuneData.overshoot > 5 ? 0.10 : 0.05;
    autotuneData.kd = (autotuneData.kp * Tr * kdFactor) * aggressiveness;
    
    // Clamp values to reasonable ranges
    autotuneData.kp = Math.max(0.1, Math.min(5.0, autotuneData.kp));
    autotuneData.ki = Math.max(0.01, Math.min(2.0, autotuneData.ki));
    autotuneData.kd = Math.max(0.0, Math.min(0.5, autotuneData.kd));
    
    // Display results
    const resultsDiv = document.getElementById('autotuneResults');
    const resultsContent = document.getElementById('autotuneResultsContent');
    
    const aggrValue = autotuneData.aggressiveness;
    const aggrText = aggrValue < 0.8 ? 'conservative' : aggrValue > 1.5 ? 'aggressive' : 'balanced';
    
    resultsContent.innerHTML = `
        <strong style="font-size: 14px;">Step Response Characteristics:</strong><br>
        Target Velocity: ${target.toFixed(1)} cm/s<br>
        Peak Velocity: ${autotuneData.maxVel.toFixed(1)} cm/s<br>
        Steady-State Velocity: ${steadyStateVel.toFixed(1)} cm/s<br>
        Rise Time (0-90%): ${(Tr * 1000).toFixed(0)} ms<br>
        Settling Time (±5%): ${(Ts * 1000).toFixed(0)} ms<br>
        Overshoot: ${autotuneData.overshoot.toFixed(1)}%<br>
        Steady-State Error: ${autotuneData.steadyStateError.toFixed(1)} cm/s<br>
        <br>
        <strong style="font-size: 14px; color: #4CAF50;">Calculated PID Gains:</strong><br>
        <span style="font-size: 15px; color: #fff;">
        Kp (Proportional): <strong>${autotuneData.kp.toFixed(3)}</strong><br>
        Ki (Integral): <strong>${autotuneData.ki.toFixed(3)}</strong><br>
        Kd (Derivative): <strong>${autotuneData.kd.toFixed(3)}</strong><br>
        </span>
        <br>
        <em style="color: #4CAF50;">✓ Gains tuned for ${aggrText} response</em><br>
        <em style="color: #888;">Tip: If response is too oscillatory, reduce aggressiveness and re-run</em>
    `;
    
    // Force display the results
    resultsDiv.style.display = 'block';
    resultsDiv.style.visibility = 'visible';
    updateAutotuneStatus('✓ Autotuning complete! Results shown below.');
    
    console.log('Results div displayed, style:', resultsDiv.style.display);
    console.log('Autotuned PID gains:', autotuneData);
    console.log(`Collected ${autotuneData.samples.length} samples over ${(autotuneData.duration/1000).toFixed(1)}s`);
    
    // Scroll results into view after a small delay to ensure rendering
    setTimeout(() => {
        resultsDiv.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
    }, 100);
}

function applyAutotuneGains() {
    if (ws && ws.readyState === WebSocket.OPEN) {
        WSManager.send(`PID_GAINS:${autotuneData.kp.toFixed(3)},${autotuneData.ki.toFixed(3)},${autotuneData.kd.toFixed(3)}`);
        
        // Update the PID tuning section inputs
        document.getElementById('pidKp').value = autotuneData.kp.toFixed(3);
        document.getElementById('pidKi').value = autotuneData.ki.toFixed(3);
        document.getElementById('pidKd').value = autotuneData.kd.toFixed(3);
        
        alert('PID gains applied! You can now enable PID and test.');
    }
}

function startCalibration(motor) {
    if (calibrationRunning) return;
    
    const startPWM = parseInt(document.getElementById('startPWM').value);
    const endPWM = parseInt(document.getElementById('endPWM').value);
    const stepSize = parseInt(document.getElementById('stepSize').value);
    const holdTime = parseInt(document.getElementById('holdTime').value);
    
    calibrationRunning = true;
    currentMotor = motor;
    calibrationData = [];
    
    // Disable start buttons, enable stop
    document.getElementById('startLeft').disabled = true;
    document.getElementById('startRight').disabled = true;
    document.getElementById('startBoth').disabled = true;
    document.getElementById('stopBtn').disabled = false;
    
    // Clear chart and table
    clearChart();
    clearTable();
    
    updateStatus(`Starting ${motor} motor calibration...`);
    
    // Send start command
    WSManager.send(`START_CALIBRATION:${motor},${startPWM},${endPWM},${stepSize},${holdTime}`);
}

function stopCalibration() {
    calibrationRunning = false;
    currentMotor = null;
    
    WSManager.send('STOP_CALIBRATION');
    
    document.getElementById('startLeft').disabled = false;
    document.getElementById('startRight').disabled = false;
    document.getElementById('startBoth').disabled = false;
    document.getElementById('stopBtn').disabled = true;
    
    updateStatus('Calibration stopped');
}

function updateCurrentData(leftVel, rightVel) {
    // This will be updated with PWM value from the controller
    // For now, data is accumulated when we receive CALIBRATION_POINT message
}

function addDataPoint(pwm, leftVel, rightVel) {
    calibrationData.push({pwm, leftVel, rightVel});
    
    // Add to table
    const table = document.getElementById('dataTable');
    const row = table.insertRow();
    row.insertCell(0).textContent = pwm;
    row.insertCell(1).textContent = leftVel.toFixed(2);
    row.insertCell(2).textContent = rightVel.toFixed(2);
    
    // Update chart
    drawChart();
}

function clearChart() {
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    drawGrid();
}

function clearTable() {
    document.getElementById('dataTable').innerHTML = '';
}

function drawGrid() {
    ctx.strokeStyle = '#444';
    ctx.lineWidth = 1;
    
    // Vertical lines
    for (let i = 0; i <= 10; i++) {
        const x = (canvas.width / 10) * i;
        ctx.beginPath();
        ctx.moveTo(x, 0);
        ctx.lineTo(x, canvas.height);
        ctx.stroke();
    }
    
    // Horizontal lines
    for (let i = 0; i <= 10; i++) {
        const y = (canvas.height / 10) * i;
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(canvas.width, y);
        ctx.stroke();
    }
}

function drawChart() {
    clearChart();
    
    if (calibrationData.length === 0) return;
    
    const maxPWM = 255;
    const maxVel = Math.max(...calibrationData.map(d => Math.max(d.leftVel, d.rightVel))) * 1.1;
    
    // Draw left motor data (blue)
    ctx.strokeStyle = '#2196F3';
    ctx.lineWidth = 2;
    ctx.beginPath();
    calibrationData.forEach((point, i) => {
        const x = (point.pwm / maxPWM) * canvas.width;
        const y = canvas.height - (point.leftVel / maxVel) * canvas.height;
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
    });
    ctx.stroke();
    
    // Draw right motor data (red)
    ctx.strokeStyle = '#f44336';
    ctx.beginPath();
    calibrationData.forEach((point, i) => {
        const x = (point.pwm / maxPWM) * canvas.width;
        const y = canvas.height - (point.rightVel / maxVel) * canvas.height;
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
    });
    ctx.stroke();
    
    // Draw points
    calibrationData.forEach(point => {
        const x = (point.pwm / maxPWM) * canvas.width;
        const yLeft = canvas.height - (point.leftVel / maxVel) * canvas.height;
        const yRight = canvas.height - (point.rightVel / maxVel) * canvas.height;
        
        ctx.fillStyle = '#2196F3';
        ctx.fillRect(x - 3, yLeft - 3, 6, 6);
        
        ctx.fillStyle = '#f44336';
        ctx.fillRect(x - 3, yRight - 3, 6, 6);
    });
    
    // Draw legend
    ctx.fillStyle = '#2196F3';
    ctx.fillRect(10, 10, 20, 10);
    ctx.fillStyle = '#fff';
    ctx.fillText('Left Motor', 35, 20);
    
    ctx.fillStyle = '#f44336';
    ctx.fillRect(10, 30, 20, 10);
    ctx.fillStyle = '#fff';
    ctx.fillText('Right Motor', 35, 40);
}

function exportCSV() {
    let csv = 'PWM,Left Velocity (cm/s),Right Velocity (cm/s)\n';
    calibrationData.forEach(point => {
        csv += `${point.pwm},${point.leftVel},${point.rightVel}\n`;
    });
    
    const blob = new Blob([csv], {type: 'text/csv'});
    const url = window.URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'motor_calibration.csv';
    a.click();
}

function exportJSON() {
    const json = JSON.stringify(calibrationData, null, 2);
    const blob = new Blob([json], {type: 'application/json'});
    const url = window.URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'motor_calibration.json';
    a.click();
}

function calculateModel() {
    if (calibrationData.length < 2) {
        alert('Need at least 2 data points');
        return;
    }
    
    // Find deadzone (first PWM where velocity > 0)
    let leftDeadzone = 0;
    let rightDeadzone = 0;
    
    for (let point of calibrationData) {
        if (leftDeadzone === 0 && Math.abs(point.leftVel) > 1.0) {
            leftDeadzone = point.pwm;
        }
        if (rightDeadzone === 0 && Math.abs(point.rightVel) > 1.0) {
            rightDeadzone = point.pwm;
        }
    }
    
    // Linear regression for points above deadzone
    const leftPoints = calibrationData.filter(p => p.pwm >= leftDeadzone && Math.abs(p.leftVel) > 1.0);
    const rightPoints = calibrationData.filter(p => p.pwm >= rightDeadzone && Math.abs(p.rightVel) > 1.0);
    
    const leftGain = linearRegression(leftPoints.map(p => p.leftVel), leftPoints.map(p => p.pwm - leftDeadzone));
    const rightGain = linearRegression(rightPoints.map(p => p.rightVel), rightPoints.map(p => p.pwm - rightDeadzone));
    
    const result = `
Motor Calibration Results:
==========================

Left Motor:
Deadzone: ${leftDeadzone} PWM
Gain: ${leftGain.toFixed(3)} PWM per cm/s
Model: PWM = ${leftDeadzone} + ${leftGain.toFixed(3)} × velocity

Right Motor:
Deadzone: ${rightDeadzone} PWM
Gain: ${rightGain.toFixed(3)} PWM per cm/s
Model: PWM = ${rightDeadzone} + ${rightGain.toFixed(3)} × velocity
`;
    
    alert(result);
    console.log(result);
}

function linearRegression(x, y) {
    const n = x.length;
    const sumX = x.reduce((a, b) => a + b, 0);
    const sumY = y.reduce((a, b) => a + b, 0);
    const sumXY = x.reduce((acc, xi, i) => acc + xi * y[i], 0);
    const sumXX = x.reduce((acc, xi) => acc + xi * xi, 0);
    
    const slope = (n * sumXY - sumX * sumY) / (n * sumXX - sumX * sumX);
    return slope;
}

// PID control functions
let pidEnabled = false;

function updatePIDGains() {
    const kp = parseFloat(document.getElementById('pidKp').value);
    const ki = parseFloat(document.getElementById('pidKi').value);
    const kd = parseFloat(document.getElementById('pidKd').value);
    
    WSManager.send(`PID_GAINS:${kp},${ki},${kd}`);
    console.log(`PID gains set: Kp=${kp}, Ki=${ki}, Kd=${kd}`);
}

function togglePID() {
    pidEnabled = !pidEnabled;
    WSManager.send(`PID_ENABLE:${pidEnabled}`);
    document.getElementById('pidToggle').textContent = pidEnabled ? 'Disable PID' : 'Enable PID';
    console.log(`PID ${pidEnabled ? 'enabled' : 'disabled'}`);
}

function setTargetVelocity() {
    const vel = parseFloat(document.getElementById('targetVel').value);
    WSManager.send(`VELOCITY:${vel}`);
    console.log(`Target velocity set: ${vel} cm/s`);
}

function stopMotors() {
    WSManager.send('VELOCITY:0');
    console.log('Motors stopped');
}

// Initialize on page load
window.addEventListener('load', () => {
    drawGrid();
    updateFFGainDisplay();
    updateDeadzoneDisplay();
    
    // Load configuration from robot on page load
    setTimeout(() => {
        loadConfigFromRobot();
    }, 1000); // Wait 1 second for WebSocket to connect
});

// Configuration Manager Functions
function loadConfigFromRobot() {
    if (!ws || ws.readyState !== WebSocket.OPEN) {
        updateConfigStatus('❌ WebSocket not connected! Cannot load configuration.');
        return;
    }
    
    updateConfigStatus('📡 Requesting configuration from robot...');
    WSManager.send('CONFIG_GET');
}

function saveConfigToRobot() {
    if (!ws || ws.readyState !== WebSocket.OPEN) {
        updateConfigStatus('❌ WebSocket not connected! Cannot save configuration.');
        return;
    }
    
    // Collect all values from the form
    const config = {
        feedforwardGain: parseFloat(document.getElementById('config_feedforwardGain').value) || 3.0,
        deadzonePWM: parseFloat(document.getElementById('config_deadzonePWM').value) || 60.0,
        pidEnabled: document.getElementById('config_pidEnabled').checked,
        pidKp: parseFloat(document.getElementById('config_pidKp').value) || 0.0,
        pidKi: parseFloat(document.getElementById('config_pidKi').value) || 0.0,
        pidKd: parseFloat(document.getElementById('config_pidKd').value) || 0.0,
        polynomialEnabled: document.getElementById('config_polynomialEnabled').checked,
        vel2pwm_a0: parseFloat(document.getElementById('config_vel2pwm_a0').value) || 0.0,
        vel2pwm_a1: parseFloat(document.getElementById('config_vel2pwm_a1').value) || 1.0,
        vel2pwm_a2: parseFloat(document.getElementById('config_vel2pwm_a2').value) || 0.0,
        vel2pwm_a3: parseFloat(document.getElementById('config_vel2pwm_a3').value) || 0.0,
        pwm2vel_b0: parseFloat(document.getElementById('config_pwm2vel_b0').value) || 0.0,
        pwm2vel_b1: parseFloat(document.getElementById('config_pwm2vel_b1').value) || 1.0,
        pwm2vel_b2: parseFloat(document.getElementById('config_pwm2vel_b2').value) || 0.0,
        pwm2vel_b3: parseFloat(document.getElementById('config_pwm2vel_b3').value) || 0.0
    };
    
    const jsonStr = JSON.stringify(config);
    WSManager.send('CONFIG_SET:' + jsonStr);
    updateConfigStatus('💾 Saving configuration to robot...');
    console.log('Sent configuration:', config);
}

function resetConfigToDefaults() {
    if (!confirm('Reset all parameters to default values? This will clear all calibration data on the robot.')) {
        return;
    }
    
    if (!ws || ws.readyState !== WebSocket.OPEN) {
        updateConfigStatus('❌ WebSocket not connected! Cannot reset configuration.');
        return;
    }
    
    WSManager.send('CONFIG_RESET');
    updateConfigStatus('🔄 Resetting configuration to defaults...');
}

function updateConfigStatus(message) {
    document.getElementById('configStatus').textContent = message;
}

function populateConfigForm(config) {
    // Populate all form fields with received configuration
    document.getElementById('config_feedforwardGain').value = config.feedforwardGain || 3.0;
    document.getElementById('config_deadzonePWM').value = config.deadzonePWM || 60.0;
    document.getElementById('config_pidEnabled').checked = config.pidEnabled || false;
    document.getElementById('config_pidKp').value = config.pidKp || 0.0;
    document.getElementById('config_pidKi').value = config.pidKi || 0.0;
    document.getElementById('config_pidKd').value = config.pidKd || 0.0;
    document.getElementById('config_polynomialEnabled').checked = config.polynomialEnabled || false;
    document.getElementById('config_vel2pwm_a0').value = config.vel2pwm_a0 || 0.0;
    document.getElementById('config_vel2pwm_a1').value = config.vel2pwm_a1 || 1.0;
    document.getElementById('config_vel2pwm_a2').value = config.vel2pwm_a2 || 0.0;
    document.getElementById('config_vel2pwm_a3').value = config.vel2pwm_a3 || 0.0;
    document.getElementById('config_pwm2vel_b0').value = config.pwm2vel_b0 || 0.0;
    document.getElementById('config_pwm2vel_b1').value = config.pwm2vel_b1 || 1.0;
    document.getElementById('config_pwm2vel_b2').value = config.pwm2vel_b2 || 0.0;
    document.getElementById('config_pwm2vel_b3').value = config.pwm2vel_b3 || 0.0;
    
    updateConfigStatus('✅ Configuration loaded successfully from robot');
    console.log('Configuration loaded:', config);
}

// Handle configuration responses from robot
WSManager.on('onRawMessage', (function() {
    const originalHandler = WSManager._callbacks.onRawMessage || [];
    return function(data) {
        // Call existing handlers
        originalHandler.forEach(handler => {
            if (handler !== arguments.callee) {
                handler(data);
            }
        });
        
        // Handle configuration messages
        if (data.startsWith('CONFIG_DATA:')) {
            const jsonStr = data.substring(12);
            try {
                const config = JSON.parse(jsonStr);
                populateConfigForm(config);
            } catch (e) {
                console.error('Failed to parse configuration:', e);
                updateConfigStatus('❌ Failed to parse configuration from robot');
            }
        } else if (data.startsWith('CONFIG_SAVED')) {
            updateConfigStatus('✅ Configuration saved to robot successfully');
        } else if (data.startsWith('CONFIG_RESET')) {
            updateConfigStatus('✅ Configuration reset to defaults');
            // Reload configuration after reset
            setTimeout(loadConfigFromRobot, 500);
        } else if (data.startsWith('CONFIG_ERROR:')) {
            const errorMsg = data.substring(13);
            updateConfigStatus('❌ Configuration error: ' + errorMsg);
        }
    };
})());
