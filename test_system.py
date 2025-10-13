#!/usr/bin/env python3
"""
CAN-RTOS-Sim System Test
Simple integration test to verify the system is working correctly
"""

import requests
import time
import sys
import json
from typing import Dict, Any

API_BASE = "http://localhost:8000"

def test_api_health() -> bool:
    """Test if the API is responding"""
    try:
        response = requests.get(f"{API_BASE}/health", timeout=5)
        return response.status_code == 200
    except requests.exceptions.RequestException:
        return False

def test_metrics_endpoint() -> bool:
    """Test if metrics endpoint returns valid data"""
    try:
        response = requests.get(f"{API_BASE}/metrics", timeout=5)
        if response.status_code != 200:
            return False
        
        metrics = response.json()
        required_fields = [
            'frames_received', 'frames_dropped', 'error_frames',
            'bus_load_percent', 'frame_rate_hz', 'last_update'
        ]
        
        return all(field in metrics for field in required_fields)
    except (requests.exceptions.RequestException, json.JSONDecodeError):
        return False

def test_fault_injection() -> bool:
    """Test fault injection endpoint"""
    try:
        fault_data = {
            "type": "drop",
            "durationMs": 1000
        }
        
        response = requests.post(
            f"{API_BASE}/faults/inject",
            json=fault_data,
            timeout=5
        )
        
        return response.status_code == 200
    except requests.exceptions.RequestException:
        return False

def test_frames_endpoint() -> bool:
    """Test frames endpoint"""
    try:
        response = requests.get(f"{API_BASE}/frames?limit=10", timeout=5)
        return response.status_code == 200
    except requests.exceptions.RequestException:
        return False

def wait_for_frames(timeout: int = 30) -> bool:
    """Wait for CAN frames to appear"""
    print("Waiting for CAN frames...")
    
    for i in range(timeout):
        try:
            response = requests.get(f"{API_BASE}/metrics", timeout=5)
            if response.status_code == 200:
                metrics = response.json()
                if metrics.get('frames_received', 0) > 0:
                    print(f"✓ Received {metrics['frames_received']} frames")
                    return True
            
            time.sleep(1)
        except requests.exceptions.RequestException:
            pass
    
    print("✗ No frames received within timeout")
    return False

def main():
    """Run all tests"""
    print("CAN-RTOS-Sim System Test")
    print("=" * 30)
    
    tests = [
        ("API Health", test_api_health),
        ("Metrics Endpoint", test_metrics_endpoint),
        ("Frames Endpoint", test_frames_endpoint),
        ("Fault Injection", test_fault_injection),
    ]
    
    passed = 0
    total = len(tests)
    
    for test_name, test_func in tests:
        print(f"\nTesting {test_name}...")
        if test_func():
            print(f"✓ {test_name} passed")
            passed += 1
        else:
            print(f"✗ {test_name} failed")
    
    # Wait for frames (this is the most important test)
    print(f"\nWaiting for CAN traffic...")
    if wait_for_frames():
        passed += 1
    total += 1
    
    print("\n" + "=" * 30)
    print(f"Test Results: {passed}/{total} passed")
    
    if passed == total:
        print("🎉 All tests passed! System is working correctly.")
        return 0
    else:
        print("❌ Some tests failed. Check the system logs.")
        return 1

if __name__ == "__main__":
    sys.exit(main())

