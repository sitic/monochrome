#!/bin/sh

if command -v curl &> /dev/null; then
    echo "Using curl to install uv..."
    curl -LsSf https://astral.sh/uv/install.sh | sh
elif command -v wget &> /dev/null; then
    echo "Using wget to install uv..."
    wget -qO- https://astral.sh/uv/install.sh | sh
else
    echo "Error: Neither curl nor wget found. Please install uv manually from https://docs.astral.sh/uv/"
    exit 1
fi
