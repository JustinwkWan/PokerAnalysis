import React, { useState } from 'react';

type Props = {
  username: string;
  games: any[];
  onListGames: () => void;
  onCreateGame: (smallBlind: number, bigBlind: number) => void;
  onJoinGame: (gameId: any, startingStack?: number) => void;
  onUnregister: () => void;
};

const LobbyPanel: React.FC<Props> = ({ username, games, onListGames, onCreateGame, onJoinGame, onUnregister }) => {
  const [showCreateForm, setShowCreateForm] = useState(false);
  const [smallBlind, setSmallBlind] = useState<number>(10);
  const [bigBlind, setBigBlind] = useState<number>(20);
  const [startingStack, setStartingStack] = useState<number>(1000);

  const handleCreate = () => {
    onCreateGame(smallBlind, bigBlind);
    setShowCreateForm(false);
  };

  const handleJoin = (gameId: any) => {
    onJoinGame(gameId, startingStack);
  };

  return (
    <div className="panel">
      <h2>Game Lobby <span style={{fontSize: '14px', color: '#aaa'}}>({username})</span></h2>
      
      <div className="flex-row" style={{marginBottom: '15px'}}>
        <button onClick={onListGames}>Refresh Games</button>
        <button onClick={() => setShowCreateForm(!showCreateForm)}>Create Game</button>
        <button onClick={onUnregister} style={{background: '#dc3545', color: 'white'}}>Logout</button>
      </div>

      {showCreateForm && (
        <div style={{margin: '15px 0', padding: '15px', background: 'rgba(0,0,0,0.3)', borderRadius: '5px'}}>
          <h3>Create New Game</h3>
          <div className="flex-row">
            <label>
              Small Blind: 
              <input
                type="number"
                value={smallBlind}
                onChange={(e) => setSmallBlind(parseInt(e.target.value))}
                min="1"
                style={{width: '80px'}}
              />
            </label>
            <label>
              Big Blind: 
              <input
                type="number"
                value={bigBlind}
                onChange={(e) => setBigBlind(parseInt(e.target.value))}
                min="2"
                style={{width: '80px'}}
              />
            </label>
            <label>
              Starting Stack: 
              <input
                type="number"
                value={startingStack}
                onChange={(e) => setStartingStack(parseInt(e.target.value))}
                min="100"
                style={{width: '100px'}}
              />
            </label>
            <button onClick={handleCreate}>Create</button>
            <button onClick={() => setShowCreateForm(false)}>Cancel</button>
          </div>
        </div>
      )}

      <div className="games-list">
        {games.length === 0 ? (
          <p>No games available. Create one!</p>
        ) : (
          games.map(g => (
            <div key={g.game_id} className="game-item">
              <span>
                Game #{g.game_id} | Blinds: {g.small_blind}/{g.big_blind} | 
                Players: {g.player_count} | {g.game_stage}
              </span>
              <button 
                onClick={() => handleJoin(g.game_id)}
                disabled={g.game_stage !== 'WAITING'}
              >
                {g.game_stage === 'WAITING' ? 'Join' : 'In Progress'}
              </button>
            </div>
          ))
        )}
      </div>
    </div>
  );
};

export default LobbyPanel;
