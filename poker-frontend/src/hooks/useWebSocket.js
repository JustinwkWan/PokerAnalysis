import { useState, useEffect, useRef, useCallback } from 'react';

export const useWebSocket = (url) => {
  const [isConnected, setIsConnected] = useState(false);
  const [messages, setMessages] = useState([]);
  const wsRef = useRef(null);
  const reconnectTimeoutRef = useRef(null);

  const log = useCallback((message, type = 'info') => {
    const entry = {
      time: new Date().toLocaleTimeString(),
      message,
      type
    };
    setMessages(prev => [...prev, entry]);
  }, []);

  const connect = useCallback(() => {
    log(`Connecting to ${url}...`);
    
    try {
      wsRef.current = new WebSocket(url);
      
      wsRef.current.onopen = () => {
        log('Connected to server!', 'success');
        setIsConnected(true);
      };
      
      wsRef.current.onclose = () => {
        log('Disconnected from server', 'error');
        setIsConnected(false);
        
        // Auto-reconnect after 2 seconds
        reconnectTimeoutRef.current = setTimeout(() => {
          log('Attempting to reconnect...', 'info');
          connect();
        }, 2000);
      };
      
      wsRef.current.onerror = (error) => {
        log('WebSocket error', 'error');
        console.error(error);
      };
      
    } catch (e) {
      log(`Connection error: ${e.message}`, 'error');
    }
  }, [url, log]);

  const send = useCallback((data) => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify(data));
      console.log('Sent:', data);
    } else {
      log('Not connected to server', 'error');
    }
  }, [log]);

  const onMessage = useCallback((callback) => {
    if (wsRef.current) {
      wsRef.current.onmessage = (event) => {
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