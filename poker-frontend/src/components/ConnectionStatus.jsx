const ConnectionStatus = ({ isConnected }) => {
  return (
    <div style={{
      position: 'fixed',
      top: '10px',
      right: '10px',
      padding: '5px 10px',
      background: 'rgba(0,0,0,0.5)',
      borderRadius: '5px',
      fontSize: '12px',
      color: isConnected ? '#69db7c' : '#ff6b6b'
    }}>
      {isConnected ? 'Connected' : 'Disconnected'}
    </div>
  );
};

export default ConnectionStatus;