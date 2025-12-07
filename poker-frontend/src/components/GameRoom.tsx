import React from 'react';
import CommunityCards from './CommunityCards';
import PlayerCards from './PlayerCards';
import PlayerInfo from './PlayerInfo';
import ActionPanel from './ActionPanel';

type Card = { rank: string; suit: string };
type Player = {
  username: string;
  chips: number;
  current_bet?: number;
  has_hand?: boolean;
  is_active?: boolean;
};

type GameState = {
  pot: number;
  currentBet: number;
  stage: string;
  communityCards: Card[];
  myCards: Card[];
  players: Player[];
  currentPlayer: number;
};

type Props = {
  gameId: string | null;
  username: string;
  gameState: GameState;
  myPosition: number;
  onStartGame: () => void;
  onExitGame: () => void;
  onAction: (action: string, amount?: number) => void;
};

const GameRoom: React.FC<Props> = ({ gameId, username, gameState, myPosition, onStartGame, onExitGame, onAction }) => {
  const isMyTurn = gameState.players[gameState.currentPlayer]?.username === username;

  return (
    <div className="panel">
      <h2>Game Room <span id="gameRoomId">#{gameId}</span></h2>
      
      <div className="flex-row" style={{marginBottom: '15px'}}>
        <button onClick={onStartGame}>Start Game</button>
        <button onClick={onExitGame}>Exit Game</button>
      </div>

      <div id="gameInfo">
        <span>Pot: <span id="pot">{gameState.pot}</span></span> | 
        <span>Current Bet: <span id="currentBet">{gameState.currentBet}</span></span> | 
        <span>Stage: <span id="stage">{gameState.stage}</span></span>
      </div>

      <CommunityCards cards={gameState.communityCards} />

      <PlayerCards cards={gameState.myCards} />

      <PlayerInfo 
        players={gameState.players} 
        currentPlayer={gameState.currentPlayer}
        username={username}
      />

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
