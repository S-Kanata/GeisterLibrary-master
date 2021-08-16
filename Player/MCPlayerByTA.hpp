#include <string>
#include "random.hpp"
#include "player.hpp"
#include "simulator.hpp"

class MCPlayerByTA: public Player{
    cpprefjp::random_device rd;
    std::mt19937 mt;
public:
    MCPlayerByTA(): mt(rd()){
    }

    virtual std::string decideRed(){
        cpprefjp::random_device rd;
        std::mt19937 mt(rd());

        std::uniform_int_distribution<int> serector(0, pattern.size() - 1);
        return pattern[serector(mt)];
    }

    virtual std::string decideHand(std::string_view res){
        game.setState(res);

        auto legalMoves = candidateHand();
        for( auto move : legalMoves) {
            Unit u = move.unit;
            if(u.isBlue() && (u.x() == 0 && u.y() == 0) ) { return Hand{u, 'W'}; }
            if(u.isBlue() && (u.x() == 5 && u.y() == 0) ) { return Hand{u, 'E'}; }
        }

        std::vector<Simulator> simulators;
        for( auto m : legalMoves ) {
            Simulator s{game};
            s.root.move(m);
            simulators.push_back(s);
        }

        std::vector<double> rewards(legalMoves.size(), 0.0);
        for(int i = 0; i < legalMoves.size(); i++) {
            rewards[i] += simulators[i].run(100);
        }

        int index_max = 0;
        for (int i = 1; i < legalMoves.size(); i++) {
            if(rewards[index_max] < rewards[i]) index_max = i;
        }

        return legalMoves[index_max];
    }

    virtual std::vector<Hand> candidateHand(){
        return game.getLegalMove1st();
    }
};