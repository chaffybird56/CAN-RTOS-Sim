export interface CANFrame {
  id: string
  data: string
  length: number
  timestamp: string
  type: 'rx' | 'tx'
}

export interface Metrics {
  frames_received: number
  frames_dropped: number
  error_frames: number
  bus_load_percent: number
  frame_rate_hz: number
  node_a_status: string
  node_b_status: string
  node_b_state: string
  watchdog_timeouts: number
  recovery_count: number
  last_update: string
}

export interface FaultInjectionRequest {
  type: 'drop' | 'delay' | 'flood' | 'reset' | 'system_reset'
  durationMs?: number
  delayMs?: number
  rate?: number
}

export interface WebSocketMessage {
  type: 'frame' | 'stats' | 'error'
  data: any
}
