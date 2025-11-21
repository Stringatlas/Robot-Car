#!/bin/bash

# Sync Files Script
# Copies web files from data/ to test/web_content folder

SOURCE_DIR="../data"
DEST_DIR="./web_content"

echo "🔄 Syncing files from data/ to test/web_content/..."
echo ""

# Create web_content directory
mkdir -p "$DEST_DIR"

# Copy HTML files
echo "📄 Copying HTML files..."
cp "$SOURCE_DIR/index.html" "$DEST_DIR/"
cp "$SOURCE_DIR/calibration.html" "$DEST_DIR/"

# Copy CSS files
echo "🎨 Copying CSS files..."
cp "$SOURCE_DIR/common.css" "$DEST_DIR/"

# Copy JavaScript files
echo "📜 Copying JavaScript files..."
cp "$SOURCE_DIR/script.js" "$DEST_DIR/"
cp "$SOURCE_DIR/calibration.js" "$DEST_DIR/"

# Copy components folder
echo "🧩 Copying components..."
mkdir -p "$DEST_DIR/components"
cp -r "$SOURCE_DIR/components/"* "$DEST_DIR/components/"

# Modify HTML files to include mock-websocket.js
echo "🔧 Patching HTML files to enable mock WebSocket..."

# For index.html - uncomment the mock-websocket.js line
sed -i '' 's|<!-- <script src="/mock-websocket.js"></script> -->|<script src="/mock-websocket.js"></script>|g' "$DEST_DIR/index.html"
echo "  ✓ Enabled mock WebSocket in index.html"

# For calibration.html - uncomment the mock-websocket.js line
sed -i '' 's|<!-- <script src="/mock-websocket.js"></script> -->|<script src="/mock-websocket.js"></script>|g' "$DEST_DIR/calibration.html"
echo "  ✓ Enabled mock WebSocket in calibration.html"

echo ""
echo "✅ Sync complete! Test environment is ready."
echo ""
echo "📁 Files copied to: test/web_content/"
echo "🚀 Run './start-test-server.sh' to start testing."
