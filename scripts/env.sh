#!/bin/bash
# Environment variables for Contractor Quotes
export QUOTES_HTTP_PORT="${QUOTES_HTTP_PORT:-8080}"
export WT_APP_ROOT="$(cd "$(dirname "$0")" && pwd)"
