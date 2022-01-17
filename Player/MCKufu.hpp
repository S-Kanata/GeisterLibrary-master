#include <string>
#include "random.hpp"
#include "player.hpp"
#include "simulator.hpp"


#ifndef PLAYOUT_COUNT
#define PLAYOUT_COUNT 100
#endif

#define SIMULATOR Simulator


class MCKufu: public Player{
    cpprefjp::random_device rd;
    std::mt19937 mt;
public:
    MCKufu(): mt(rd()){
    }

    std::vector<int> EnemyColor; //敵駒色の推定
    std::vector<int> EstimatedRed; //敵駒色の推定
    std::vector<std::vector<int>> AfterPosition; //推定した駒の移動先

    virtual std::string decideRed(){
        cpprefjp::random_device rd;
        std::mt19937 mt(rd());
        std::cout << "This is Kufu" << std::endl;
        
        EnemyColor = std::vector<int>(16);
        EstimatedRed = std::vector<int>(16);
        AfterPosition = std::vector<std::vector<int>>(16, std::vector<int>(37));

        std::uniform_int_distribution<int> serector(0, pattern.size() - 1);
        return pattern[serector(mt)];
    }

    std::vector<std::string> getLegalPattern(Geister game){
        std::vector<std::string> res;
        std::vector<char> blue;
        std::vector<char> red;
        std::vector<char> unknown;
        // 判明している色ごとにリスト化
        for(int u = 8; u < 16; u++){
            if(game.allUnit()[u].color() == UnitColor::blue)
                blue.push_back(u - 8 + 'A');
            else if(game.allUnit()[u].color() == UnitColor::red)
                red.push_back(u - 8 + 'A');
            else
                unknown.push_back(u - 8 + 'A');
        }

        for(int i = 0; i < red.size(); i++){
            std::cout << red[i] << std::endl;
        }

        for(int i = 0; i < blue.size(); i++){
            std::cout << blue[i] << std::endl;
        }

        for(int i = 0; i < unknown.size(); i++){
            std::cout << unknown[i] << std::endl;
        }
        game.printBoard();
        
        // 判明している情報と矛盾するパターンを除外
        for(auto p: pattern){
            // 青と分かっている駒を含むパターンを除外
            int checkB = 0;
            for(auto b: blue){
                if(std::string(p).find(b) != std::string::npos){
                    checkB++;
                    break;
                }
            }
            if(checkB != 0) continue;
            // 赤と分かっている駒を含むパターンを除外
            int checkR = 0;
            for(auto r: red){
                if(std::string(p).find(r) == std::string::npos){
                    checkR++;
                    break;
                }
            }
            if(checkR != 0) continue;
            res.push_back(std::string(p));
        }
        return res;
    }

    virtual std::string decideHand(std::string_view res){
        game.setState(res);

        //脱出可能な手があれば脱出
        int canescape = CheckCanEscape(game);
        if(canescape > 0){
            Hand escapehand = Escape(game, canescape);
            printf("必勝手が存在\n");
            fflush(stdout);
            return escapehand;
        }

        //脱出をしてこなかった相手駒を赤駒と断定
        auto enemy = game;
        enemy.changeSide();
        auto units = enemy.allUnit();
        bool IsNoRed = true;
        for(int i = 0; i < 16; i++){
            if(EstimatedRed[i] == 1){
                int pos = units[i].x() + units[i].y() * 6;
                if(pos < 37){
                    if(!AfterPosition[i][pos]==1)
                    {
                        EnemyColor[i] = 1;
                        IsNoRed = false;
                        auto u = game.allUnit()[i+8];
                        printf("Discovered red!!\n position, %d, %d\n", u.x(), u.y());
                    }
                }
            }
        }

        //見積もりが正しくなければリセット
        for(int i = 0; i < 16; i++){
            EstimatedRed[i] = 0;
            for(int j = 0; j < 37; j++){
                AfterPosition[i][j] = 0;
            }
        }


        for(int i = 0; i < 16; i++){
            if(EnemyColor[i]==1){
                if(game.allUnit()[i+8].color() == UnitColor::unknown){
                    game.setColor(i+8, UnitColor::red);
                    auto enemygame = game;
                    /*
                    auto legalPattern = getLegalPattern(enemygame);
                    for(int i = 0; i < legalPattern.size(); i++){
                        std::cout << legalPattern[i] << std::endl;
                    }
                    std::cout << legalPattern.size() << std::endl;
                    */
                }
            }
        }

        auto legalMoves = candidateHand();
    
        SIMULATOR s0(game);
        int size = legalMoves.size();

        std::vector<double> rewards(size, 0.0);
        int pc = 1000/size+0.9999;
        
        for(int l = 0; l < size; ++l){
            auto m = legalMoves[l];
            SIMULATOR s(game);
            s.root.move(m);
            rewards[l] += s.run_WithPlob(pc);
        }

        auto&& max_iter = std::max_element(rewards.begin(), rewards.end());
        int index = std::distance(rewards.begin(), max_iter);
        
        std::cout << "Kufu's Max Reward is " << rewards[index] << std::endl;

        
        auto enemyboard = game;
        enemyboard.move(legalMoves[index]);
        fflush(stdout);
        enemyboard.changeSide();

        for(int i = 0; i < 16; i++){
            if(enemyboard.IsExistUnit(0,0) == 3){
                if ((units[i].x()== 0)&&(units[i].y() == 0)){
                    enemyboard.setColor(i, UnitColor::Blue);
                }
            }

            if(enemyboard.IsExistUnit(5,0) == 3){
                if ((units[i].x()== 5)&&(units[i].y() == 0)){
                    enemyboard.setColor(i, UnitColor::Blue);
                }
            }

            if(enemyboard.IsExistUnit(1,0) == 3){
                if ((units[i].x()== 1)&&(units[i].y() == 0)){
                    enemyboard.setColor(i, UnitColor::Blue);
                }
            }

            if(enemyboard.IsExistUnit(0,1) == 3){
                if ((units[i].x()== 0)&&(units[i].y() == 1)){
                    enemyboard.setColor(i, UnitColor::Blue);
                }
            }

            if(enemyboard.IsExistUnit(4,0) == 3){
                if ((units[i].x()== 4)&&(units[i].y() == 0)){
                    enemyboard.setColor(i, UnitColor::Blue);
                }
            }

            if(enemyboard.IsExistUnit(5,1) == 3){
                if ((units[i].x()== 5)&&(units[i].y() == 1)){
                    enemyboard.setColor(i, UnitColor::Blue);
                }
            }
        }

        units = enemyboard.allUnit();
        int canescapeenemy = CheckCanEscape(enemyboard);
        //std::cout << canescapeenemy << std::endl;

        if(canescapeenemy == 1){
            for(int i = 0; i < 16; i++){
                if ((units[i].x() == 0)&&(units[i].y() == 0)){
                    if (units[i].color() == UnitColor::Blue){
                        EstimatedRed[i] = 1;
                        AfterPosition[i][36] = 1;
                        printf("%d,%dに赤駒らしき駒を発見\n", 5-units[i].x(), 5-units[i].y());
                    }
                }
                if ((units[i].x() == 5)&&(units[i].y() == 0)){
                    if (units[i].color() == UnitColor::Blue){
                        EstimatedRed[i] = 1;
                        AfterPosition[i][36] = 1;
                        printf("%d,%dに赤駒らしき駒を発見\n", 5-units[i].x(), 5-units[i].y());
                    }

                }
            }
        }

        if(canescapeenemy == 3){
            for(int i = 0; i < 16; i++){
                if ((units[i].x() == 0)&&(units[i].y() == 1)){
                    if (units[i].color() == UnitColor::Blue){
                        EstimatedRed[i] = 1;
                        AfterPosition[i][0] = 1;
                        printf("%d,%dに赤駒らしき駒を発見\n", 5-units[i].x(), 5-units[i].y());
                    }
                }
                if ((units[i].x() == 1)&&(units[i].y() == 0)){
                    if (units[i].color() == UnitColor::Blue){
                        EstimatedRed[i] = 1;
                        AfterPosition[i][0] = 1;
                        printf("%d,%dに赤駒らしき駒を発見\n", 5-units[i].x(), 5-units[i].y());
                    }
                }
                if ((units[i].x() == 5)&&(units[i].y() == 1)){
                    if (units[i].color() == UnitColor::Blue){
                        EstimatedRed[i] = 1;
                        AfterPosition[i][5] = 1;
                        printf("%d,%dに赤駒らしき駒を発見\n", 5-units[i].x(), 5-units[i].y());
                    }
                }
                if ((units[i].x() == 4)&&(units[i].y() == 0)){
                    if (units[i].color() == UnitColor::Blue){
                        EstimatedRed[i] = 1;
                        AfterPosition[i][5] = 1;
                        printf("%d,%dに赤駒らしき駒を発見\n", 5-units[i].x(), 5-units[i].y());
                    }
                }
            }
        }


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

    //脱出可能かどうかチェック
    int CheckCanEscape(Geister currentGame){
        auto legalMoves = currentGame.getLegalMove1st();
        //出口との距離が0の場合
        if (currentGame.IsExistUnit(0, 0) == 1){
            //printf("脱出可能なコマがあります\n");
            return 1;
        }
        if (currentGame.IsExistUnit(5, 0) == 1){
            //printf("脱出可能なコマがあります\n");
            return 1;
        }

        //出口との距離が1の場合
        if ((((currentGame.IsExistUnit(5, 5) < 4)||(currentGame.IsExistUnit(5, 5) == 6)) && ((currentGame.IsExistUnit(0, 5) < 4)||(currentGame.IsExistUnit(0, 5) == 6)))){
            if (!(currentGame.takenCount(UnitColor::red) == 3)){
                if ((currentGame.IsExistUnit(0, 1) == 1) && (currentGame.IsExistUnit(1, 0) < 4)){
                    return 3;
                }
                if ((currentGame.IsExistUnit(1, 0) == 1) && (currentGame.IsExistUnit(0, 1) < 4)){
                    return 3;
                }
            }

            if (!(currentGame.IsExistUnit(5, 0) != 6 && currentGame.takenCount(UnitColor::red) == 3)){
                if ((currentGame.IsExistUnit(5, 1) == 1) && (currentGame.IsExistUnit(4, 0) < 4)){
                        return 3;
                }
                if ((currentGame.IsExistUnit(4, 0) == 1) && (currentGame.IsExistUnit(5, 1) < 4)){
                        return 3;
                }
            }
        }
        return 0;
    }

    Hand Escape(Geister currentGame, int depth){
        auto legalMoves = currentGame.getLegalMove1st();

        if(depth == 1){
            for( auto move : legalMoves) {
                Unit u = move.unit;
                if(u.isBlue() && (u.x() == 0 && u.y() == 0) ) { return Hand{u, 'W'}; }
                if(u.isBlue() && (u.x() == 5 && u.y() == 0) ) { return Hand{u, 'E'}; }
            }
        }

        if(depth == 3){
            for( auto move : legalMoves) {
                Unit u = move.unit;
                if(u.isBlue() && (u.x() == 0 && u.y() == 1) ) { return Hand{u, 'N'}; }
                if(u.isBlue() && (u.x() == 1 && u.y() == 0) ) { return Hand{u, 'W'}; }
                if(u.isBlue() && (u.x() == 5 && u.y() == 1) ) { return Hand{u, 'N'}; }
                if(u.isBlue() && (u.x() == 4 && u.y() == 0) ) { return Hand{u, 'E'}; }
            }
        }



    }
};