#ifndef POKER_H
#define POKER_H

#include <vector>
#include <random>
#include <map>
#include <string>

enum class Suit {
    Hearts,
    Diamonds,
    Clubs,
    Spades
};

enum class Rank {
    Two = 2,
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
    std::vector<Card> cards;              // Deck of cards
    std::vector<Card> communityCards;      // Community cards (flop, turn, river)
    std::random_device rd;                 // Random device for seeding
    std::mt19937 gen;                      // Random number generator
    
    // Helper methods
    Card drawCard();                       // Draw one card from deck
    std::map<Rank, int> countRanks(const std::vector<Card>& hand) const;
    std::map<Suit, int> countSuits(const std::vector<Card>& hand) const;
    bool isFlush(const std::vector<Card>& hand) const;
    bool isStraight(const std::vector<Card>& hand) const;
    
public: 
    Poker();
    ~Poker();
    
    // Deck management
    void resetDeck();                      // Reset and shuffle deck for new round
    
    // Card dealing
    std::vector<Card> dealCards(int numCards = 2);  // Deal cards to a player
    void dealFlop();                       // Deal 3 community cards
    void dealTurn();                       // Deal 4th community card
    void dealRiver();                      // Deal 5th community card
    
    // Game state
    std::vector<Card> getCommunityCards() const;
    std::string getGameState() const;
    
    // Hand evaluation
    HandRank evaluateHand(const std::vector<Card>& holeCards) const;
    std::vector<int> determineWinners(const std::vector<std::vector<Card>>& playerHands) const;
    
    struct HandValue {
        HandRank rank;
        std::vector<int> values;  // For tie-breaking (sorted high to low)
        
        bool operator>(const HandValue& other) const;
        bool operator==(const HandValue& other) const;
    };
    HandValue evaluateHandDetailed(const std::vector<Card>& holeCards) const;

    // Utility functions
    std::string rankToString(Rank r) const;
    std::string suitToString(Suit s) const;
    std::string handRankToString(HandRank hr) const;
};

#endif