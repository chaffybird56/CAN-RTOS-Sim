/**
 * FaultInjection Component
 * =======================
 * 
 * Provides fault injection controls for testing system resilience.
 * Allows injection of various fault types to simulate real-world failures.
 * 
 * Fault Types:
 * - Drop Sensor: Suppress sensor frames to trigger watchdog timeouts
 * - Delay Frames: Add latency to simulate network jitter
 * - Flood Bus: Generate high frame rate traffic to test overload handling
 * - Reset Node A: Restart sensor node to test recovery mechanisms
 */

import { useState } from 'react'
import { FaultInjectionRequest } from '../types'

const FaultInjection: React.FC = () => {
  const [loading, setLoading] = useState<string | null>(null)
  const [faultStatus, setFaultStatus] = useState<{ active: boolean; type?: string }>({ active: false })

  const injectFault = async (faultRequest: FaultInjectionRequest) => {
    setLoading(faultRequest.type)
    
    try {
      const apiUrl = process.env.NEXT_PUBLIC_API_URL || 'http://localhost:8000'
      
      // Handle system reset differently
      if (faultRequest.type === 'system_reset') {
        const response = await fetch(`${apiUrl}/system/reset`, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
        })
        
        if (response.ok) {
          console.log('System reset successfully')
          setFaultStatus({ active: false })
          // Refresh the page to update all metrics
          window.location.reload()
        } else {
          console.error('Failed to reset system')
        }
        return
      }
      
      // Handle regular fault injection
      const response = await fetch(`${apiUrl}/faults/inject`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(faultRequest),
      })
      
      if (response.ok) {
        setFaultStatus({ active: true, type: faultRequest.type })
        console.log(`Fault ${faultRequest.type} injected successfully`)
        
        // Reset status after duration
        if (faultRequest.durationMs) {
          setTimeout(() => {
            setFaultStatus({ active: false })
          }, faultRequest.durationMs)
        }
      } else {
        console.error('Failed to inject fault')
      }
    } catch (error) {
      console.error('Error injecting fault:', error)
    } finally {
      setLoading(null)
    }
  }

  const faultButtons = [
    {
      type: 'drop' as const,
      label: 'Drop Sensor',
      description: 'Suppress sensor frames (0x100-0x10F)',
      durationMs: 5000,
      color: 'bg-red-600 hover:bg-red-700'
    },
    {
      type: 'delay' as const,
      label: 'Delay Frames',
      description: 'Add latency to frame transmission',
      durationMs: 3000,
      delayMs: 100,
      color: 'bg-yellow-600 hover:bg-yellow-700'
    },
    {
      type: 'flood' as const,
      label: 'Flood Bus',
      description: 'Generate high frame rate traffic',
      durationMs: 3000,
      rate: 1000,
      color: 'bg-orange-600 hover:bg-orange-700'
    },
    {
      type: 'reset' as const,
      label: 'Reset Node A',
      description: 'Restart sensor node',
      color: 'bg-purple-600 hover:bg-purple-700'
    },
    {
      type: 'system_reset' as const,
      label: 'Reset System',
      description: 'Clear all counters and metrics',
      color: 'bg-gray-600 hover:bg-gray-700'
    }
  ]

  return (
    <div className="bg-white rounded-lg shadow p-6">
      <h3 className="text-lg font-medium text-gray-900 mb-4">Fault Injection</h3>
      
      {faultStatus.active && (
        <div className="mb-4 p-3 bg-red-50 border border-red-200 rounded-md">
          <div className="flex items-center">
            <div className="flex-shrink-0">
              <div className="w-2 h-2 bg-red-500 rounded-full animate-pulse"></div>
            </div>
            <div className="ml-3">
              <p className="text-sm text-red-800">
                Fault injection active: <span className="font-medium">{faultStatus.type}</span>
              </p>
            </div>
          </div>
        </div>
      )}

          <div className="grid grid-cols-1 gap-3">
            {faultButtons.map((fault) => (
              <button
                key={fault.type}
                onClick={() => injectFault({
                  type: fault.type,
                  durationMs: fault.durationMs,
                  delayMs: fault.delayMs,
                  rate: fault.rate
                })}
                disabled={loading === fault.type}
                className={`${fault.color} text-white px-4 py-2 rounded-md text-sm font-medium transition-colors duration-200 disabled:opacity-50 disabled:cursor-not-allowed`}
              >
                <div className="text-left">
                  <div className="font-medium">{fault.label}</div>
                  <div className="text-xs opacity-90">{fault.description}</div>
                  {loading === fault.type && (
                    <div className="text-xs mt-1">Injecting...</div>
                  )}
                </div>
              </button>
            ))}
            
        {/* FORCE RESET BUTTON - Always visible */}
        <button
          onClick={() => {
            console.log('FORCE RESET CLICKED');
            injectFault({ type: 'system_reset' });
          }}
          disabled={loading === 'system_reset'}
          className="bg-red-600 hover:bg-red-700 text-white px-4 py-2 rounded-md text-sm font-medium transition-colors duration-200 disabled:opacity-50 disabled:cursor-not-allowed"
        >
          <div className="text-left">
            <div className="font-medium">🚨 FORCE RESET SYSTEM 🚨</div>
            <div className="text-xs opacity-90">Clear all counters and metrics</div>
            {loading === 'system_reset' && (
              <div className="text-xs mt-1">Resetting...</div>
            )}
          </div>
        </button>
          </div>

      <div className="mt-4 p-3 bg-gray-50 rounded-md">
        <h4 className="text-sm font-medium text-gray-900 mb-2">Quick Demo Script</h4>
        <div className="text-xs text-gray-600 space-y-1">
          <div>1. Click "Drop Sensor" → Watch Node B go DEGRADED</div>
          <div>2. Wait for recovery → Node B returns to NORMAL</div>
          <div>3. Click "Flood Bus" → Observe error counters</div>
          <div>4. Click "Delay Frames" → See timestamp spread</div>
        </div>
      </div>
    </div>
  )
}

export default FaultInjection
