import { useState } from 'react';

const RegisterPanel = ({ onRegister }) => {
  const [username, setUsername] = useState('');

  const handleSubmit = () => {
    if (!username.trim()) {
      alert('Please enter a username');
      return;
    }
    onRegister(username.trim());
  };

  return (
    <div className="panel">
      <h2>Register</h2>
      <div className="flex-row">
        <input
          type="text"
          value={username}
          onChange={(e) => setUsername(e.target.value)}
          placeholder="Enter username"
          onKeyPress={(e) => e.key === 'Enter' && handleSubmit()}
        />
        <button onClick={handleSubmit}>Register</button>
      </div>
    </div>
  );
};

export default RegisterPanel;