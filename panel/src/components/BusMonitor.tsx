import { useState } from 'react'
import { CANFrame } from '../types'

interface BusMonitorProps {
  frames: CANFrame[]
}

const BusMonitor: React.FC<BusMonitorProps> = ({ frames }) => {
  const [filter, setFilter] = useState('all')
  const [showHex, setShowHex] = useState(true)

  const getFrameType = (id: string) => {
    const numId = parseInt(id, 16)
    if (numId >= 0x100 && numId <= 0x10F) return 'sensor'
    if (numId >= 0x200 && numId <= 0x2FF) return 'command'
    if (numId === 0x105) return 'sequence'
    return 'other'
  }

  const getFrameTypeColor = (type: string) => {
    switch (type) {
      case 'sensor': return 'frame-sensor'
      case 'command': return 'frame-command'
      case 'sequence': return 'frame-sequence'
      default: return 'bg-white'
    }
  }

  const filteredFrames = frames.filter(frame => {
    if (filter === 'all') return true
    return getFrameType(frame.id) === filter
  })

  const formatTimestamp = (timestamp: string) => {
    const date = new Date(timestamp)
    return date.toLocaleTimeString('en-US', { 
      hour12: false,
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit'
    })
  }

  const formatData = (data: string, length: number) => {
    if (showHex) {
      return data.slice(0, length * 2).toUpperCase()
    } else {
      // Convert hex to decimal bytes
      const bytes = []
      for (let i = 0; i < length * 2; i += 2) {
        bytes.push(parseInt(data.slice(i, i + 2), 16))
      }
      return bytes.join(' ')
    }
  }

  return (
    <div className="bg-white rounded-lg shadow">
      <div className="px-6 py-4 border-b border-gray-200">
        <div className="flex justify-between items-center">
          <h3 className="text-lg font-medium text-gray-900">Bus Monitor</h3>
          <div className="flex items-center space-x-4">
            <select
              value={filter}
              onChange={(e) => setFilter(e.target.value)}
              className="text-sm border border-gray-300 rounded-md px-3 py-1"
            >
              <option value="all">All Frames</option>
              <option value="sensor">Sensor (0x100-0x10F)</option>
              <option value="command">Command (0x200-0x2FF)</option>
              <option value="sequence">Sequence (0x105)</option>
            </select>
            <label className="flex items-center text-sm">
              <input
                type="checkbox"
                checked={showHex}
                onChange={(e) => setShowHex(e.target.checked)}
                className="mr-2"
              />
              Show Hex
            </label>
          </div>
        </div>
      </div>
      
      <div className="overflow-x-auto">
        <table className="min-w-full divide-y divide-gray-200">
          <thead className="bg-gray-50">
            <tr>
              <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                Time
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                ID
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                DLC
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                Data
              </th>
              <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                Type
              </th>
            </tr>
          </thead>
          <tbody className="bg-white divide-y divide-gray-200">
            {filteredFrames.slice(-50).reverse().map((frame, index) => {
              const frameType = getFrameType(frame.id)
              return (
                <tr key={index} className={getFrameTypeColor(frameType)}>
                  <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-900 can-frame">
                    {formatTimestamp(frame.timestamp)}
                  </td>
                  <td className="px-6 py-4 whitespace-nowrap text-sm font-medium text-gray-900 can-frame">
                    {frame.id}
                  </td>
                  <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-900">
                    {frame.length}
                  </td>
                  <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-900 can-frame">
                    {formatData(frame.data, frame.length)}
                  </td>
                  <td className="px-6 py-4 whitespace-nowrap">
                    <span className={`inline-flex px-2 py-1 text-xs font-medium rounded-full ${
                      frameType === 'sensor' ? 'bg-can-100 text-can-800' :
                      frameType === 'command' ? 'bg-purple-100 text-purple-800' :
                      frameType === 'sequence' ? 'bg-gray-100 text-gray-800' :
                      'bg-gray-100 text-gray-800'
                    }`}>
                      {frameType}
                    </span>
                  </td>
                </tr>
              )
            })}
          </tbody>
        </table>
      </div>
      
      {filteredFrames.length === 0 && (
        <div className="text-center py-8 text-gray-500">
          No frames received yet
        </div>
      )}
    </div>
  )
}

export default BusMonitor

