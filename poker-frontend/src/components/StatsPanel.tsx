import { useState, useEffect } from 'react';

interface StatsData {
  hands_played: number;
  hands_won: number;
  total_wagered: number;
  total_won: number;
  vpip_count: number;
  pfr_count: number;
  showdowns: number;
  showdowns_won: number;
  vpip_percentage: number;
  pfr_percentage: number;
  win_rate: number;
  showdown_win_rate: number;
}

interface HandHistory {
  id: number;
  game_id: number;
  hand_number: number;
  position: string;
  action: string;
  amount: number;
  stage: string;
  cards: string;
  timestamp: string;
}

interface StatsPanelProps {
  userId: number;
  username: string;
  onClose: () => void;
}

const StatsPanel: React.FC<StatsPanelProps> = ({ userId, username, onClose }) => {
  const [stats, setStats] = useState<StatsData | null>(null);
  const [history, setHistory] = useState<HandHistory[]>([]);
  const [loading, setLoading] = useState(true);
  const [activeTab, setActiveTab] = useState<'stats' | 'history'>('stats');

  useEffect(() => {
    fetchStats();
    fetchHistory();
  }, [userId]);

  const fetchStats = async () => {
    try {
      const response = await fetch(`http://localhost:3001/api/stats/${userId}`);
      const data = await response.json();
      setStats(data);
      setLoading(false);
    } catch (error) {
      console.error('Failed to fetch stats:', error);
      setLoading(false);
    }
  };

  const fetchHistory = async () => {
    try {
      const response = await fetch(`http://localhost:3001/api/stats/history/${userId}?limit=20`);
      const data = await response.json();
      setHistory(data.history || []);
    } catch (error) {
      console.error('Failed to fetch history:', error);
    }
  };

  if (loading) {
    return (
      <div className="panel stats-panel">
        <h2>Loading Stats...</h2>
      </div>
    );
  }

  return (
    <div className="panel stats-panel">
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
        <h2>📊 Stats for {username}</h2>
        <button onClick={onClose}>Close</button>
      </div>

      <div className="stats-tabs">
        <button 
          className={activeTab === 'stats' ? 'active' : ''}
          onClick={() => setActiveTab('stats')}
        >
          Statistics
        </button>
        <button 
          className={activeTab === 'history' ? 'active' : ''}
          onClick={() => setActiveTab('history')}
        >
          Hand History
        </button>
      </div>

      {activeTab === 'stats' && stats && (
        <div className="stats-content">
          <div className="stats-grid">
            {/* Overall Stats */}
            <div className="stat-card">
              <h3>Overall Performance</h3>
              <div className="stat-row">
                <span>Hands Played:</span>
                <span className="stat-value">{stats.hands_played}</span>
              </div>
              <div className="stat-row">
                <span>Hands Won:</span>
                <span className="stat-value success">{stats.hands_won}</span>
              </div>
              <div className="stat-row">
                <span>Win Rate:</span>
                <span className="stat-value">{stats.win_rate}%</span>
              </div>
              <div className="stat-row">
                <span>Net Profit:</span>
                <span className={`stat-value ${stats.total_won - stats.total_wagered >= 0 ? 'success' : 'error'}`}>
                  {stats.total_won - stats.total_wagered > 0 ? '+' : ''}{stats.total_won - stats.total_wagered}
                </span>
              </div>
            </div>

            {/* Playing Style */}
            <div className="stat-card">
              <h3>Playing Style</h3>
              <div className="stat-row">
                <span>VPIP:</span>
                <span className="stat-value">{stats.vpip_percentage}%</span>
                <span className="stat-label">
                  {parseFloat(stats.vpip_percentage as any) < 20 ? '(Tight)' : 
                   parseFloat(stats.vpip_percentage as any) < 30 ? '(Normal)' : '(Loose)'}
                </span>
              </div>
              <div className="stat-row">
                <span>PFR:</span>
                <span className="stat-value">{stats.pfr_percentage}%</span>
                <span className="stat-label">
                  {parseFloat(stats.pfr_percentage as any) < 15 ? '(Passive)' : 
                   parseFloat(stats.pfr_percentage as any) < 25 ? '(Balanced)' : '(Aggressive)'}
                </span>
              </div>
              <div className="stat-row">
                <span>VPIP/PFR Gap:</span>
                <span className="stat-value">
                  {(parseFloat(stats.vpip_percentage as any) - parseFloat(stats.pfr_percentage as any)).toFixed(1)}
                </span>
              </div>
            </div>

            {/* Showdown Stats */}
            <div className="stat-card">
              <h3>Showdown Performance</h3>
              <div className="stat-row">
                <span>Showdowns:</span>
                <span className="stat-value">{stats.showdowns}</span>
              </div>
              <div className="stat-row">
                <span>Showdowns Won:</span>
                <span className="stat-value success">{stats.showdowns_won}</span>
              </div>
              <div className="stat-row">
                <span>Showdown Win Rate:</span>
                <span className="stat-value">{stats.showdown_win_rate}%</span>
                <span className="stat-label">
                  {parseFloat(stats.showdown_win_rate as any) > 50 ? '(Good)' : '(Needs Work)'}
                </span>
              </div>
            </div>

            {/* Money Stats */}
            <div className="stat-card">
              <h3>Money Management</h3>
              <div className="stat-row">
                <span>Total Wagered:</span>
                <span className="stat-value">{stats.total_wagered}</span>
              </div>
              <div className="stat-row">
                <span>Total Won:</span>
                <span className="stat-value success">{stats.total_won}</span>
              </div>
              <div className="stat-row">
                <span>ROI:</span>
                <span className={`stat-value ${stats.total_wagered > 0 && ((stats.total_won / stats.total_wagered - 1) * 100) >= 0 ? 'success' : 'error'}`}>
                  {stats.total_wagered > 0 ? `${((stats.total_won / stats.total_wagered - 1) * 100).toFixed(1)}%` : 'N/A'}
                </span>
              </div>
            </div>
          </div>

          {stats.hands_played === 0 && (
            <div className="no-stats">
              <p>No statistics yet. Play some hands to see your stats!</p>
            </div>
          )}
        </div>
      )}

      {activeTab === 'history' && (
        <div className="history-content">
          {history.length === 0 ? (
            <p>No hand history yet.</p>
          ) : (
            <div className="history-list">
              <table className="history-table">
                <thead>
                  <tr>
                    <th>Time</th>
                    <th>Game</th>
                    <th>Hand</th>
                    <th>Position</th>
                    <th>Stage</th>
                    <th>Action</th>
                    <th>Amount</th>
                    <th>Cards</th>
                  </tr>
                </thead>
                <tbody>
                  {history.map((hand) => (
                    <tr key={hand.id}>
                      <td>{new Date(hand.timestamp).toLocaleTimeString()}</td>
                      <td>#{hand.game_id}</td>
                      <td>#{hand.hand_number}</td>
                      <td>{hand.position}</td>
                      <td>{hand.stage}</td>
                      <td className={`action-${hand.action.toLowerCase()}`}>{hand.action}</td>
                      <td>{hand.amount || '-'}</td>
                      <td>{hand.cards || '-'}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          )}
        </div>
      )}
    </div>
  );
};

export default StatsPanel;