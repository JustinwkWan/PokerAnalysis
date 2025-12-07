const GameLog = ({ messages }) => {
  return (
    <div className="panel">
      <h2>Game Log</h2>
      <div id="log">
        {messages.map((msg, i) => (
          <div key={i} className={msg.type}>
            [{msg.time}] {msg.message}
          </div>
        ))}
      </div>
    </div>
  );
};

export default GameLog;