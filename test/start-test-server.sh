#!/bin/bash

# Start Test Server Script
# Serves the web interface on http://localhost:8000

cd web_content

echo "🚀 Starting Robot Car Web Interface Test Server..."
echo "📂 Serving from: $(pwd)"
echo ""
echo "Access the interface at:"
echo "  • Main Control:   http://localhost:8000/index.html"
echo "  • Calibration:    http://localhost:8000/calibration.html"
echo ""
echo "Press Ctrl+C to stop the server"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

python3 -m http.server 8000
