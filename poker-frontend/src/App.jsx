import { useState, useEffect } from 'react';
import { useWebSocket } from './hooks/useWebSocket';
import ConnectionStatus from './components/ConnectionStatus';
import RegisterPanel from './components/RegisterPanel';
import LobbyPanel from './components/LobbyPanel';
import GameRoom from './components/GameRoom';
import GameLog from './components/GameLog';
import './styles/poker.css';

function App() {
  const [serverUrl, setServerUrl] = useState('http://localhost:8081');
  const { isConnected, send, onMessage, messages, log } = useWebSocket(serverUrl);
  
  const [username, setUsername] = useState('');
  const [currentGameId, setCurrentGameId] = useState(null);
  const [myPosition, setMyPosition] = useState(-1);
  const [games, setGames] = useState([]);
  const [gameState, setGameState] = useState({
    pot: 0,
    currentBet: 0,
    stage: 'WAITING',
    communityCards: [],
    myCards: [],
    players: [],
    currentPlayer: -1
  });
  
  const [showRegister, setShowRegister] = useState(false);
  const [showLobby, setShowLobby] = useState(false);
  const [showGameRoom, setShowGameRoom] = useState(false);

  useEffect(() => {
    if (isConnected && !showRegister && !showLobby && !showGameRoom) {
      setShowRegister(true);
    }
  }, [isConnected]);

  useEffect(() => {
    onMessage((data) => handleMessage(data));
  }, [onMessage]);

  const handleMessage = (data) => {
    switch (data.type) {
      case 'REGISTER_RESPONSE':
        if (data.status === 'SUCCESS') {
          log(data.message, 'success');
          setShowRegister(false);
          setShowLobby(true);
          send({ type: 'LIST_GAMES' });
        } else {
          log(`Registration failed: ${data.error}`, 'error');
        }
        break;
        
      case 'UNREGISTER_RESPONSE':
        if (data.status === 'SUCCESS') {
          log('Logged out successfully', 'success');
          setUsername('');
          setShowLobby(false);
          setShowRegister(true);
        } else {
          log(`Logout failed: ${data.error}`, 'error');
        }
        break;
        
      case 'LIST_GAMES_RESPONSE':
        log(`Found ${data.games ? data.games.length : 0} games`, 'info');
        setGames(data.games || []);
        break;
        
      case 'CREATE_GAME_RESPONSE':
        if (data.status === 'SUCCESS') {
          log(`Game created with ID: ${data.game_id}`, 'success');
          // Auto-join
          handleJoinGame(data.game_id);
        } else {
          log(`Failed to create game: ${data.error}`, 'error');
        }
        break;
        
      case 'JOIN_GAME_RESPONSE':
        if (data.status === 'SUCCESS') {
          log('Joined game successfully', 'success');
          setShowLobby(false);
          setShowGameRoom(true);
        } else {
          log(`Failed to join game: ${data.error}`, 'error');
          setCurrentGameId(null);
        }
        break;
        
      case 'EXIT_GAME_RESPONSE':
        if (data.status === 'SUCCESS') {
          log('Exited game', 'success');
          setCurrentGameId(null);
          setShowGameRoom(false);
          setShowLobby(true);
          send({ type: 'LIST_GAMES' });
        } else {
          log(`Failed to exit game: ${data.error}`, 'error');
        }
        break;
        
      case 'START_GAME_RESPONSE':
        if (data.status === 'SUCCESS') {
          log('Game started!', 'success');
        } else {
          log(`Failed to start game: ${data.error}`, 'error');
        }
        break;
        
      case 'GAME_STARTED':
        handleGameStarted(data);
        break;
        
      case 'GAME_STATE_UPDATE':
        handleGameStateUpdate(data);
        break;
        
      case 'PLAY_TURN_RESPONSE':
        if (data.status === 'SUCCESS') {
          log(data.message, 'success');
        } else {
          log(`Action failed: ${data.error}`, 'error');
        }
        break;
        
      case 'SHOWDOWN':
        handleShowdown(data);
        break;
        
      case 'GAME_OVER':
        handleGameOver(data);
        break;
        
      default:
        log(`Unknown message type: ${data.type}`, 'info');
    }
  };

  const handleRegister = (name) => {
    setUsername(name);
    send({ type: 'REGISTER', name });
  };

  const handleUnregister = () => {
    if (currentGameId) {
      log('Please exit the game first before logging out', 'error');
      return;
    }
    send({ type: 'UNREGISTER', username });
  };

  const handleListGames = () => {
    send({ type: 'LIST_GAMES' });
  };

  const handleCreateGame = (smallBlind, bigBlind) => {
    send({ type: 'CREATE_GAME', small_blind: smallBlind, big_blind: bigBlind });
  };

  const handleJoinGame = (gameId, startingStack = 1000) => {
    setCurrentGameId(gameId);
    log(`Joining game ${gameId} with stack ${startingStack}...`, 'info');
    send({
      type: 'JOIN_GAME',
      game_id: gameId,
      username,
      starting_stack: startingStack
    });
  };

  const handleExitGame = () => {
    send({ type: 'EXIT_GAME', username });
  };

  const handleStartGame = () => {
    send({ type: 'START_GAME', game_id: currentGameId, username });
  };

  const handleAction = (action, amount = 0) => {
    const message = {
      type: 'PLAY_TURN',
      username,
      game_id: currentGameId,
      action
    };
    
    if (action === 'BET' || action === 'RAISE' || action === 'CALL') {
      if (action === 'CALL') {
        const myPlayer = gameState.players.find(p => p.username === username);
        message.amount = gameState.currentBet - (myPlayer?.current_bet || 0);
      } else {
        message.amount = amount;
      }
    }
    
    send(message);
  };

  const handleGameStarted = (data) => {
    log('Game has started!', 'success');
    setMyPosition(data.your_position);
    setGameState({
      pot: data.pot,
      currentBet: data.current_bet,
      stage: 'PREFLOP',
      communityCards: [],
      myCards: data.hole_cards,
      players: data.players,
      currentPlayer: data.current_player
    });
  };

  const handleGameStateUpdate = (data) => {
    log(`Stage: ${data.stage}`, 'info');
    setGameState({
      pot: data.pot,
      currentBet: data.current_bet,
      stage: data.stage,
      communityCards: data.community_cards,
      myCards: gameState.myCards,
      players: data.players,
      currentPlayer: data.current_player
    });
  };

  const handleShowdown = (data) => {
    log('=== SHOWDOWN ===', 'success');
    
    data.all_hands.forEach(h => {
      log(`${h.username}: ${h.cards.map(c => c.rank + c.suit).join(' ')} - ${h.hand_rank}`, 'info');
    });
    
    data.winners.forEach(w => {
      log(`🏆 ${w.username} wins ${w.chips_won} with ${w.hand_rank}!`, 'success');
    });
    
    setGameState(prev => ({
      ...prev,
      communityCards: data.community_cards,
      players: data.players,
      currentPlayer: -1,
      myCards: [],
      stage: 'WAITING'
    }));
    
    const activePlayers = data.players.filter(p => p.chips > 0);
    if (activePlayers.length > 1) {
      log('Next hand starting in 2 seconds...', 'info');
      setTimeout(() => {
        if (currentGameId) handleStartGame();
      }, 2000);
    } else {
      log(`🎉 GAME OVER! ${activePlayers[0]?.username} wins the game!`, 'success');
    }
  };

  const handleGameOver = (data) => {
    log(`🏆 ${data.winner} wins ${data.pot}! (${data.reason})`, 'success');
    
    setGameState(prev => ({
      ...prev,
      players: data.players,
      currentPlayer: -1,
      myCards: [],
      communityCards: [],
      stage: 'WAITING',
      pot: 0,
      currentBet: 0
    }));
    
    const activePlayers = data.players.filter(p => p.chips > 0);
    if (activePlayers.length > 1) {
      log('Next hand starting in 2 seconds...', 'info');
      setTimeout(() => {
        if (currentGameId) handleStartGame();
      }, 2000);
    } else {
      log(`🎉 GAME OVER! ${activePlayers[0]?.username} wins the game!`, 'success');
    }
  };

  return (
    <div className="container">
      <ConnectionStatus isConnected={isConnected} />
      
      <input 
        type="text" 
        value={serverUrl} 
        onChange={(e) => setServerUrl(e.target.value)}
        placeholder="ws://server:8081"
        style={{width: '300px', margin: '10px'}}
      />
      
      {showRegister && (
        <RegisterPanel onRegister={handleRegister} />
      )}
      
      {showLobby && (
        <LobbyPanel
          username={username}
          games={games}
          onListGames={handleListGames}
          onCreateGame={handleCreateGame}
          onJoinGame={handleJoinGame}
          onUnregister={handleUnregister}
        />
      )}
      
      {showGameRoom && (
        <GameRoom
          gameId={currentGameId}
          username={username}
          gameState={gameState}
          myPosition={myPosition}
          onStartGame={handleStartGame}
          onExitGame={handleExitGame}
          onAction={handleAction}
        />
      )}
      
      <GameLog messages={messages} />
    </div>
  );
}

export default App;