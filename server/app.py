"""
CAN-RTOS-Sim Server
==================

A FastAPI-based server that simulates a CAN bus system with fault injection capabilities.
This server provides:
- Simulated CAN frame generation (sensor and controller nodes)
- Real-time WebSocket streaming for dashboard updates
- REST API for metrics and fault injection
- Fault injection simulation (drop, delay, flood, reset)

"""

import asyncio
import json
import os
import subprocess
import threading
import time
import random
import struct
from datetime import datetime
from typing import Dict, List, Optional, Any
from enum import Enum

from fastapi import FastAPI, WebSocket, WebSocketDisconnect, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import HTMLResponse
from pydantic import BaseModel

# Configuration
CAN_INTERFACE = os.getenv("CAN_INTERFACE", "vcan0")
FASTAPI_HOST = os.getenv("FASTAPI_HOST", "0.0.0.0")
FASTAPI_PORT = int(os.getenv("FASTAPI_PORT", "8000"))

# Global state
bus = None
connected_clients = set()
can_frames = []
fault_injection_active = False
fault_injection_type = None
fault_injection_end_time = 0

# Statistics
stats = {
    "frames_received": 0,
    "frames_dropped": 0,
    "error_frames": 0,
    "bus_load_percent": 0.0,
    "frame_rate_hz": 0.0,
    "node_a_status": "unknown",
    "node_b_status": "unknown",
    "node_b_state": "unknown",
    "watchdog_timeouts": 0,
    "recovery_count": 0,
    "last_update": datetime.now().isoformat()
}

class FaultType(str, Enum):
    DROP = "drop"
    DELAY = "delay"
    FLOOD = "flood"
    RESET = "reset"

class FaultInjectionRequest(BaseModel):
    type: FaultType
    durationMs: Optional[int] = 5000
    delayMs: Optional[int] = 100
    rate: Optional[int] = 1000

class CANFrameData(BaseModel):
    id: str
    data: str
    length: int
    timestamp: str
    type: str

# FastAPI app
app = FastAPI(title="CAN-RTOS-Sim Server", version="1.0.0")

# CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

def init_can_bus():
    """
    Initialize simulated CAN bus
    
    Creates a simulated CAN bus interface using in-memory queues instead of 
    actual SocketCAN hardware. This allows the system to run on any platform
    including macOS without requiring kernel modules.
    
    Returns:
        bool: True if initialization successful, False otherwise
    """
    global bus
    try:
        # Simulate CAN bus with a simple message queue
        bus = {
            'interface': CAN_INTERFACE,
            'queue': asyncio.Queue(),
            'connected': True
        }
        print(f"Simulated CAN bus initialized on {CAN_INTERFACE}")
        return True
    except Exception as e:
        print(f"Failed to initialize CAN bus: {e}")
        return False

def create_simulated_frame(frame_id: int, data: bytes, is_rx: bool = True) -> Dict[str, Any]:
    """Create a simulated CAN frame"""
    return {
        "id": f"0x{frame_id:03X}",
        "data": data.hex().upper(),
        "length": len(data),
        "timestamp": datetime.now().isoformat(),
        "type": "rx" if is_rx else "tx"
    }

def should_drop_frame(frame_id: int) -> bool:
    """Determine if frame should be dropped based on fault injection"""
    if not fault_injection_active or fault_injection_type != FaultType.DROP:
        return False
    
    # Drop sensor frames (0x100-0x10F) during drop fault
    return 0x100 <= frame_id <= 0x10F

def inject_delay():
    """Inject delay into frame"""
    if fault_injection_active and fault_injection_type == FaultType.DELAY:
        time.sleep(0.001)  # 1ms delay

def simulate_can_traffic():
    """Simulate CAN traffic from sensor and controller nodes"""
    global stats, can_frames
    
    if not bus:
        return
    
    frame_count = 0
    
    while True:
        try:
            # Simulate sensor frames from Node A
            if not fault_injection_active or fault_injection_type != FaultType.DROP:
                # Temperature sensor (0x100)
                temp_value = 25.0 + random.uniform(-2, 2)
                temp_data = struct.pack('<f', temp_value)  # 4-byte float
                seq_data = frame_count.to_bytes(4, 'little')
                frame_data = temp_data + seq_data
                
                if not should_drop_frame(0x100):
                    inject_delay()
                    frame_dict = create_simulated_frame(0x100, frame_data)
                    can_frames.append(frame_dict)
                    stats["frames_received"] += 1
                    
                    # Store frame for WebSocket broadcasting (handled by stats_updater)
                
                # Pressure sensor (0x101)
                if not should_drop_frame(0x101):
                    inject_delay()
                    pressure_value = 1013.25 + random.uniform(-5, 5)
                    pressure_data = struct.pack('<f', pressure_value)  # 4-byte float
                    frame_data = pressure_data + seq_data
                    frame_dict = create_simulated_frame(0x101, frame_data)
                    can_frames.append(frame_dict)
                    stats["frames_received"] += 1
                    
                    # Frame stored for WebSocket broadcasting
                
                # Humidity sensor (0x102)
                if not should_drop_frame(0x102):
                    inject_delay()
                    humidity_value = 60.0 + random.uniform(-10, 10)
                    humidity_data = struct.pack('<f', humidity_value)  # 4-byte float
                    frame_data = humidity_data + seq_data
                    frame_dict = create_simulated_frame(0x102, frame_data)
                    can_frames.append(frame_dict)
                    stats["frames_received"] += 1
                    
                    # Frame stored for WebSocket broadcasting
                
                # Light sensor (0x103)
                if not should_drop_frame(0x103):
                    inject_delay()
                    light_data = (500 + random.randint(-100, 100)).to_bytes(2, 'little')
                    frame_data = light_data + seq_data[:2] + b'\x00\x00'
                    frame_dict = create_simulated_frame(0x103, frame_data)
                    can_frames.append(frame_dict)
                    stats["frames_received"] += 1
                    
                    # Frame stored for WebSocket broadcasting
                
                # Status frame (0x104)
                if not should_drop_frame(0x104):
                    inject_delay()
                    status_data = b'\x01' + b'\x00' * 3 + seq_data  # OK status
                    frame_dict = create_simulated_frame(0x104, status_data)
                    can_frames.append(frame_dict)
                    stats["frames_received"] += 1
                    
                    # Frame stored for WebSocket broadcasting
                
                # Sequence frame (0x105)
                if not should_drop_frame(0x105):
                    inject_delay()
                    seq_frame_data = seq_data + (int(time.time() * 1000) & 0xFFFFFFFF).to_bytes(4, 'little')
                    frame_dict = create_simulated_frame(0x105, seq_frame_data)
                    can_frames.append(frame_dict)
                    stats["frames_received"] += 1
                    
                    # Frame stored for WebSocket broadcasting
            
            # Simulate controller frames from Node B (less frequent)
            if frame_count % 10 == 0:  # Every 10th frame
                # Temperature threshold command (0x200)
                temp_cmd_value = 25.0 + random.uniform(-2, 2)
                temp_threshold_value = 30.0
                temp_cmd_data = struct.pack('<f', temp_cmd_value) + struct.pack('<f', temp_threshold_value)
                frame_dict = create_simulated_frame(0x200, temp_cmd_data)
                can_frames.append(frame_dict)
                stats["frames_received"] += 1
                
                # Frame stored for WebSocket broadcasting (handled by stats_updater)
            
            # Keep only last 1000 frames
            if len(can_frames) > 1000:
                can_frames = can_frames[-1000:]
            
            frame_count += 1
            time.sleep(0.1)  # 10Hz frame rate
            
        except Exception as e:
            print(f"CAN simulation error: {e}")
            time.sleep(0.1)

async def broadcast_frame(frame: Dict[str, Any]):
    """Broadcast frame to all connected WebSocket clients"""
    global connected_clients
    if connected_clients:
        message = json.dumps({
            "type": "frame",
            "data": frame
        })
        disconnected = set()
        for client in connected_clients:
            try:
                await client.send_text(message)
            except Exception as e:
                print(f"Error broadcasting frame to client: {e}")
                disconnected.add(client)
        
        # Remove disconnected clients
        connected_clients -= disconnected

async def broadcast_stats():
    """Broadcast statistics to WebSocket clients"""
    global connected_clients
    if connected_clients:
        message = json.dumps({
            "type": "stats",
            "data": stats
        })
        disconnected = set()
        for client in connected_clients:
            try:
                await client.send_text(message)
            except Exception as e:
                print(f"Error broadcasting stats to client: {e}")
                disconnected.add(client)
        
        # Remove disconnected clients
        connected_clients -= disconnected

async def stats_updater():
    """Update statistics periodically"""
    global stats, connected_clients
    
    while True:
        try:
            # Calculate frame rate (frames per second)
            current_time = time.time()
            if hasattr(stats_updater, 'last_frame_count'):
                frame_diff = stats["frames_received"] - stats_updater.last_frame_count
                time_diff = current_time - stats_updater.last_time
                if time_diff > 0:
                    stats["frame_rate_hz"] = frame_diff / time_diff
            
            stats_updater.last_frame_count = stats["frames_received"]
            stats_updater.last_time = current_time
            
            # Update bus load (target ~25% for healthy operation)
            base_load = 25.0  # Base healthy load
            fault_multiplier = 1.0
            
            # Increase load during fault injection
            if fault_injection_active:
                if fault_injection_type == FaultType.FLOOD:
                    fault_multiplier = 3.0  # 75% load during flood
                elif fault_injection_type == FaultType.DROP:
                    fault_multiplier = 0.3  # 7.5% load during drop
                elif fault_injection_type == FaultType.DELAY:
                    fault_multiplier = 1.5  # 37.5% load during delay
            
            stats["bus_load_percent"] = min(100.0, base_load * fault_multiplier)
            
            stats["last_update"] = datetime.now().isoformat()
            
            # Broadcast stats and recent frames
            await broadcast_stats()
            
            # Broadcast recent frames to WebSocket clients
            if connected_clients and can_frames:
                recent_frames = can_frames[-10:]  # Last 10 frames
                for frame in recent_frames:
                    await broadcast_frame(frame)
            
            await asyncio.sleep(1)
            
        except Exception as e:
            print(f"Stats updater error: {e}")
            await asyncio.sleep(1)

def inject_fault(fault_request: FaultInjectionRequest):
    """Inject fault into the system"""
    global fault_injection_active, fault_injection_type, fault_injection_end_time, stats
    
    print(f"Injecting fault: {fault_request.type}")
    
    fault_injection_active = True
    fault_injection_type = fault_request.type
    fault_injection_end_time = time.time() + (fault_request.durationMs / 1000.0)
    
    if fault_request.type == FaultType.DROP:
        print(f"Dropping sensor frames for {fault_request.durationMs}ms")
        # Increment watchdog timeouts when sensor drops
        stats["watchdog_timeouts"] += 1
        
    elif fault_request.type == FaultType.DELAY:
        print(f"Adding {fault_request.delayMs}ms delay to frames for {fault_request.durationMs}ms")
        # Increment watchdog timeouts for delays
        stats["watchdog_timeouts"] += 1
        
    elif fault_request.type == FaultType.FLOOD:
        print(f"Flooding bus at {fault_request.rate} frames/sec for {fault_request.durationMs}ms")
        # Start flood thread
        threading.Thread(target=flood_bus, args=(fault_request.rate, fault_request.durationMs), daemon=True).start()
        
    elif fault_request.type == FaultType.RESET:
        print("Resetting node A")
        # Increment recovery count on reset
        stats["recovery_count"] += 1
        reset_node_a()
    
    # Schedule fault end
    threading.Timer(fault_request.durationMs / 1000.0, end_fault_injection).start()

def flood_bus(rate: int, duration_ms: int):
    """Flood the bus with random frames"""
    if not bus:
        return
    
    end_time = time.time() + (duration_ms / 1000.0)
    frame_interval = 1.0 / rate
    
    while time.time() < end_time and fault_injection_active:
        try:
            # Generate random frame
            frame_id = 0x300 + (int(time.time() * 1000) % 100)
            frame_data = bytes([i % 256 for i in range(8)])
            
            frame_dict = create_simulated_frame(frame_id, frame_data)
            can_frames.append(frame_dict)
            stats["frames_received"] += 1
            
            # Frame stored for WebSocket broadcasting
            
            time.sleep(frame_interval)
            
        except Exception as e:
            print(f"Flood error: {e}")
            break

def reset_node_a():
    """Reset node A (simulated)"""
    # In a real implementation, this would restart the node A container
    # For now, we'll just log it
    print("Node A reset requested")

def end_fault_injection():
    """End fault injection"""
    global fault_injection_active, fault_injection_type, stats
    fault_injection_active = False
    fault_injection_type = None
    # Increment recovery count when fault ends (automatic recovery)
    stats["recovery_count"] += 1
    print("Fault injection ended - automatic recovery")

# API Endpoints

@app.get("/")
async def root():
    return {"message": "CAN-RTOS-Sim Server", "version": "1.0.0"}

@app.get("/health")
async def health():
    return {"status": "healthy", "can_interface": CAN_INTERFACE, "bus_connected": bus is not None}

@app.get("/metrics")
async def get_metrics():
    """Get current system metrics"""
    return stats

@app.get("/frames")
async def get_frames(limit: int = 100):
    """Get recent CAN frames"""
    return can_frames[-limit:]

@app.post("/faults/inject")
async def inject_fault_endpoint(fault_request: FaultInjectionRequest):
    """Inject fault into the system"""
    try:
        inject_fault(fault_request)
        return {"message": f"Fault {fault_request.type} injected successfully"}
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/faults/status")
async def get_fault_status():
    """Get current fault injection status"""
    return {
        "active": fault_injection_active,
        "type": fault_injection_type,
        "end_time": fault_injection_end_time
    }

@app.post("/system/reset")
async def reset_system():
    """Reset system metrics and counters"""
    global stats, fault_injection_active, fault_injection_type, fault_injection_end_time
    
    # Reset all counters
    stats["watchdog_timeouts"] = 0
    stats["recovery_count"] = 0
    stats["frames_dropped"] = 0
    stats["error_frames"] = 0
    
    # Clear fault injection state
    fault_injection_active = False
    fault_injection_type = None
    fault_injection_end_time = 0
    
    print("System metrics reset")
    return {"message": "System metrics reset successfully"}

@app.websocket("/stream")
async def websocket_endpoint(websocket: WebSocket):
    """WebSocket endpoint for real-time data streaming"""
    global connected_clients
    await websocket.accept()
    connected_clients.add(websocket)
    print(f"WebSocket client connected. Total clients: {len(connected_clients)}")
    
    try:
        while True:
            # Keep connection alive - wait for messages or ping
            try:
                # Wait for a message with a timeout
                await asyncio.wait_for(websocket.receive_text(), timeout=30.0)
            except asyncio.TimeoutError:
                # Send a ping to keep connection alive (FastAPI WebSocket doesn't have ping method)
                await websocket.send_text("ping")
    except WebSocketDisconnect:
        connected_clients.discard(websocket)
        print(f"WebSocket client disconnected. Total clients: {len(connected_clients)}")
    except Exception as e:
        print(f"WebSocket error: {e}")
        connected_clients.discard(websocket)
        print(f"WebSocket client disconnected due to error. Total clients: {len(connected_clients)}")

@app.on_event("startup")
async def startup_event():
    """Initialize the application"""
    print("Starting CAN-RTOS-Sim Server...")
    
    # Initialize simulated CAN bus
    if init_can_bus():
        # Start CAN traffic simulation thread
        threading.Thread(target=simulate_can_traffic, daemon=True).start()
        
        # Start stats updater
        asyncio.create_task(stats_updater())
        
        print("Server started successfully")
    else:
        print("Failed to initialize CAN bus")

@app.on_event("shutdown")
async def shutdown_event():
    """Cleanup on shutdown"""
    global bus
    if bus:
        bus['connected'] = False
    print("Server shutdown")

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host=FASTAPI_HOST, port=FASTAPI_PORT)
