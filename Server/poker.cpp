#include "poker.h"
#include <algorithm>
#include <stdexcept>
#include <sstream>

// Constructor - Initialize deck and random number generator
Poker::Poker() : gen(rd()) {
    resetDeck();
}

// Destructor
Poker::~Poker() {
    // Default destructor
}

// Reset and shuffle the deck for a new round
void Poker::resetDeck() {
    cards.clear();
    communityCards.clear();
    
    // Create all 52 cards in the deck
    for(int r = static_cast<int>(Rank::Two); r <= static_cast<int>(Rank::Ace); ++r) {
        for(int s = static_cast<int>(Suit::Hearts); s <= static_cast<int>(Suit::Spades); ++s) {
            cards.emplace_back(static_cast<Rank>(r), static_cast<Suit>(s));
        }
    }
    
    // Shuffle the deck
    std::shuffle(cards.begin(), cards.end(), gen);
}

// Draw a single card from the deck
Card Poker::drawCard() {
    if(cards.empty()) {
        throw std::runtime_error("No cards left in deck!");
    }
    
    Card drawnCard = cards.back();
    cards.pop_back();
    return drawnCard;
}

// Deal specified number of cards (typically 2 for Texas Hold'em)
std::vector<Card> Poker::dealCards(int numCards) {
    std::vector<Card> hand;
    for(int i = 0; i < numCards; ++i) {
        hand.push_back(drawCard());
    }
    return hand;
}

// Deal the flop (3 community cards)
void Poker::dealFlop() {
    // Burn one card (standard poker practice)
    if(!cards.empty()) {
        drawCard();
    }
    
    // Deal 3 community cards
    for(int i = 0; i < 3; ++i) {
        communityCards.push_back(drawCard());
    }
}

// Deal the turn (4th community card)
void Poker::dealTurn() {
    // Burn one card
    if(!cards.empty()) {
        drawCard();
    }
    
    // Deal 1 community card
    communityCards.push_back(drawCard());
}

// Deal the river (5th community card)
void Poker::dealRiver() {
    // Burn one card
    if(!cards.empty()) {
        drawCard();
    }
    
    // Deal 1 community card
    communityCards.push_back(drawCard());
}

// Get current community cards
std::vector<Card> Poker::getCommunityCards() const {
    return communityCards;
}

// Helper function to count rank occurrences
std::map<Rank, int> Poker::countRanks(const std::vector<Card>& hand) const {
    std::map<Rank, int> rankCounts;
    for(const auto& card : hand) {
        rankCounts[card.rank]++;
    }
    return rankCounts;
}

// Helper function to count suit occurrences
std::map<Suit, int> Poker::countSuits(const std::vector<Card>& hand) const {
    std::map<Suit, int> suitCounts;
    for(const auto& card : hand) {
        suitCounts[card.suit]++;
    }
    return suitCounts;
}

// Check if hand contains a flush (5+ cards of same suit)
bool Poker::isFlush(const std::vector<Card>& hand) const {
    auto suitCounts = countSuits(hand);
    for(const auto& pair : suitCounts) {
        if(pair.second >= 5) {
            return true;
        }
    }
    return false;
}

// Check if hand contains a straight (5 consecutive ranks)
bool Poker::isStraight(const std::vector<Card>& hand) const {
    std::vector<int> ranks;
    for(const auto& card : hand) {
        ranks.push_back(static_cast<int>(card.rank));
    }
    
    // Remove duplicates and sort
    std::sort(ranks.begin(), ranks.end());
    ranks.erase(std::unique(ranks.begin(), ranks.end()), ranks.end());
    
    // Check for 5 consecutive ranks
    int consecutive = 1;
    for(size_t i = 1; i < ranks.size(); ++i) {
        if(ranks[i] == ranks[i-1] + 1) {
            consecutive++;
            if(consecutive >= 5) {
                return true;
            }
        } else {
            consecutive = 1;
        }
    }
    
    // Check for Ace-low straight (A-2-3-4-5)
    if(ranks.size() >= 5) {
        bool hasAce = (ranks.back() == 14);
        bool has2 = (ranks[0] == 2);
        bool has3 = (ranks.size() > 1 && ranks[1] == 3);
        bool has4 = (ranks.size() > 2 && ranks[2] == 4);
        bool has5 = (ranks.size() > 3 && ranks[3] == 5);
        
        if(hasAce && has2 && has3 && has4 && has5) {
            return true;
        }
    }
    
    return false;
}

// Evaluate a poker hand (player's hole cards + community cards)
HandRank Poker::evaluateHand(const std::vector<Card>& holeCards) const {
    // Combine hole cards with community cards
    std::vector<Card> allCards = holeCards;
    allCards.insert(allCards.end(), communityCards.begin(), communityCards.end());
    
    if(allCards.size() < 5) {
        // Not enough cards to evaluate
        return HandRank::HighCard;
    }
    
    // Count ranks
    auto rankCounts = countRanks(allCards);
    
    // Find pairs, trips, quads
    int pairs = 0;
    int trips = 0;
    int quads = 0;
    
    for(const auto& pair : rankCounts) {
        if(pair.second == 4) {
            quads++;
        } else if(pair.second == 3) {
            trips++;
        } else if(pair.second == 2) {
            pairs++;
        }
    }
    
    bool flush = isFlush(allCards);
    bool straight = isStraight(allCards);
    
    // Determine hand rank
    if(straight && flush) {
        // Check for royal flush (10-J-Q-K-A of same suit)
        bool hasRoyal = false;
        for(const auto& card : allCards) {
            if(card.rank == Rank::Ace) {
                hasRoyal = true;
                break;
            }
        }
        // Simplified check - just return straight flush for now
        return HandRank::StraightFlush;
    }
    
    if(quads > 0) {
        return HandRank::FourOfAKind;
    }
    
    if(trips > 0 && pairs > 0) {
        return HandRank::FullHouse;
    }
    
    if(flush) {
        return HandRank::Flush;
    }
    
    if(straight) {
        return HandRank::Straight;
    }
    
    if(trips > 0) {
        return HandRank::ThreeOfAKind;
    }
    
    if(pairs >= 2) {
        return HandRank::TwoPair;
    }
    
    if(pairs == 1) {
        return HandRank::OnePair;
    }
    
    return HandRank::HighCard;
}

// Compare two hand values
bool Poker::HandValue::operator>(const HandValue& other) const {
    // First compare hand rank
    if(rank != other.rank) {
        return rank > other.rank;
    }
    
    // Same rank, compare values (kickers)
    for(size_t i = 0; i < std::min(values.size(), other.values.size()); i++) {
        if(values[i] != other.values[i]) {
            return values[i] > other.values[i];
        }
    }
    
    // All values equal - true tie
    return false;
}

bool Poker::HandValue::operator==(const HandValue& other) const {
    if(rank != other.rank) return false;
    if(values.size() != other.values.size()) return false;
    
    for(size_t i = 0; i < values.size(); i++) {
        if(values[i] != other.values[i]) return false;
    }
    
    return true;
}

// Detailed hand evaluation with tie-breaking values
Poker::HandValue Poker::evaluateHandDetailed(const std::vector<Card>& holeCards) const {
    HandValue result;
    
    // Combine hole cards with community cards
    std::vector<Card> allCards = holeCards;
    allCards.insert(allCards.end(), communityCards.begin(), communityCards.end());
    
    if(allCards.size() < 5) {
        result.rank = HandRank::HighCard;
        return result;
    }
    
    // Count ranks
    auto rankCounts = countRanks(allCards);
    
    // Find pairs, trips, quads
    std::vector<int> quads;
    std::vector<int> trips;
    std::vector<int> pairs;
    std::vector<int> singles;
    
    for(const auto& pair : rankCounts) {
        int rankValue = static_cast<int>(pair.first);
        if(pair.second == 4) {
            quads.push_back(rankValue);
        } else if(pair.second == 3) {
            trips.push_back(rankValue);
        } else if(pair.second == 2) {
            pairs.push_back(rankValue);
        } else if(pair.second == 1) {
            singles.push_back(rankValue);
        }
    }
    
    // Sort in descending order for tie-breaking
    std::sort(quads.rbegin(), quads.rend());
    std::sort(trips.rbegin(), trips.rend());
    std::sort(pairs.rbegin(), pairs.rend());
    std::sort(singles.rbegin(), singles.rend());
    
    bool flush = isFlush(allCards);
    bool straight = isStraight(allCards);
    
    // Determine hand rank and values for comparison
    if(straight && flush) {
        result.rank = HandRank::StraightFlush;
        // Get highest card in straight for comparison
        std::vector<int> ranks;
        for(const auto& card : allCards) {
            ranks.push_back(static_cast<int>(card.rank));
        }
        std::sort(ranks.rbegin(), ranks.rend());
        result.values.push_back(ranks[0]);  // High card of straight
    }
    else if(!quads.empty()) {
        result.rank = HandRank::FourOfAKind;
        result.values.push_back(quads[0]);  // Quad rank
        // Add best kicker
        if(!singles.empty()) result.values.push_back(singles[0]);
        else if(!trips.empty()) result.values.push_back(trips[0]);
        else if(!pairs.empty()) result.values.push_back(pairs[0]);
    }
    else if(!trips.empty() && !pairs.empty()) {
        result.rank = HandRank::FullHouse;
        result.values.push_back(trips[0]);  // Trip rank
        result.values.push_back(pairs[0]);   // Pair rank
    }
    else if(flush) {
        result.rank = HandRank::Flush;
        // Add all 5 highest cards for comparison
        std::vector<int> ranks;
        for(const auto& card : allCards) {
            ranks.push_back(static_cast<int>(card.rank));
        }
        std::sort(ranks.rbegin(), ranks.rend());
        for(int i = 0; i < 5 && i < (int)ranks.size(); i++) {
            result.values.push_back(ranks[i]);
        }
    }
    else if(straight) {
        result.rank = HandRank::Straight;
        // Get highest card in straight
        std::vector<int> ranks;
        for(const auto& card : allCards) {
            ranks.push_back(static_cast<int>(card.rank));
        }
        std::sort(ranks.rbegin(), ranks.rend());
        result.values.push_back(ranks[0]);
    }
    else if(!trips.empty()) {
        result.rank = HandRank::ThreeOfAKind;
        result.values.push_back(trips[0]);  // Trip rank
        // Add top 2 kickers
        int kickersAdded = 0;
        for(int s : singles) {
            if(kickersAdded >= 2) break;
            result.values.push_back(s);
            kickersAdded++;
        }
    }
    else if(pairs.size() >= 2) {
        result.rank = HandRank::TwoPair;
        result.values.push_back(pairs[0]);  // Higher pair
        result.values.push_back(pairs[1]);  // Lower pair
        // Add best kicker
        if(!singles.empty()) result.values.push_back(singles[0]);
        else if(pairs.size() > 2) result.values.push_back(pairs[2]);
    }
    else if(pairs.size() == 1) {
        result.rank = HandRank::OnePair;
        result.values.push_back(pairs[0]);  // Pair rank
        // Add top 3 kickers
        int kickersAdded = 0;
        for(int s : singles) {
            if(kickersAdded >= 3) break;
            result.values.push_back(s);
            kickersAdded++;
        }
    }
    else {
        result.rank = HandRank::HighCard;
        // Add top 5 cards
        int cardsAdded = 0;
        for(int s : singles) {
            if(cardsAdded >= 5) break;
            result.values.push_back(s);
            cardsAdded++;
        }
    }
    
    return result;
}

// Determine winners among multiple players (returns winner indices - can be multiple for ties!)
std::vector<int> Poker::determineWinners(const std::vector<std::vector<Card>>& playerHands) const {
    std::vector<int> winners;
    
    if(playerHands.empty()) {
        return winners;
    }
    
    // Evaluate all hands
    std::vector<HandValue> handValues;
    for(const auto& hand : playerHands) {
        handValues.push_back(evaluateHandDetailed(hand));
    }
    
    // Find the best hand
    HandValue bestHand = handValues[0];
    winners.push_back(0);
    
    for(size_t i = 1; i < handValues.size(); i++) {
        if(handValues[i] > bestHand) {
            // New best hand found
            bestHand = handValues[i];
            winners.clear();
            winners.push_back(i);
        } else if(handValues[i] == bestHand) {
            // Tie - multiple winners!
            winners.push_back(i);
        }
    }
    
    return winners;
}

// Handle blinds (small blind and big blind)
void Poker::handleBlinds() {
    // This would typically be handled by the game room/server
    // Placeholder for now
}

// Get game state as a string (for debugging/display)
std::string Poker::getGameState() const {
    std::stringstream ss;
    ss << "Cards in deck: " << cards.size() << "\n";
    ss << "Community cards: " << communityCards.size() << "\n";
    
    if(!communityCards.empty()) {
        ss << "Community: ";
        for(const auto& card : communityCards) {
            ss << rankToString(card.rank) << suitToString(card.suit) << " ";
        }
        ss << "\n";
    }
    
    return ss.str();
}

// Helper function to convert rank to string
std::string Poker::rankToString(Rank r) const {
    switch(r) {
        case Rank::Two: return "2";
        case Rank::Three: return "3";
        case Rank::Four: return "4";
        case Rank::Five: return "5";
        case Rank::Six: return "6";
        case Rank::Seven: return "7";
        case Rank::Eight: return "8";
        case Rank::Nine: return "9";
        case Rank::Ten: return "10";
        case Rank::Jack: return "J";
        case Rank::Queen: return "Q";
        case Rank::King: return "K";
        case Rank::Ace: return "A";
        default: return "?";
    }
}

// Helper function to convert suit to string
std::string Poker::suitToString(Suit s) const {
    switch(s) {
        case Suit::Hearts: return "♥";
        case Suit::Diamonds: return "♦";
        case Suit::Clubs: return "♣";
        case Suit::Spades: return "♠";
        default: return "?";
    }
}

// Helper function to convert hand rank to string
std::string Poker::handRankToString(HandRank hr) const {
    switch(hr) {
        case HandRank::HighCard: return "High Card";
        case HandRank::OnePair: return "One Pair";
        case HandRank::TwoPair: return "Two Pair";
        case HandRank::ThreeOfAKind: return "Three of a Kind";
        case HandRank::Straight: return "Straight";
        case HandRank::Flush: return "Flush";
        case HandRank::FullHouse: return "Full House";
        case HandRank::FourOfAKind: return "Four of a Kind";
        case HandRank::StraightFlush: return "Straight Flush";
        case HandRank::RoyalFlush: return "Royal Flush";
        default: return "Unknown";
    }
}