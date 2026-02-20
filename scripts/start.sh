#!/bin/bash
# Start the Contractor Quotes & Sourcing application
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PORT="${QUOTES_HTTP_PORT:-8080}"

echo "Starting Contractor Quotes on port ${PORT}..."
exec "${SCRIPT_DIR}/contractor_quotes" \
    --docroot "${SCRIPT_DIR}/resources" \
    --http-address 0.0.0.0 \
    --http-port "${PORT}"
