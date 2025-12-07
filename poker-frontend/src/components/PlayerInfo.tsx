import React from 'react';

type Player = {
  username: string;
  chips: number;
  current_bet?: number;
  has_hand?: boolean;
  is_active?: boolean;
};

const PlayerInfo: React.FC<{ players: Player[]; currentPlayer: number; username: string }> = ({ players, currentPlayer, username }) => {
  return (
    <div id="playerInfo">
      {players.map((p, i) => {
        const isCurrentPlayer = i === currentPlayer;
        const isFolded = !p.has_hand;
        const isMe = p.username === username;

        return (
          <div 
            key={i}
            className={`player-box ${isCurrentPlayer ? 'current' : ''} ${isFolded ? 'folded' : ''}`}
          >
            <h3>{p.username} {isMe ? '(You)' : ''}</h3>
            <div className="player-chips">💰 {p.chips}</div>
            <div className="player-bet">Bet: {p.current_bet || 0}</div>
            {isFolded && <div style={{color: '#ff6b6b'}}>Folded</div>}
            {!p.is_active && p.has_hand && <div style={{color: '#ffd700'}}>All-In</div>}
          </div>
        );
      })}
    </div>
  );
};

export default PlayerInfo;
