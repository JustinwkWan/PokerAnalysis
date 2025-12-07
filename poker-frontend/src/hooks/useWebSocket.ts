import { useState, useEffect, useRef, useCallback } from 'react';

type MessageEntry = {
  time: string;
  message: string;
  type: string;
};

export const useWebSocket = (url: string) => {
  const [isConnected, setIsConnected] = useState(false);
  const [messages, setMessages] = useState<MessageEntry[]>([]);
  const wsRef = useRef<WebSocket | null>(null);
  const reconnectTimeoutRef = useRef<number | null>(null);

  // Convert http/https to ws/wss
  const wsUrl = url.startsWith('ws') ? url : url.replace(/^https?:\/\//, (match) => match.includes('https') ? 'wss://' : 'ws://');

  const log = useCallback((message: string, type = 'info') => {
    const entry: MessageEntry = {
      time: new Date().toLocaleTimeString(),
      message,
      type
    };
    setMessages(prev => [...prev, entry]);
  }, []);

  const connect = useCallback(() => {
    log(`Connecting to ${wsUrl}...`);
    
    try {
      wsRef.current = new WebSocket(wsUrl);
      
      wsRef.current.onopen = () => {
        log('Connected to server!', 'success');
        setIsConnected(true);
      };
      
      wsRef.current.onclose = () => {
        log('Disconnected from server', 'error');
        setIsConnected(false);
        
        // Auto-reconnect after 2 seconds
        reconnectTimeoutRef.current = window.setTimeout(() => {
          log('Attempting to reconnect...', 'info');
          connect();
        }, 2000);
      };
      
      wsRef.current.onerror = (error) => {
        log('WebSocket error', 'error');
        console.error(error);
      };
      
    } catch (e: any) {
      log(`Connection error: ${e?.message ?? String(e)}`, 'error');
    }
  }, [wsUrl, log]);

  const send = useCallback((data: any) => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify(data));
      console.log('Sent:', data);
    } else {
      log('Not connected to server', 'error');
    }
  }, [log]);

  const onMessage = useCallback((callback: (d: any) => void) => {
    if (wsRef.current) {
      wsRef.current.onmessage = (event: MessageEvent) => {
        console.log('Raw message received:', event.data);
        try {
          const data = JSON.parse(event.data);
          console.log('Parsed message:', data);
          log(`Received: ${data.type}`, 'info');
          callback(data);
        } catch (e) {
          console.error('Failed to parse message:', e);
          console.error('Raw data was:', event.data);
        }
      };
    }
  }, [log]);

  useEffect(() => {
    connect();
    
    return () => {
      if (reconnectTimeoutRef.current) {
        clearTimeout(reconnectTimeoutRef.current);
      }
      if (wsRef.current) {
        wsRef.current.close();
      }
    };
  }, [connect]);

  return { isConnected, send, onMessage, messages, log };
};
