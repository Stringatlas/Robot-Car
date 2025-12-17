// Service Worker for caching TensorFlow.js and COCO-SSD models
const CACHE_NAME = 'ai-phone-cache-v1';

// Resources to cache
const CACHE_URLS = [
    // TensorFlow.js core and backends
    'https://cdn.jsdelivr.net/npm/@tensorflow/tfjs@4.17.0/dist/tf.min.js',
    'https://cdn.jsdelivr.net/npm/@tensorflow/tfjs-backend-webgpu@4.17.0/dist/tf-backend-webgpu.min.js',
    'https://cdn.jsdelivr.net/npm/@tensorflow/tfjs-backend-wasm@4.17.0/dist/tf-backend-wasm.min.js',
    
    // WASM binaries
    'https://cdn.jsdelivr.net/npm/@tensorflow/tfjs-backend-wasm@4.17.0/dist/tfjs-backend-wasm.wasm',
    'https://cdn.jsdelivr.net/npm/@tensorflow/tfjs-backend-wasm@4.17.0/dist/tfjs-backend-wasm-simd.wasm',
    'https://cdn.jsdelivr.net/npm/@tensorflow/tfjs-backend-wasm@4.17.0/dist/tfjs-backend-wasm-threaded-simd.wasm',
    
    // COCO-SSD model
    'https://cdn.jsdelivr.net/npm/@tensorflow-models/coco-ssd@2.2.3/dist/coco-ssd.min.js',
    
    // Local files
    './phone-ai.html',
    './websockets.js'
];

// Install event - cache resources
self.addEventListener('install', (event) => {
    console.log('[SW] Installing service worker...');
    event.waitUntil(
        caches.open(CACHE_NAME)
            .then((cache) => {
                console.log('[SW] Caching app resources...');
                return cache.addAll(CACHE_URLS);
            })
            .then(() => {
                console.log('[SW] All resources cached');
                return self.skipWaiting();
            })
            .catch((error) => {
                console.error('[SW] Cache failed:', error);
            })
    );
});

// Activate event - clean up old caches
self.addEventListener('activate', (event) => {
    console.log('[SW] Activating service worker...');
    event.waitUntil(
        caches.keys().then((cacheNames) => {
            return Promise.all(
                cacheNames
                    .filter((name) => name !== CACHE_NAME)
                    .map((name) => {
                        console.log('[SW] Deleting old cache:', name);
                        return caches.delete(name);
                    })
            );
        }).then(() => {
            return self.clients.claim();
        })
    );
});

// Fetch event - serve from cache, fallback to network
self.addEventListener('fetch', (event) => {
    // Only handle GET requests
    if (event.request.method !== 'GET') {
        return;
    }

    event.respondWith(
        caches.match(event.request)
            .then((cachedResponse) => {
                if (cachedResponse) {
                    console.log('[SW] Serving from cache:', event.request.url);
                    return cachedResponse;
                }

                // Not in cache - fetch from network
                return fetch(event.request)
                    .then((networkResponse) => {
                        // Cache successful responses for future use
                        if (networkResponse && networkResponse.status === 200) {
                            const responseClone = networkResponse.clone();
                            caches.open(CACHE_NAME)
                                .then((cache) => {
                                    // Cache model weight files and other resources dynamically
                                    const url = event.request.url;
                                    if (url.includes('tfhub.dev') || 
                                        url.includes('storage.googleapis.com') ||
                                        url.includes('jsdelivr.net') ||
                                        url.endsWith('.bin') ||
                                        url.endsWith('.json')) {
                                        console.log('[SW] Caching dynamically:', url);
                                        cache.put(event.request, responseClone);
                                    }
                                });
                        }
                        return networkResponse;
                    })
                    .catch((error) => {
                        console.error('[SW] Fetch failed:', error);
                        // Could return a fallback response here
                    });
            })
    );
});

// Message handler for cache management
self.addEventListener('message', (event) => {
    if (event.data && event.data.type === 'CLEAR_CACHE') {
        caches.delete(CACHE_NAME).then(() => {
            console.log('[SW] Cache cleared');
            event.ports[0].postMessage({ success: true });
        });
    }
    
    if (event.data && event.data.type === 'GET_CACHE_SIZE') {
        caches.open(CACHE_NAME).then((cache) => {
            cache.keys().then((keys) => {
                event.ports[0].postMessage({ count: keys.length, keys: keys.map(k => k.url) });
            });
        });
    }
});
