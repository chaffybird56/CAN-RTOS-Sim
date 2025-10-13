# Multi-stage build for C++ application
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    can-utils \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY app/ ./app/

# Build the application
WORKDIR /app/app
RUN g++ -std=c++17 -pthread -O2 -o controller_node main.cpp can_interface.cpp controller_tasks.cpp

# Runtime stage
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    can-utils \
    && rm -rf /var/lib/apt/lists/*

# Copy built application
COPY --from=builder /app/app/controller_node /usr/local/bin/controller_node

# Set working directory
WORKDIR /app

# Copy any additional runtime files
COPY --from=builder /app/app/*.h /app/

# Run the application
CMD ["controller_node"]

