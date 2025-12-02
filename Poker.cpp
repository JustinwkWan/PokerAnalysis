#include "Poker.h"

Poker::Poker()
{
  // Create all 52 cards in the deck
  for (int r = static_cast<int>(Rank::Two); r <= static_cast<int>(Rank::Ace); ++r)
  {
    for (int s = static_cast<int>(Suit::Hearts); s <= static_cast<int>(Suit::Spades); ++s)
    {
      cards.emplace_back(static_cast<Rank>(r), static_cast<Suit>(s));
    }
  }
}

Poker::~Poker()
{
}


void Poker::dealCards() {
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> randomInt(0,100);    // set up the range
    int dropVal = randomInt(gen);  // pick a random value between 0 and 100
}