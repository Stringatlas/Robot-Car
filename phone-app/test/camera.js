let modelLoaded = false;
let cocoModel = null;
let videoStream = null;
let detectionActive = false;
let lastDetections = [];
let currentBackend = null;

let frameTimes = [];
let inferenceTotal = 0;
let inferenceCount = 0;
let fpsTotal = 0;
let fpsCount = 0;

let loadedModel = null;
let loadedBackend = null;

const resolutions = {
    '640x480': { width: 640, height: 480 },
    '480x360': { width: 480, height: 360 },
    '320x240': { width: 320, height: 240 },
    '160x120': { width: 160, height: 120 }
};

async function initTensorFlow() {
    const selectedBackend = document.getElementById('backend-select').value;
    const selectedModel = document.getElementById('model-select').value;
    
    try {
        logToConsole('Initializing TensorFlow.js...', 'info');
        document.getElementById('detection-status').textContent = 'Initializing backend...';
        document.getElementById('detection-status').className = 'model-status loading';
        
        currentBackend = await selectBestBackend(selectedBackend);
        logToConsole(`Using ${currentBackend.toUpperCase()} backend`, 'success');
        
        document.getElementById('detection-status').textContent = `Loading ${selectedModel} model...`;
        logToConsole(`Loading COCO-SSD (${selectedModel})...`, 'info');
        
        cocoModel = await cocoSsd.load({ base: selectedModel });
        modelLoaded = true;
        
        document.getElementById('detection-status').textContent = `✓ Ready (${currentBackend.toUpperCase()} + ${selectedModel})`;
        document.getElementById('detection-status').className = 'model-status ready';
        logToConsole('COCO-SSD model loaded successfully', 'success');
        
        return true;
        
    } catch (error) {
        console.error('Error loading model:', error);
        document.getElementById('detection-status').textContent = '✗ Error: ' + error.message;
        document.getElementById('detection-status').className = 'model-status error';
        logToConsole('Error loading model: ' + error.message, 'error');
        return false;
    }
}

async function selectBestBackend(preferred) {
    const backends = preferred === 'auto' 
        ? ['webgpu', 'wasm', 'webgl'] 
        : [preferred];
    
    for (const backend of backends) {
        try {
            if (backend === 'webgpu') {
                if (!('gpu' in navigator)) {
                    logToConsole('WebGPU not available in this browser', 'warning');
                    continue;
                }
            }
            
            if (backend === 'wasm') {
                tf.wasm.setWasmPaths('https://cdn.jsdelivr.net/npm/@tensorflow/tfjs-backend-wasm@4.17.0/dist/');
            }
            
            await tf.setBackend(backend);
            await tf.ready();
            
            if (backend === 'wasm') {
                const hasSimd = await tf.env().getAsync('WASM_HAS_SIMD_SUPPORT');
                const hasThreads = await tf.env().getAsync('WASM_HAS_MULTITHREAD_SUPPORT');
                const features = [];
                if (hasSimd) features.push('SIMD');
                if (hasThreads) features.push('Multi-thread');
                logToConsole(`WASM features: ${features.length ? features.join(' + ') : 'Basic'}`, 'info');
            }
            
            return backend;
            
        } catch (e) {
            logToConsole(`${backend.toUpperCase()} backend failed: ${e.message}`, 'warning');
        }
    }
    
    throw new Error('No suitable backend available');
}

async function initAndStart() {
    document.getElementById('start-camera-btn').disabled = true;
    document.getElementById('start-camera-btn').textContent = 'Loading...';
    
    const selectedModel = document.getElementById('model-select').value;
    const selectedBackend = document.getElementById('backend-select').value;
    
    const needsReload = !modelLoaded || 
                        loadedModel !== selectedModel || 
                        loadedBackend !== selectedBackend;
    
    if (needsReload) {
        if (cocoModel) {
            cocoModel.dispose();
            cocoModel = null;
            modelLoaded = false;
        }
        
        const success = await initTensorFlow();
        if (!success) {
            document.getElementById('start-camera-btn').disabled = false;
            document.getElementById('start-camera-btn').textContent = 'Load Model & Start';
            return;
        }
        
        loadedModel = selectedModel;
        loadedBackend = selectedBackend;
    }
    
    await startCamera();
}

async function getCameras() {
    try {
        const devices = await navigator.mediaDevices.enumerateDevices();
        const videoDevices = devices.filter(device => device.kind === 'videoinput');
        
        const select = document.getElementById('camera-select');
        select.innerHTML = '<option value="">Select Camera...</option>';
        
        videoDevices.forEach((device, index) => {
            const option = document.createElement('option');
            option.value = device.deviceId;
            option.text = device.label || `Camera ${index + 1}`;
            select.appendChild(option);
        });
        
        const rearCamera = videoDevices.find(d => 
            d.label.toLowerCase().includes('back') || 
            d.label.toLowerCase().includes('rear')
        );
        if (rearCamera) {
            select.value = rearCamera.deviceId;
        }
        
    } catch (error) {
        console.error('Error enumerating cameras:', error);
        logToConsole('Error getting cameras: ' + error.message, 'error');
    }
}

async function startCamera() {
    try {
        const selectedCamera = document.getElementById('camera-select').value;
        const selectedResolution = document.getElementById('resolution-select').value;
        const res = resolutions[selectedResolution] || resolutions['320x240'];
        
        const constraints = {
            video: {
                facingMode: selectedCamera ? undefined : { ideal: 'environment' },
                deviceId: selectedCamera ? { exact: selectedCamera } : undefined,
                width: { ideal: res.width },
                height: { ideal: res.height }
            }
        };
        
        videoStream = await navigator.mediaDevices.getUserMedia(constraints);
        
        const video = document.getElementById('camera-feed');
        video.srcObject = videoStream;
        
        video.onloadedmetadata = () => {
            const canvas = document.getElementById('canvas-overlay');
            canvas.width = video.videoWidth;
            canvas.height = video.videoHeight;
            logToConsole(`Actual video resolution: ${video.videoWidth}×${video.videoHeight}`, 'info');
        };
        
        document.getElementById('video-container').style.display = 'block';
        document.getElementById('start-camera-btn').disabled = true;
        document.getElementById('start-camera-btn').textContent = 'Running';
        document.getElementById('stop-camera-btn').disabled = false;
        document.getElementById('toggle-detection-btn').disabled = !modelLoaded;
        
        document.getElementById('resolution-select').disabled = true;
        document.getElementById('model-select').disabled = true;
        document.getElementById('backend-select').disabled = true;
        
        logToConsole(`Camera started (${res.width}×${res.height})`, 'success');
        
        await getCameras();
        
    } catch (error) {
        console.error('Error accessing camera:', error);
        logToConsole('Camera error: ' + error.message, 'error');
        alert('Cannot access camera. Please check permissions.');
    }
}

function stopCamera() {
    if (videoStream) {
        videoStream.getTracks().forEach(track => track.stop());
        videoStream = null;
    }
    
    if (detectionActive) {
        toggleDetection();
    }
    
    document.getElementById('video-container').style.display = 'none';
    document.getElementById('start-camera-btn').disabled = false;
    document.getElementById('start-camera-btn').textContent = 'Load Model & Start';
    document.getElementById('stop-camera-btn').disabled = true;
    document.getElementById('toggle-detection-btn').disabled = true;
    
    document.getElementById('resolution-select').disabled = false;
    document.getElementById('model-select').disabled = false;
    document.getElementById('backend-select').disabled = false;
    
    logToConsole('Camera stopped', 'info');
}

function toggleDetection() {
    if (detectionActive) {
        detectionActive = false;
        document.getElementById('toggle-detection-btn').textContent = 'Start Detection';
        logToConsole('Detection stopped', 'info');
    } else {
        detectionActive = true;
        frameTimes = [];
        inferenceTotal = 0;
        inferenceCount = 0;
        fpsTotal = 0;
        fpsCount = 0;
        document.getElementById('toggle-detection-btn').textContent = 'Stop Detection';
        logToConsole('Detection started', 'success');
        runDetectionLoop();
    }
}

async function runDetectionLoop() {
    if (!detectionActive) return;
    
    await runDetection();
    
    if (detectionActive) {
        requestAnimationFrame(runDetectionLoop);
    }
}

async function runDetection() {
    if (!modelLoaded || !detectionActive) return;
    
    const video = document.getElementById('camera-feed');
    const canvas = document.getElementById('canvas-overlay');
    const ctx = canvas.getContext('2d');
    
    const startTime = performance.now();
    
    try {
        const predictions = await cocoModel.detect(video);
        const inferenceTime = performance.now() - startTime;
        
        inferenceTotal += inferenceTime;
        inferenceCount++;
        const avgInference = inferenceTotal / inferenceCount;
        
        const now = performance.now();
        frameTimes.push(now);
        
        while (frameTimes.length > 30) {
            frameTimes.shift();
        }
        
        let fps = 0;
        if (frameTimes.length >= 2) {
            const timeSpan = frameTimes[frameTimes.length - 1] - frameTimes[0];
            fps = ((frameTimes.length - 1) / timeSpan) * 1000;
            
            if (frameTimes.length >= 10) {
                fpsTotal += fps;
                fpsCount++;
            }
        }
        const avgFps = fpsCount > 0 ? fpsTotal / fpsCount : 0;
        
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        
        ctx.font = '16px Arial';
        ctx.lineWidth = 2;
        
        predictions.forEach(prediction => {
            const [x, y, width, height] = prediction.bbox;
            
            ctx.strokeStyle = '#00ff00';
            ctx.strokeRect(x, y, width, height);
            
            const label = `${prediction.class} (${Math.round(prediction.score * 100)}%)`;
            ctx.fillStyle = 'rgba(0, 255, 0, 0.8)';
            ctx.fillRect(x, y - 25, ctx.measureText(label).width + 10, 25);
            
            ctx.fillStyle = '#000';
            ctx.fillText(label, x + 5, y - 7);
        });
        
        lastDetections = predictions;
        updateDetectionsList(predictions);
        
        if (predictions.length > 0 && inferenceCount % 10 === 0) {
            const topDetection = predictions[0];
            logToConsole(`Detected: ${topDetection.class} (${Math.round(topDetection.score * 100)}%)`, 'success');
        }
        
        document.getElementById('detection-stats').textContent = 
            `FPS: ${fps.toFixed(1)} (avg: ${avgFps.toFixed(1)}) | Inference: ${inferenceTime.toFixed(1)}ms (avg: ${avgInference.toFixed(1)}ms) | Objects: ${predictions.length}`;
        
    } catch (error) {
        console.error('Detection error:', error);
        logToConsole('Detection error: ' + error.message, 'error');
    }
}

function updateDetectionsList(predictions) {
    const list = document.getElementById('detection-list');
    
    if (predictions.length === 0) {
        list.innerHTML = '<div class="text-muted">No objects detected</div>';
        return;
    }
    
    list.innerHTML = predictions.map(p => 
        `<div class="detection-item">
            ${p.class} - ${Math.round(p.score * 100)}% confidence
        </div>`
    ).join('');
}
