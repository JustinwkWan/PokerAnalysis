# Texas Hold'em Poker - How to Play

This guide walks you through the complete gameplay experience from logging in to logging out.

## Getting Started

### Step 1: Start the Server
Open a terminal and run:
```bash
./poker_server
```
You should see:
```
TCP Server started on port 8080
WebSocket Server started on port 8081
```

### Step 2: Start the Web Server
Open another terminal and run:
```bash
python3 -m http.server 8000
```

### Step 3: Open the Game
Open your web browser and go to:
```
http://localhost:8000/index.html
```

## Registration

1. The game automatically connects to the server
2. You'll see "Connected" in the top-right corner
3. Enter a unique username in the text field
4. Click **Register**
5. You'll be taken to the Game Lobby

## Game Lobby

In the lobby, you can:

- **Refresh Games**: See available games to join
- **Create Game**: Create a new poker table
- **Logout**: Unregister and return to login screen

### Creating a Game

1. Click **Create Game**
2. Set the blind amounts:
   - **Small Blind**: The smaller forced bet (default: 10)
   - **Big Blind**: The larger forced bet (default: 20)
   - **Starting Stack**: How many chips each player starts with (default: 1000)
3. Click **Create**
4. You'll automatically join the game you created

### Joining an Existing Game

1. Click **Refresh Games** to see available games
2. Games show: Game ID, Blinds, Player Count, and Status
3. Click **Join** on any game in "WAITING" status
4. You'll enter the game room

## Game Room

### Waiting for Players

- You need at least 2 players to start
- Share the game with friends - they can join from their own browser
- Click **Start Game** when ready

### Playing a Hand

Once the game starts:

1. **Your Cards**: Your two hole cards are displayed in "Your Cards" section
2. **Community Cards**: Shared cards appear in the center as they're dealt
3. **Game Info**: Shows the current pot, bet amount, and stage (PREFLOP, FLOP, TURN, RIVER)
4. **Players**: Shows all players, their chips, and current bets

### Taking Actions

When it's your turn, you'll see "Your Turn!" and action buttons:

| Button | When Available | What It Does |
|--------|----------------|--------------|
| **Fold** | Always | Give up your hand and lose any chips you've bet |
| **Check** | When no one has bet | Pass without betting |
| **Call (X)** | When someone has bet | Match the current bet (X shows amount needed) |
| **Bet** | When no one has bet yet | Make the first bet (enter amount in the Amount field) |
| **Raise** | When someone has bet | Increase the bet (enter total amount in the Amount field) |

### Betting Tips

- The **Amount** field is used for Bet and Raise actions
- Minimum bet is the big blind amount
- Minimum raise is double the current bet
- To go all-in, simply bet/raise your entire chip stack

### Hand Stages

1. **PREFLOP**: Each player has 2 hole cards, betting begins
2. **FLOP**: 3 community cards are dealt, betting round
3. **TURN**: 4th community card is dealt, betting round
4. **RIVER**: 5th community card is dealt, final betting round
5. **SHOWDOWN**: Remaining players reveal cards, best hand wins

### Winning

- If everyone else folds, you win the pot automatically
- At showdown, the best 5-card hand wins
- Hands are ranked (highest to lowest):
  - Royal Flush
  - Straight Flush
  - Four of a Kind
  - Full House
  - Flush
  - Straight
  - Three of a Kind
  - Two Pair
  - One Pair
  - High Card

### Next Hand

- After each hand, the next hand starts automatically in 2 seconds
- The dealer button moves clockwise
- Blinds are posted automatically

### Game Over

- The game ends when one player has all the chips
- The winner is announced in the game log

## Leaving the Game

### Exit Game
1. Click **Exit Game** in the game room
2. You'll return to the lobby
3. Your chips are forfeited

### Logout
1. From the lobby, click **Logout**
2. You'll return to the registration screen
3. You can register with a new username or the same one

## Multiplayer Setup

To play with friends:

1. All players open `http://localhost:8000/index.html` in their browsers
2. Each player registers with a **unique username**
3. One player creates a game
4. Other players click **Refresh Games** and **Join** the game
5. Any player can click **Start Game** when ready