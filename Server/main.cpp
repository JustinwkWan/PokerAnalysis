#include "poker.h"
#include <iostream>
#include <vector>

int main() {
    std::cout << "=== Poker Game Test ===\n\n";
    
    // Create poker game
    Poker game;
    
    // Test 1: Deal cards to 3 players
    std::cout << "Test 1: Dealing cards to 3 players\n";
    std::vector<std::vector<Card>> playerHands;
    
    for(int i = 0; i < 3; i++) {
        auto hand = game.dealCards(2);
        playerHands.push_back(hand);
        
        std::cout << "Player " << (i+1) << ": ";
        for(const auto& card : hand) {
            std::cout << game.rankToString(card.rank) 
                     << game.suitToString(card.suit) << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
    
    // Test 2: Deal flop
    std::cout << "Test 2: Dealing the flop\n";
    game.dealFlop();
    auto community = game.getCommunityCards();
    std::cout << "Flop: ";
    for(const auto& card : community) {
        std::cout << game.rankToString(card.rank) 
                 << game.suitToString(card.suit) << " ";
    }
    std::cout << "\n\n";
    
    // Test 3: Deal turn
    std::cout << "Test 3: Dealing the turn\n";
    game.dealTurn();
    community = game.getCommunityCards();
    std::cout << "Board: ";
    for(const auto& card : community) {
        std::cout << game.rankToString(card.rank) 
                 << game.suitToString(card.suit) << " ";
    }
    std::cout << "\n\n";
    
    // Test 4: Deal river
    std::cout << "Test 4: Dealing the river\n";
    game.dealRiver();
    community = game.getCommunityCards();
    std::cout << "Board: ";
    for(const auto& card : community) {
        std::cout << game.rankToString(card.rank) 
                 << game.suitToString(card.suit) << " ";
    }
    std::cout << "\n\n";
    
    // Test 5: Evaluate hands
    std::cout << "Test 5: Evaluating hands\n";
    for(size_t i = 0; i < playerHands.size(); i++) {
        HandRank rank = game.evaluateHand(playerHands[i]);
        std::cout << "Player " << (i+1) << " has: " 
                 << game.handRankToString(rank) << "\n";
    }
    std::cout << "\n";
    
    // Test 6: Determine winner
    std::cout << "Test 6: Determining winner\n";
    int winner = game.determineWinner(playerHands);
    std::cout << "Winner is Player " << (winner + 1) << "!\n\n";
    
    // Test 7: Reset deck
    std::cout << "Test 7: Resetting deck for new round\n";
    game.resetDeck();
    std::cout << game.getGameState();
    std::cout << "Deck reset successfully!\n\n";
    
    // Test 8: New round
    std::cout << "Test 8: Starting new round\n";
    auto newHand = game.dealCards(2);
    std::cout << "New hand: ";
    for(const auto& card : newHand) {
        std::cout << game.rankToString(card.rank) 
                 << game.suitToString(card.suit) << " ";
    }
    std::cout << "\n\n";
    
    std::cout << "=== All tests completed! ===\n";
    
    return 0;
}