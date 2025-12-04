let recognition = null;

function initVoiceControl() {
    if (!('webkitSpeechRecognition' in window) && !('SpeechRecognition' in window)) {
        document.getElementById('voice-status').textContent = '✗ Speech recognition not supported';
        document.getElementById('voice-status').className = 'model-status error';
        document.getElementById('start-voice-btn').disabled = true;
        logToConsole('Speech recognition not supported in this browser', 'error');
        return;
    }
    
    const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
    recognition = new SpeechRecognition();
    recognition.continuous = true;
    recognition.interimResults = false;
    recognition.lang = 'en-US';
    
    recognition.onresult = (event) => {
        const result = event.results[event.results.length - 1];
        const transcript = result[0].transcript.toLowerCase().trim();
        
        document.getElementById('voice-transcript').textContent = transcript;
        logToConsole(`Voice: "${transcript}"`, 'info');
        
        processVoiceCommand(transcript);
    };
    
    recognition.onerror = (event) => {
        console.error('Speech recognition error:', event.error);
        logToConsole('Voice error: ' + event.error, 'error');
    };
    
    recognition.onend = () => {
        if (document.getElementById('voice-indicator').classList.contains('listening')) {
            try {
                recognition.start();
            } catch (e) {
                // Ignore - likely already started
            }
        }
    };
}

function startVoiceControl() {
    if (!recognition) return;
    
    try {
        recognition.start();
        document.getElementById('voice-indicator').classList.add('listening');
        document.getElementById('start-voice-btn').disabled = true;
        document.getElementById('stop-voice-btn').disabled = false;
        logToConsole('Voice control started', 'success');
    } catch (error) {
        console.error('Error starting voice control:', error);
        logToConsole('Voice control error: ' + error.message, 'error');
    }
}

function stopVoiceControl() {
    if (!recognition) return;
    
    recognition.stop();
    document.getElementById('voice-indicator').classList.remove('listening');
    document.getElementById('start-voice-btn').disabled = false;
    document.getElementById('stop-voice-btn').disabled = true;
    logToConsole('Voice control stopped', 'info');
}

function processVoiceCommand(transcript) {
    let command = null;
    
    if (transcript.includes('forward') || transcript.includes('ahead') || transcript.includes('go')) {
        command = 'forward';
    } else if (transcript.includes('back') || transcript.includes('backward') || transcript.includes('reverse')) {
        command = 'backward';
    } else if (transcript.includes('left')) {
        command = 'left';
    } else if (transcript.includes('right')) {
        command = 'right';
    } else if (transcript.includes('stop') || transcript.includes('halt')) {
        command = 'stop';
    }
    
    if (command) {
        logToConsole(`Command recognized: ${command.toUpperCase()}`, 'success');
    } else {
        logToConsole(`Unknown voice command: "${transcript}"`, 'warning');
    }
}
