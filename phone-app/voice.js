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

let speechRecognizer = null;
let isSpeechListening = false;

function initSpeechCommands() {
    const statusEl = document.getElementById('speech-model-status');
    if (typeof speechCommands === 'undefined') {
        statusEl.textContent = 'Speech Commands library not loaded';
        statusEl.className = 'model-status error';
        return;
    }
    statusEl.textContent = 'Ready to load model';
    statusEl.className = 'model-status loading';
}

async function loadSpeechModel() {
    const statusEl = document.getElementById('speech-model-status');
    const loadBtn = document.getElementById('load-speech-btn');
    const startBtn = document.getElementById('start-speech-btn');
    const vocabSelect = document.getElementById('speech-vocab-select');
    
    const vocabulary = vocabSelect.value;
    
    statusEl.textContent = 'Initializing TensorFlow...';
    statusEl.className = 'model-status loading';
    loadBtn.disabled = true;
    logToConsole(`Loading speech commands model (${vocabulary})...`, 'info');
    
    try {
        await tf.ready();
        logToConsole(`Using backend: ${tf.getBackend()}`, 'info');
        
        statusEl.textContent = 'Loading model...';
        speechRecognizer = speechCommands.create('BROWSER_FFT', vocabulary);
        await speechRecognizer.ensureModelLoaded();
        
        const words = speechRecognizer.wordLabels();
        statusEl.textContent = `Model loaded (${words.length} words)`;
        statusEl.className = 'model-status ready';
        startBtn.disabled = false;
        logToConsole(`Speech model loaded: ${words.join(', ')}`, 'success');
    } catch (error) {
        statusEl.textContent = 'Failed to load model: ' + error.message;
        statusEl.className = 'model-status error';
        loadBtn.disabled = false;
        logToConsole('Speech model error: ' + error.message, 'error');
    }
}

let speechDetectionBuffer = [];
let lastCommandTime = 0;
const REQUIRED_CONSECUTIVE = 2;
const COMMAND_COOLDOWN_MS = 500;

async function startSpeechRecognition() {
    if (!speechRecognizer || isSpeechListening) return;
    
    const startBtn = document.getElementById('start-speech-btn');
    const stopBtn = document.getElementById('stop-speech-btn');
    const indicator = document.getElementById('speech-indicator');
    const words = speechRecognizer.wordLabels();
    const noiseIndex = words.indexOf('_background_noise_');
    const unknownIndex = words.indexOf('_unknown_');
    
    speechDetectionBuffer = [];
    lastCommandTime = 0;
    
    try {
        await speechRecognizer.listen(result => {
            const scores = result.scores;
            const now = Date.now();
            
            let maxScore = 0;
            let maxIndex = 0;
            for (let i = 0; i < scores.length; i++) {
                if (scores[i] > maxScore) {
                    maxScore = scores[i];
                    maxIndex = i;
                }
            }
            
            const detectedWord = words[maxIndex];
            const confidence = (maxScore * 100).toFixed(1);
            const noiseScore = noiseIndex >= 0 ? scores[noiseIndex] : 0;
            const unknownScore = unknownIndex >= 0 ? scores[unknownIndex] : 0;
            
            document.getElementById('speech-result').textContent = 
                `${detectedWord} (${confidence}%)`;
            
            updateSpeechScores(words, scores);
            
            const isCommand = maxIndex !== noiseIndex && maxIndex !== unknownIndex;
            const clearlyAboveNoise = maxScore > noiseScore + 0.25 && maxScore > unknownScore + 0.25;
            
            if (isCommand && clearlyAboveNoise && maxScore > 0.9) {
                speechDetectionBuffer.push({ word: detectedWord, time: now });
                speechDetectionBuffer = speechDetectionBuffer.filter(d => now - d.time < 800);
                
                const recentSameWord = speechDetectionBuffer.filter(d => d.word === detectedWord);
                
                if (recentSameWord.length >= REQUIRED_CONSECUTIVE && now - lastCommandTime > COMMAND_COOLDOWN_MS) {
                    logToConsole(`Speech: ${detectedWord} (${confidence}%)`, 'success');
                    lastCommandTime = now;
                    speechDetectionBuffer = [];
                }
            } else {
                speechDetectionBuffer = [];
            }
        }, {
            probabilityThreshold: 0.6,
            overlapFactor: 0.20,
            invokeCallbackOnNoiseAndUnknown: true
        });
        
        isSpeechListening = true;
        indicator.classList.add('listening');
        startBtn.disabled = true;
        stopBtn.disabled = false;
        logToConsole('Speech recognition started', 'success');
    } catch (error) {
        logToConsole('Speech recognition error: ' + error.message, 'error');
    }
}

function stopSpeechRecognition() {
    if (!speechRecognizer || !isSpeechListening) return;
    
    speechRecognizer.stopListening();
    isSpeechListening = false;
    
    document.getElementById('speech-indicator').classList.remove('listening');
    document.getElementById('start-speech-btn').disabled = false;
    document.getElementById('stop-speech-btn').disabled = true;
    logToConsole('Speech recognition stopped', 'info');
}

function updateSpeechScores(words, scores) {
    const container = document.getElementById('speech-scores');
    const sorted = words.map((word, i) => ({ word, score: scores[i] }))
        .sort((a, b) => b.score - a.score)
        .slice(0, 5);
    
    container.innerHTML = sorted.map(item => {
        const pct = (item.score * 100).toFixed(1);
        return `<div class="detection-item">${item.word}: ${pct}%</div>`;
    }).join('');
}