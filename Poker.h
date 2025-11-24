
#define POKER_H
#ifdef POKER_H


#include <vector>

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
    FourOfAKing,
    StraightFlush,
    RoyalFlush
};

class Poker {
  private: 
    std::vector<Rank> ranks;
    std::vector<Suit> suits;
  public: 
    Poker();
    ~Poker();
    HandRank evaluateHand(const std::vector<Rank>& handRanks, const std::vector<Suit>& handSuits);
    
};

#endif