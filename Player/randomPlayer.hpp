#include <string>
#include "random.hpp"
#include "player.hpp"
#include "Actionprob.hpp"


class RandomPlayer: public Player{
    cpprefjp::random_device rd;
    std::mt19937 mt;
public:
    RandomPlayer(): mt(rd()){
    }

    virtual std::string decideRed(){
        cpprefjp::random_device rd;
        std::mt19937 mt(rd());

        std::uniform_int_distribution<int> serector(0, pattern.size() - 1);
        return pattern[serector(mt)];
    }

    virtual std::string decideHand(std::string_view res){
        game.setState(res);
        Hand hand;
        auto canEscape = CanEscape(game);
        if(canEscape > 0){
            return EscapeHand(game, canEscape);
        } 

        auto legalMoves = candidateHand();
        std::uniform_int_distribution<int> serector1(0, legalMoves.size() - 1);
        auto action = legalMoves[serector1(mt) % legalMoves.size()];
        return action;
    }

    virtual std::vector<Hand> candidateHand(){
        return game.getLegalMove1st();
    }
};