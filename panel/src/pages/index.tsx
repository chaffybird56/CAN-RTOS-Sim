/**
 * CAN-RTOS-Sim Dashboard
 * =====================
 * 
 * Main dashboard component for the CAN-RTOS-Sim project.
 * Provides real-time monitoring of:
 * - CAN bus traffic and frame analysis
 * - Node health status (sensor and controller nodes)
 * - Fault injection controls
 * - System performance charts
 * 
 * Features:
 * - WebSocket-based real-time updates
 * - Polling fallback for reliability
 * - Fault injection testing
 * - Live CAN frame monitoring
 * 
 */

import React, { useState, useEffect } from 'react'
import Head from 'next/head'
import BusMonitor from '../components/BusMonitor'
import NodeHealth from '../components/NodeHealth'
import FaultInjection from '../components/FaultInjection'
import Charts from '../components/Charts'
import { Metrics, CANFrame } from '../types'

export default function Home() {
  const [metrics, setMetrics] = useState<Metrics | null>(null)
  const [frames, setFrames] = useState<CANFrame[]>([])
  const [connected, setConnected] = useState(false)
  const [ws, setWs] = useState<WebSocket | null>(null)

  // Fetch data from API
  const fetchData = async () => {
    try {
      const apiUrl = 'http://localhost:8000' // Hardcoded for testing
      console.log('Fetching data from:', apiUrl)
      
      // Fetch metrics
      const metricsResponse = await fetch(`${apiUrl}/metrics`)
      if (metricsResponse.ok) {
        const metricsData = await metricsResponse.json()
        console.log('Metrics data received:', metricsData)
        console.log('Setting metrics state...')
        setMetrics(metricsData)
        console.log('Metrics state set successfully')
      } else {
        console.error('Failed to fetch metrics:', metricsResponse.status)
      }
      
      // Fetch recent frames
      const framesResponse = await fetch(`${apiUrl}/frames?limit=50`)
      if (framesResponse.ok) {
        const framesData = await framesResponse.json()
        console.log('Frames data received:', framesData.length, 'frames')
        setFrames(prev => {
          // Merge with existing frames, avoiding duplicates
          const existingIds = new Set(prev.map(f => f.timestamp))
          const newFrames = framesData.filter((f: any) => !existingIds.has(f.timestamp))
          const updated = [...prev, ...newFrames].slice(-999)
          console.log('Updated frames count:', updated.length)
          return updated
        })
      } else {
        console.error('Failed to fetch frames:', framesResponse.status)
      }
    } catch (error) {
      console.error('Error fetching data:', error)
    }
  }

  useEffect(() => {
    // Initialize WebSocket connection
    const apiUrl = 'http://localhost:8000' // Hardcoded for testing
    const wsUrl = apiUrl.replace('http', 'ws')
    
    const websocket = new WebSocket(`${wsUrl}/stream`)
    
    websocket.onopen = () => {
      console.log('WebSocket connected to:', wsUrl)
      setConnected(true)
    }
    
    websocket.onmessage = (event) => {
      try {
        // Skip ping messages
        if (event.data === 'ping') {
          return
        }
        
        const data = JSON.parse(event.data)
        console.log('WebSocket message received:', data.type, data)
        
        if (data.type === 'frame') {
          setFrames(prev => [...prev.slice(-999), data.data])
        } else if (data.type === 'stats') {
          setMetrics(data.data)
        }
      } catch (error) {
        console.error('Error parsing WebSocket message:', error)
      }
    }
    
    websocket.onclose = () => {
      console.log('WebSocket disconnected')
      setConnected(false)
    }
    
    websocket.onerror = (error) => {
      console.error('WebSocket error:', error)
    }
    
    setWs(websocket)
    
    // Initial data fetch
    fetchData()
    
    // Set up polling as fallback (every 2 seconds)
    const pollInterval = setInterval(fetchData, 2000)
    
    return () => {
      websocket.close()
      clearInterval(pollInterval)
    }
  }, [])

  return (
    <div className="min-h-screen bg-gray-50">
      <Head>
        <title>CAN-RTOS-Sim Dashboard</title>
        <meta name="description" content="CAN Bus Real-time Monitoring Dashboard" />
        <link rel="icon" href="/favicon.ico" />
      </Head>

      <header className="bg-white shadow-sm border-b">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex justify-between items-center h-16">
            <div className="flex items-center">
              <h1 className="text-2xl font-bold text-gray-900">CAN-RTOS-Sim</h1>
              <span className="ml-4 px-2 py-1 text-xs font-medium rounded-full bg-can-100 text-can-800">
                Dashboard
              </span>
            </div>
            <div className="flex items-center space-x-4">
              <div className={`flex items-center ${connected ? 'text-green-600' : 'text-red-600'}`}>
                <div className={`w-2 h-2 rounded-full mr-2 ${connected ? 'bg-green-500' : 'bg-red-500'}`}></div>
                <span className="text-sm font-medium">
                  {connected ? 'Connected' : 'Disconnected'}
                </span>
              </div>
            </div>
          </div>
        </div>
      </header>

      <main className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-6 mb-8">
          {/* Node Health */}
          <div className="lg:col-span-1">
            <NodeHealth metrics={metrics} />
          </div>
          
          {/* Fault Injection */}
          <div className="lg:col-span-1">
            <FaultInjection />
          </div>
        </div>

        {/* Charts */}
        <div className="mb-8">
          <Charts frames={frames} metrics={metrics} />
        </div>

        {/* Bus Monitor */}
        <div>
          <BusMonitor frames={frames} />
        </div>
      </main>
    </div>
  )
}
