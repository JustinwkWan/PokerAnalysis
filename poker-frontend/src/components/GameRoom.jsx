import CommunityCards from './CommunityCards';
import PlayerCards from './PlayerCards';
import PlayerInfo from './PlayerInfo';
import ActionPanel from './ActionPanel';

const GameRoom = ({ gameId, username, gameState, myPosition, onStartGame, onExitGame, onAction }) => {
  const isMyTurn = gameState.players[gameState.currentPlayer]?.username === username;

  return (
    <div className="panel">
      <h2>Game Room <span id="gameRoomId">#{gameId}</span></h2>
      
      <div className="flex-row" style={{marginBottom: '15px'}}>
        <button onClick={onStartGame}>Start Game</button>
        <button onClick={onExitGame}>Exit Game</button>
      </div>

      {/* Game Info */}
      <div id="gameInfo">
        <span>Pot: <span id="pot">{gameState.pot}</span></span> | 
        <span>Current Bet: <span id="currentBet">{gameState.currentBet}</span></span> | 
        <span>Stage: <span id="stage">{gameState.stage}</span></span>
      </div>

      {/* Community Cards */}
      <CommunityCards cards={gameState.communityCards} />

      {/* My Cards */}
      <PlayerCards cards={gameState.myCards} />

      {/* Players */}
      <PlayerInfo 
        players={gameState.players} 
        currentPlayer={gameState.currentPlayer}
        username={username}
      />

      {/* Actions */}
      {isMyTurn && (
        <ActionPanel
          currentBet={gameState.currentBet}
          myBet={gameState.players[gameState.currentPlayer]?.current_bet || 0}
          onAction={onAction}
        />
      )}
    </div>
  );
};

export default GameRoom;