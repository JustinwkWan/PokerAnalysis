import { useState } from 'react';

const ActionPanel = ({ currentBet, myBet, onAction }) => {
  const [betAmount, setBetAmount] = useState(20);
  
  const callAmount = currentBet - myBet;
  const canCheck = currentBet === myBet;
  const canCall = currentBet > myBet;

  const handleBetOrRaise = () => {
    if (currentBet === 0) {
      onAction('BET', betAmount);
    } else {
      onAction('RAISE', betAmount);
    }
  };

  return (
    <div id="actionPanel" style={{textAlign: 'center', marginTop: '20px'}}>
      <h3>Your Turn!</h3>
      <div className="flex-row" style={{justifyContent: 'center'}}>
        <button className="action-btn fold" onClick={() => onAction('FOLD')}>
          Fold
        </button>
        <button 
          className="action-btn check" 
          onClick={() => onAction('CHECK')}
          disabled={!canCheck}
        >
          Check
        </button>
        <button 
          className="action-btn call" 
          onClick={() => onAction('CALL')}
          disabled={!canCall}
        >
          Call {callAmount > 0 ? `(${callAmount})` : ''}
        </button>
        <button 
          className={`action-btn ${currentBet === 0 ? 'bet' : 'raise'}`}
          onClick={handleBetOrRaise}
        >
          {currentBet === 0 ? 'Bet' : 'Raise'}
        </button>
      </div>
      <div className="flex-row" style={{justifyContent: 'center', marginTop: '10px'}}>
        <label>
          Amount: 
          <input
            type="number"
            value={betAmount}
            onChange={(e) => setBetAmount(parseInt(e.target.value) || 0)}
            min="1"
            style={{width: '100px'}}
          />
        </label>
      </div>
    </div>
  );
};

export default ActionPanel;