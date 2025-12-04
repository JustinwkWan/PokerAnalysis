#ifndef POKER_H
#define POKER_H

#include <vector>
#include <map>
#include <random>
#include <string>

// Enums for card ranks and suits
enum class Rank {
    Two = 2,
    Three = 3,
    Four = 4,
    Five = 5,
    Six = 6,
    Seven = 7,
    Eight = 8,
    Nine = 9,
    Ten = 10,
    Jack = 11,
    Queen = 12,
    King = 13,
    Ace = 14
};

enum class Suit {
    Hearts = 0,
    Diamonds = 1,
    Clubs = 2,
    Spades = 3
};

// Card structure
struct Card {
    Rank rank;
    Suit suit;
    
    Card(Rank r, Suit s) : rank(r), suit(s) {}
};

// Hand rankings from lowest to highest
enum class HandRank {
    HighCard = 0,
    OnePair = 1,
    TwoPair = 2,
    ThreeOfAKind = 3,
    Straight = 4,
    Flush = 5,
    FullHouse = 6,
    FourOfAKind = 7,
    StraightFlush = 8,
    RoyalFlush = 9
};

// Main Poker game engine class
class Poker {
public:
    // Hand value structure for detailed comparison
    struct HandValue {
        HandRank rank;
        std::vector<int> values;  // For tie-breaking (kickers)
        
        bool operator>(const HandValue& other) const;
        bool operator==(const HandValue& other) const;
    };
    
    // Constructor and destructor
    Poker();
    ~Poker();
    
    // Deck management
    void resetDeck();
    Card drawCard();
    std::vector<Card> dealCards(int numCards);
    
    // Community card dealing
    void dealFlop();
    void dealTurn();
    void dealRiver();
    std::vector<Card> getCommunityCards() const;
    
    // Hand evaluation
    HandRank evaluateHand(const std::vector<Card>& holeCards) const;
    HandValue evaluateHandDetailed(const std::vector<Card>& holeCards) const;
    
    // Winner determination
    std::vector<int> determineWinners(const std::vector<std::vector<Card>>& playerHands) const;
    
    // Utility functions
    std::string getGameState() const;
    std::string rankToString(Rank r) const;
    std::string suitToString(Suit s) const;
    std::string handRankToString(HandRank hr) const;
    
private:
    std::vector<Card> cards;              // Deck of remaining cards
    std::vector<Card> communityCards;     // Community cards (flop, turn, river)
    std::random_device rd;
    std::mt19937 gen;                     // Random number generator for shuffling
    
    // Helper functions for hand evaluation
    std::map<Rank, int> countRanks(const std::vector<Card>& hand) const;
    std::map<Suit, int> countSuits(const std::vector<Card>& hand) const;
    bool isFlush(const std::vector<Card>& hand) const;
    bool isStraight(const std::vector<Card>& hand) const;
};

#endif // POKER_H