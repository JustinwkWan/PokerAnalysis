import dotenv from 'dotenv';

dotenv.config();

import express from 'express';
import mysql from 'mysql2/promise';
import bcrypt from 'bcrypt';
import jwt from 'jsonwebtoken';
import cors from 'cors';


const app = express();
app.use(express.json());
app.use(cors());

const pool = mysql.createPool({
  host: process.env.DB_HOST,
  user: process.env.DB_USER,
  password: process.env.DB_PASSWORD,
  database: process.env.DB_NAME,
  waitForConnections: true,
  connectionLimit: 10
});

// Register endpoint
app.post('/api/register', async (req, res) => {
  try {
    const { username, password, email } = req.body;

    if (!username || !password) {
      return res.status(400).json({ error: 'Username and password required' });
    }

    // Hash password
    const passwordHash = await bcrypt.hash(password, 10);

    // Insert user
    const [result] = await pool.execute(
      'INSERT INTO users (username, password_hash, email) VALUES (?, ?, ?)',
      [username, passwordHash, email || null]
    );

    res.json({ message: 'User registered successfully', userId: (result as any).insertId });
  } catch (error: any) {
    if (error.code === 'ER_DUP_ENTRY') {
      res.status(400).json({ error: 'Username already exists' });
    } else {
      console.error(error);
      res.status(500).json({ error: 'Registration failed' });
    }
  }
});

// Login endpoint
app.post('/api/login', async (req, res) => {
  try {
    const { username, password } = req.body;

    if (!username || !password) {
      return res.status(400).json({ error: 'Username and password required' });
    }

    // Get user
    const [rows] = await pool.execute(
      'SELECT id, username, password_hash FROM users WHERE username = ?',
      [username]
    );

    const users = rows as any[];
    if (users.length === 0) {
      return res.status(401).json({ error: 'Invalid credentials' });
    }

    const user = users[0];

    // Verify password
    const validPassword = await bcrypt.compare(password, user.password_hash);
    if (!validPassword) {
      return res.status(401).json({ error: 'Invalid credentials' });
    }

    // Generate JWT
    const token = jwt.sign(
      { userId: user.id, username: user.username },
      process.env.JWT_SECRET!,
      { expiresIn: '24h' }
    );

    res.json({ token, username: user.username, userId: user.id });
  } catch (error) {
    console.error(error);
    res.status(500).json({ error: 'Login failed' });
  }
});

// Record a player action
app.post('/api/stats/action', async (req, res) => {
  try {
    const { userId, gameId, handNumber, position, action, amount, stage, cards } = req.body;

    await pool.execute(
      `INSERT INTO player_actions 
       (user_id, game_id, hand_number, position, action, amount, stage, cards) 
       VALUES (?, ?, ?, ?, ?, ?, ?, ?)`,
      [userId, gameId, handNumber, position, action, amount, stage, cards]
    );

    res.json({ message: 'Action recorded' });
  } catch (error) {
    console.error(error);
    res.status(500).json({ error: 'Failed to record action' });
  }
});

// Update player stats after a hand
app.post('/api/stats/update', async (req, res) => {
  try {
    const { userId, handsPlayed, handsWon, totalWagered, totalWon, vpip, pfr, showdown, showdownWon } = req.body;

    // Insert or update stats
    await pool.execute(
      `INSERT INTO player_stats 
       (user_id, hands_played, hands_won, total_wagered, total_won, vpip_count, pfr_count, showdowns, showdowns_won)
       VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
       ON DUPLICATE KEY UPDATE
       hands_played = hands_played + ?,
       hands_won = hands_won + ?,
       total_wagered = total_wagered + ?,
       total_won = total_won + ?,
       vpip_count = vpip_count + ?,
       pfr_count = pfr_count + ?,
       showdowns = showdowns + ?,
       showdowns_won = showdowns_won + ?`,
      [userId, handsPlayed, handsWon, totalWagered, totalWon, vpip, pfr, showdown, showdownWon,
       handsPlayed, handsWon, totalWagered, totalWon, vpip, pfr, showdown, showdownWon]
    );

    res.json({ message: 'Stats updated' });
  } catch (error) {
    console.error(error);
    res.status(500).json({ error: 'Failed to update stats' });
  }
});

// Get player stats
app.get('/api/stats/:userId', async (req, res) => {
  try {
    const { userId } = req.params;

    const [rows] = await pool.execute(
      `SELECT * FROM player_stats WHERE user_id = ?`,
      [userId]
    );

    const stats = (rows as any[])[0];
    
    if (!stats) {
      // Return default stats if none exist
      return res.json({
        hands_played: 0,
        hands_won: 0,
        total_wagered: 0,
        total_won: 0,
        vpip_count: 0,
        pfr_count: 0,
        showdowns: 0,
        showdowns_won: 0,
        vpip_percentage: 0,
        pfr_percentage: 0,
        win_rate: 0,
        showdown_win_rate: 0
      });
    }

    // Calculate percentages
    const vpipPercentage = stats.hands_played > 0 ? (stats.vpip_count / stats.hands_played * 100).toFixed(1) : 0;
    const pfrPercentage = stats.hands_played > 0 ? (stats.pfr_count / stats.hands_played * 100).toFixed(1) : 0;
    const winRate = stats.hands_played > 0 ? (stats.hands_won / stats.hands_played * 100).toFixed(1) : 0;
    const showdownWinRate = stats.showdowns > 0 ? (stats.showdowns_won / stats.showdowns * 100).toFixed(1) : 0;

    res.json({
      ...stats,
      vpip_percentage: vpipPercentage,
      pfr_percentage: pfrPercentage,
      win_rate: winRate,
      showdown_win_rate: showdownWinRate
    });
  } catch (error) {
    console.error(error);
    res.status(500).json({ error: 'Failed to fetch stats' });
  }
});

// Get hand history for a player
app.get('/api/stats/history/:userId', async (req, res) => {
  try {
    const { userId } = req.params;
    const limit = req.query.limit || 50;

    const [rows] = await pool.execute(
      `SELECT * FROM player_actions 
       WHERE user_id = ? 
       ORDER BY timestamp DESC 
       LIMIT ?`,
      [userId, limit]
    );

    res.json({ history: rows });
  } catch (error) {
    console.error(error);
    res.status(500).json({ error: 'Failed to fetch history' });
  }
});

// Get game summary (all actions in a specific game)
app.get('/api/stats/game/:gameId', async (req, res) => {
  try {
    const { gameId } = req.params;

    const [rows] = await pool.execute(
      `SELECT pa.*, u.username 
       FROM player_actions pa
       JOIN users u ON pa.user_id = u.id
       WHERE pa.game_id = ?
       ORDER BY pa.hand_number, pa.timestamp`,
      [gameId]
    );

    res.json({ actions: rows });
  } catch (error) {
    console.error(error);
    res.status(500).json({ error: 'Failed to fetch game summary' });
  }
});

// Verify token endpoint
app.get('/api/verify', async (req, res) => {
  try {
    const token = req.headers.authorization?.split(' ')[1];
    
    if (!token) {
      return res.status(401).json({ error: 'No token provided' });
    }

    const decoded = jwt.verify(token, process.env.JWT_SECRET!) as any;
    res.json({ valid: true, username: decoded.username, userId: decoded.userId });
  } catch (error) {
    res.status(401).json({ error: 'Invalid token' });
  }
});

const PORT = process.env.PORT || 3001;
app.listen(PORT, () => {
  console.log(`Auth server running on port ${PORT}`);
});