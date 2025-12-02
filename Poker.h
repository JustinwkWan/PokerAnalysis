#ifndef POKER_H
#define POKER_H

#include <vector>
#include <random>

enum class Suit {
    Hearts,
    Diamonds,
    Clubs,
    Spades
};

enum class Rank {
    Two,
    Three,
    Four,
    Five,
    Six,
    Seven,
    Eight,
    Nine,
    Ten,
    Jack,
    Queen,
    King,
    Ace
};

enum class HandRank {
    HighCard,
    OnePair,
    TwoPair,
    ThreeOfAKind,
    Straight,
    Flush,
    FullHouse,
    FourOfAKind,
    StraightFlush,
    RoyalFlush
};

struct Card {
    Rank rank; 
    Suit suit;
    
    Card(Rank r, Suit s) : rank(r), suit(s) {}
};

class Poker {
private: 
    std::vector<Card> cards; 
    
public: 
    Poker();
    ~Poker();
    HandRank evaluateHand(const std::vector<Rank>& handRanks, const std::vector<Suit>& handSuits);
    void dealCards();
    void dealFlop();
    void dealTurn();
    void dealRiver();
    void determineWinner();
    void handleBlinds();
    void getGameState();
};

#endif