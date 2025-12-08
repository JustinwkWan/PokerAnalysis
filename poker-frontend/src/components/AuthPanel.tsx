import { useState } from 'react';

interface AuthPanelProps {
  onAuthSuccess: (username: string, token: string, userId: number) => void;
}

const AuthPanel: React.FC<AuthPanelProps> = ({ onAuthSuccess }) => {
  const [isLogin, setIsLogin] = useState(true);
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [confirmPassword, setConfirmPassword] = useState('');
  const [email, setEmail] = useState('');
  const [error, setError] = useState('');
  const [loading, setLoading] = useState(false);

  const handleSubmit = async () => {
    setError('');
    console.log('Login successful, calling onAuthSuccess');
    if (!username.trim() || !password.trim()) {
      setError('Please enter username and password');
      return;
    }

    if (!isLogin) {
      if (password !== confirmPassword) {
        setError('Passwords do not match');
        return;
      }
      if (password.length < 6) {
        setError('Password must be at least 6 characters');
        return;
      }
    }

    setLoading(true);

    try {
      const endpoint = isLogin ? '/api/login' : '/api/register';
      const body = isLogin
        ? { username, password }
        : { username, password, email: email || undefined };

      const response = await fetch(`http://localhost:3001${endpoint}`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(body)
      });

      const data = await response.json();

      if (!response.ok) {
        setError(data.error || 'Authentication failed');
        setLoading(false);
        return;
      }

      if (isLogin) {
        // Login successful

        console.log('Login successful, calling onAuthSuccess');
        onAuthSuccess(data.username, data.token, data.userId);
        setLoading(false);
      } else {
        // Registration successful - auto login
        setIsLogin(true);
        setError('');

        setLoading(false); // ADD THIS
        alert('Registration successful! Please login.');
      }
    } catch (err) {
      setError('Connection error. Is the auth server running?');
      console.error(err);
    }

    setLoading(false);
  };

  return (
    <div className="panel auth-panel">
      <h1>🃏 Texas Hold'em Poker 🃏</h1>

      <div className="auth-toggle">
        <button
          className={isLogin ? 'active' : ''}
          onClick={() => {
            setIsLogin(true);
            setError('');
          }}
        >
          Login
        </button>
        <button
          className={!isLogin ? 'active' : ''}
          onClick={() => {
            setIsLogin(false);
            setError('');
          }}
        >
          Register
        </button>
      </div>

      <div className="auth-form">
        <h2>{isLogin ? 'Welcome Back' : 'Create Account'}</h2>

        {error && <div className="error-message">{error}</div>}

        <input
          type="text"
          value={username}
          onChange={(e) => setUsername(e.target.value)}
          placeholder="Username"
          disabled={loading}
          onKeyPress={(e) => e.key === 'Enter' && handleSubmit()}
        />

        {!isLogin && (
          <input
            type="email"
            value={email}
            onChange={(e) => setEmail(e.target.value)}
            placeholder="Email (optional)"
            disabled={loading}
            onKeyPress={(e) => e.key === 'Enter' && handleSubmit()}
          />
        )}

        <input
          type="password"
          value={password}
          onChange={(e) => setPassword(e.target.value)}
          placeholder="Password"
          disabled={loading}
          onKeyPress={(e) => e.key === 'Enter' && handleSubmit()}
        />

        {!isLogin && (
          <input
            type="password"
            value={confirmPassword}
            onChange={(e) => setConfirmPassword(e.target.value)}
            placeholder="Confirm Password"
            disabled={loading}
            onKeyPress={(e) => e.key === 'Enter' && handleSubmit()}
          />
        )}

        <button
          onClick={handleSubmit}
          className="auth-submit"
          disabled={loading}
        >
          {loading ? 'Please wait...' : (isLogin ? 'Login' : 'Create Account')}
        </button>
      </div>
    </div>
  );
};

export default AuthPanel;