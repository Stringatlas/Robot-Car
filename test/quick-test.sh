#!/bin/bash

# Quick test script - Sets up and starts testing environment in one command

echo "🧪 Robot Car Web Interface - Quick Test Setup"
echo ""

# Check if we're in the test directory
if [ ! -f "sync-files.sh" ]; then
    echo "❌ Error: Must run from test/ directory"
    exit 1
fi

# Sync files
echo "1️⃣  Syncing files from data/..."
./sync-files.sh
echo ""

# Start server
echo "2️⃣  Starting test server..."
echo ""
./start-test-server.sh
