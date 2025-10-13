import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, AreaChart, Area } from 'recharts'
import { CANFrame, Metrics } from '../types'
import { useMemo } from 'react'

interface ChartsProps {
  frames: CANFrame[]
  metrics: Metrics | null
}

const Charts: React.FC<ChartsProps> = ({ frames, metrics }) => {
  const chartData = useMemo(() => {
    // Group frames by 5-second intervals to reduce fluctuation
    const frameCounts: { [key: string]: number } = {}
    const errorCounts: { [key: string]: number } = {}
    
    frames.forEach(frame => {
      const timestamp = new Date(frame.timestamp)
      const interval = Math.floor(timestamp.getTime() / 5000) * 5000 // 5-second intervals
      const key = new Date(interval).toLocaleTimeString('en-US', { 
        hour12: false, 
        minute: '2-digit', 
        second: '2-digit' 
      })
      
      frameCounts[key] = (frameCounts[key] || 0) + 1
      
      // Count error frames (status != 0x01 or frames with errors)
      if (frame.id === '0x104' && frame.data !== '01000000') {
        errorCounts[key] = (errorCounts[key] || 0) + 1
      }
    })
    
    // Convert to array format for charts and smooth the data
    const rawData = Object.keys(frameCounts).slice(-12).map(key => ({
      time: key,
      frames: frameCounts[key] || 0,
      errors: errorCounts[key] || 0
    }))
    
    // Apply simple moving average to smooth frame rate
    const smoothedData = rawData.map((item, index) => {
      if (index < 2) return item // Keep first 2 points as-is
      
      const prev1 = rawData[index - 1]?.frames || 0
      const prev2 = rawData[index - 2]?.frames || 0
      const smoothed = (item.frames + prev1 + prev2) / 3
      
      return {
        ...item,
        frames: Math.round(smoothed * 10) / 10 // Round to 1 decimal
      }
    })
    
    return smoothedData
  }, [frames])

  const busLoadData = useMemo(() => {
    // Create bus load data based on current metrics
    const data = []
    const now = new Date()
    
    for (let i = 19; i >= 0; i--) {
      const time = new Date(now.getTime() - i * 1000)
      const key = time.toLocaleTimeString('en-US', { 
        hour12: false, 
        minute: '2-digit', 
        second: '2-digit' 
      })
      
      data.push({
        time: key,
        load: metrics?.bus_load_percent || 0
      })
    }
    
    return data
  }, [metrics])

  return (
    <div className="bg-white rounded-lg shadow p-6">
      <h3 className="text-lg font-medium text-gray-900 mb-6">System Charts</h3>
      
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* Frame Rate Chart */}
        <div>
          <h4 className="text-sm font-medium text-gray-700 mb-3">Frame Rate (Last 60 seconds, 5s intervals)</h4>
          <div className="h-64">
            <ResponsiveContainer width="100%" height="100%">
              <AreaChart data={chartData}>
                <CartesianGrid strokeDasharray="3 3" />
                <XAxis dataKey="time" />
                <YAxis />
                <Tooltip 
                  formatter={(value: any, name: string) => [
                    `${value} frames/sec`, 
                    name === 'frames' ? 'Frame Rate' : 'Error Rate'
                  ]}
                />
                <Area 
                  type="monotone" 
                  dataKey="frames" 
                  stroke="#0ea5e9" 
                  fill="#0ea5e9" 
                  fillOpacity={0.3}
                />
                <Line 
                  type="monotone" 
                  dataKey="errors" 
                  stroke="#ef4444" 
                  strokeWidth={2}
                  dot={false}
                />
              </AreaChart>
            </ResponsiveContainer>
          </div>
        </div>

        {/* Bus Load Chart */}
        <div>
          <h4 className="text-sm font-medium text-gray-700 mb-3">Bus Load %</h4>
          <div className="h-64">
            <ResponsiveContainer width="100%" height="100%">
              <AreaChart data={busLoadData}>
                <CartesianGrid strokeDasharray="3 3" />
                <XAxis dataKey="time" />
                <YAxis domain={[0, 100]} />
                <Tooltip 
                  formatter={(value: any) => [`${value.toFixed(1)}%`, 'Bus Load']}
                />
                <Area 
                  type="monotone" 
                  dataKey="load" 
                  stroke="#10b981" 
                  fill="#10b981"
                  fillOpacity={0.3}
                  strokeWidth={2}
                />
              </AreaChart>
            </ResponsiveContainer>
          </div>
        </div>
      </div>

      {/* Frame Type Distribution */}
      <div className="mt-6">
        <h4 className="text-sm font-medium text-gray-700 mb-3">Frame Type Distribution (Last 100 frames)</h4>
        <div className="flex flex-wrap gap-2">
          {(() => {
            const recentFrames = frames.slice(-100)
            const distribution = {
              sensor: 0,
              command: 0,
              sequence: 0,
              other: 0
            }
            
            recentFrames.forEach(frame => {
              const numId = parseInt(frame.id, 16)
              if (numId >= 0x100 && numId <= 0x10F) distribution.sensor++
              else if (numId >= 0x200 && numId <= 0x2FF) distribution.command++
              else if (numId === 0x105) distribution.sequence++
              else distribution.other++
            })
            
            return Object.entries(distribution).map(([type, count]) => (
              <div key={type} className="flex items-center space-x-2">
                <span className={`inline-block w-3 h-3 rounded ${
                  type === 'sensor' ? 'bg-can-500' :
                  type === 'command' ? 'bg-purple-500' :
                  type === 'sequence' ? 'bg-gray-500' :
                  'bg-gray-300'
                }`}></span>
                <span className="text-sm text-gray-600 capitalize">{type}: {count}</span>
              </div>
            ))
          })()}
        </div>
      </div>
    </div>
  )
}

export default Charts
