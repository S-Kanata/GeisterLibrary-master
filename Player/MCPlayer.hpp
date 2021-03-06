#include <string>
#include "random.hpp"
#include "player.hpp"
#include "simulator.hpp"


#ifndef PLAYOUT_COUNT
#define PLAYOUT_COUNT 100
#endif

#define SIMULATOR Simulator


class MCPlayer: public Player{
    cpprefjp::random_device rd;
    std::mt19937 mt;
public:
    MCPlayer(): mt(rd()){
    }

    virtual std::string decideRed(){
        cpprefjp::random_device rd;
        std::mt19937 mt(rd());
        std::cout << "This is not Kufu" << std::endl;

        std::uniform_int_distribution<int> serector(0, pattern.size() - 1);
        return pattern[serector(mt)];
    }

    virtual std::string decideHand(std::string_view res){
        game.setState(res);

        auto legalMoves = candidateHand();
        
        const std::array<Unit, 16>& units = game.allUnit();
        for(const Unit& u: units){
            if(u.color() == UnitColor::Blue){
                if(u.x() == 0 && u.y() == 0){
                    return Hand{u, Direction::West};
                }
                if(u.x() == 5 && u.y() == 0){
                    return Hand{u, Direction::East};
                }
            }
        }
        
        SIMULATOR s0(game);
        std::vector<double> rewards(legalMoves.size(), 0.0);
        
        for(int l = 0; l < legalMoves.size(); ++l){
            auto m = legalMoves[l];
            SIMULATOR s(game);
            s.root.move(m);
            rewards[l] += s.run_WithNotCheckEscape(100);
        }

        auto&& max_iter = std::max_element(rewards.begin(), rewards.end());
        int index = std::distance(rewards.begin(), max_iter);

        std::cout << "NotKufu's Max Reward is " << rewards[index] << std::endl;
        return legalMoves[index];

        /*
        std::uniform_int_distribution<int> serector1(0, legalMoves.size() - 1);
        auto action = legalMoves[serector1(mt) % legalMoves.size()];
        return action;
        */
    }

    virtual std::vector<Hand> candidateHand(){
        return game.getLegalMove1st();
    }
};