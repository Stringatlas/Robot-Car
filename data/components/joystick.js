let joystick, stick, isDragging, centerX, centerY;

function initJoystick() {
    joystick = document.getElementById('joystick');
    stick = document.getElementById('stick');
    isDragging = false;

    if (!joystick || !stick) {
        console.error('Joystick elements not found');
        return;
    }

    // Calculate center based on actual joystick size
    function updateCenter() {
        const rect = joystick.getBoundingClientRect();
        centerX = rect.width / 2;
        centerY = rect.height / 2;
    }
    updateCenter();

    // Recalculate center on window resize
    window.addEventListener('resize', updateCenter);

    joystick.addEventListener('touchstart', function (e) {
        if (hasControl) {
            isDragging = true;
            updateCenter(); // Update center when touch starts
            e.preventDefault();
        }
    });

    joystick.addEventListener('touchmove', function (e) {
        if (isDragging && hasControl) {
            let rect = joystick.getBoundingClientRect();
            let x = e.touches[0].clientX - rect.left;
            let y = e.touches[0].clientY - rect.top;
            updateJoystick(x, y);
            e.preventDefault();
        }
    });

    // Listen on document level to catch touchend even if touch moves outside joystick
    document.addEventListener('touchend', function (e) {
        if (hasControl && isDragging) {
            isDragging = false;
            centerStick();
            sendJoystick(0, 0);
            e.preventDefault();
        }
    });

    document.addEventListener('touchcancel', function (e) {
        if (hasControl && isDragging) {
            isDragging = false;
            centerStick();
            sendJoystick(0, 0);
        }
    });

    // Also support mouse for desktop testing
    joystick.addEventListener('mousedown', function (e) {
        if (hasControl) {
            isDragging = true;
            e.preventDefault();
        }
    });

    document.addEventListener('mousemove', function (e) {
        if (isDragging && hasControl) {
            let rect = joystick.getBoundingClientRect();
            let x = e.clientX - rect.left;
            let y = e.clientY - rect.top;
            updateJoystick(x, y);
            e.preventDefault();
        }
    });

    document.addEventListener('mouseup', function (e) {
        if (isDragging && hasControl) {
            isDragging = false;
            centerStick();
            sendJoystick(0, 0);
            e.preventDefault();
        }
    });
}

// Helper function to center the stick
function centerStick() {
    const stickSize = parseFloat(getComputedStyle(stick).width);
    stick.style.left = (centerX - stickSize / 2) + 'px';
    stick.style.top = (centerY - stickSize / 2) + 'px';
}

function sendJoystick(x, y) {
    // Only send joystick data if we have control
    if (hasControl) {
        WSManager.send('JOYSTICK:' + x + ',' + y);
        // Update display
        document.getElementById('joy-x').textContent = x;
        document.getElementById('joy-y').textContent = y;
    }
}

function updateJoystick(x, y) {
    let deltaX = x - centerX;
    let deltaY = y - centerY;
    let distance = Math.sqrt(deltaX * deltaX + deltaY * deltaY);
    if (distance > 80) {
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

window.addEventListener('DOMContentLoaded', function () {
    initJoystick();
});