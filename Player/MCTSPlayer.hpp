#include "player.hpp"
#include "Simulator/all.hpp"
#include "random.hpp"
//#include "Encode.hpp"
//#include "Actionprob.hpp"
#include <vector>
#include <iostream>

constexpr int expandCount = 250;//PLAYOUT_COUNT * 0.1;
constexpr double alpha = 1.4;

#ifndef SIMULATOR
#define SIMULATOR Simulator0
#endif

class MCTNode: public SIMULATOR{
public:
    std::vector<MCTNode> children;
    int visitCount;
    static int totalCount;
    double reward;

    MCTNode(Geister g): SIMULATOR(g), visitCount(0), reward(0)
    {
    }

    MCTNode(Geister g, std::string ptn): SIMULATOR(g, ptn), visitCount(0), reward(0)
    {
    }

    MCTNode(const MCTNode& mctn): SIMULATOR(mctn.root), visitCount(0), reward(0)
    {
        children.clear();
    }

    double calcUCB(){
        return (reward / std::max(visitCount, 1))
            + (alpha * sqrt(log((double)totalCount) / std::max(visitCount, 1)));
    }

    void expand(){
        root.changeSide();
        root.printBoard();
        auto legalMoves = root.getLegalMove1st();
        for(auto&& lm: legalMoves){
            auto child = *this;
            child.root.move(lm);
            child.root.changeSide();
            children.push_back(child);
        }
        root.changeSide();
    }

    double select(){
        visitCount++;
        auto result = static_cast<int>(root.result());
        if(result != 0){
            totalCount++;
            reward += result;
            return -result;
        }
        double res;
        if(!children.empty()){
            std::vector<double> ucbs(children.size());
            std::transform(children.begin(), children.end(), ucbs.begin(), [](auto& x){return x.calcUCB();});
            auto&& max_iter = std::max_element(ucbs.begin(), ucbs.end());
            int index = std::distance(ucbs.begin(), max_iter);
            /*
            std::vector<int> indexes;
            for(int i = 0; i < ucbs.size(); i++){
                if(ucbs[index]==ucbs[i]){
                    
                    indexes.push_back(i);
                }
            }
            if(indexes.size() > 1){
                std::uniform_int_distribution<int> serector(0, indexes.size() - 1);
                res = children[serector(mt)].select();
            } else {
                */
            res = children[index].select();
            

            // int childIndex = 0;
            // double maxUCB = children[0].calcUCB();
            // for(int i = 1; i < children.size(); ++i){
            //     // std::cout << children[i].calcUCB() << "(" <<totalCount << ")" << "\t";
            //     if(double ucb = children[i].calcUCB(); maxUCB < ucb){
            //         maxUCB = ucb;
            //         childIndex = i;
            //     }
            // }
            // // std::cout << "\n";
            // res = children[childIndex].select();
        }
        // 訪問回数が規定数を超えていたら子ノードをたどる
        else if(visitCount >= expandCount){
            // root.printBoard();
            // 子ノードがなければ展開
            if(visitCount == expandCount){
                printf("expand");
                fflush(stdout);
                expand();
            }
            for(auto&& child: children){
            res += -child.select();
            }
        }
        // プレイアウトの実行
        else{
            totalCount++;
            res = run_WithPlob();
        }
        reward += res;
        return -res;
    }
};
int MCTNode::totalCount = 0;

#ifndef PLAYOUT_COUNT
#define PLAYOUT_COUNT 1000
#endif

class MCTSPlayer: public Player{
    int playoutCount = PLAYOUT_COUNT;
    cpprefjp::random_device rd;
    std::mt19937 mt;
public:
    MCTSPlayer(): mt(rd()){
    }

    std::vector<double> theta;   //行動パラメータθ
    std::vector<int> EnemyColor; //敵駒色の推定
    std::vector<int> EstimatedRed; //敵駒色の推定
    std::vector<std::vector<int>> AfterPosition; //推定した駒の移動先

    virtual std::string decideRed(){
        int pc = PLAYOUT_COUNT;
        EnemyColor = std::vector<int>(16);
        EstimatedRed = std::vector<int>(16);
        AfterPosition = std::vector<std::vector<int>>(16, std::vector<int>(37));
        printf("%d\n", pc);
        printf("%lf\n", alpha);
        fflush(stdout);
        std::uniform_int_distribution<int> serector(0, pattern.size() - 1);
        return pattern[serector(mt)];
    }

    virtual std::string decideHand(std::string_view res){
        printf("%s\n", res);
        fflush(stdout);
        game = Geister(res);
        MCTNode::totalCount = 0;
        return decideHand_Random();
    }

protected:
    // 可能性のあるすべての相手駒パターンを列挙
    std::vector<std::string> getLegalPattern(Geister game){
        std::vector<std::string> res;
        std::vector<char> blue;
        std::vector<char> red;
        std::vector<char> unknown;
        // 判明している色ごとにリスト化
        for(int u = 8; u < 16; ++u){
            if(game.allUnit()[u].color() == UnitColor::blue)
                blue.push_back(u - 8 + 'A');
            else if(game.allUnit()[u].color() == UnitColor::red)
                red.push_back(u - 8 + 'A');
            else
                unknown.push_back(u - 8 + 'A');
        }
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
            // 赤と分かっている駒を含まないパターンを除外
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

    std::string decideHand_Random(){
        
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
        for(int i = 0; i < 16; i++){
            if(EstimatedRed[i] == 1){
                int pos = units[i].x() + units[i].y() * 6;
                if(pos < 37){
                    if(!AfterPosition[i][pos]==1)
                    {
                        EnemyColor[i] = 1;
                    }
                }
            }
        }

        for(int i = 0; i < 16; i++){
            if(EnemyColor[i]==1){
                game.setColor(i+8, UnitColor::red);
                printf("Discovered red!!\n position, %d, %d\n", units[i].x(), units[i].y());
            }
        }
        
        // 合法手の列挙
        auto legalPattern = getLegalPattern(game);
        MCTNode Tree(game);
        auto legalMoves = Tree.root.getLegalMove1st();
        for(auto&& lm: legalMoves){
                auto child = Tree;
                child.root.move(lm);
                Tree.children.push_back(child);
        }
        
        for(int i = 0; i < playoutCount; ++i){        
                Tree.select();
                if(i%100 == 0){
                    printf("%d¥n", i);
                    fflush(stdout);
                }
        }

        auto lm = game.getLegalMove1st();
        std::vector<double> visits(lm.size());
        for(int i = 0; i < lm.size(); ++i){
                visits[i] += Tree.children[i].visitCount;
        }
        int index = 0;
        double maxVisit = visits[0];
        for(int i = 0; i < visits.size(); i++){
            double visit = visits[i];
            if( visit > maxVisit){
                maxVisit = visit;
                index = i;
            }
        }

        auto enemyboard = game;
        enemyboard.move(lm[index]);
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

        int canescapeenemy = CheckCanEscape(enemyboard);

        if(canescapeenemy == 1){
            for(int i = 0; i < 16; i++){
                if ((units[i].x() == 0)&&(units[i].y() == 0)){
                    EstimatedRed[i] = 1;
                    AfterPosition[i][36] = 1;
                }
                if ((units[i].x() == 5)&&(units[i].y() == 0)){
                    EstimatedRed[i] = 1;
                    AfterPosition[i][36] = 1;
                }
            }
        }

        if(canescapeenemy == 3){
            for(int i = 0; i < 16; i++){
                if ((units[i].x() == 0)&&(units[i].y() == 1)){
                    EstimatedRed[i] = 1;
                    AfterPosition[i][0] = 1;
                }
                if ((units[i].x() == 1)&&(units[i].y() == 0)){
                    EstimatedRed[i] = 1;
                    AfterPosition[i][0] = 1;
                }
                if ((units[i].x() == 5)&&(units[i].y() == 1)){
                    EstimatedRed[i] = 1;
                    AfterPosition[i][5] = 1;
                }
                if ((units[i].x() == 4)&&(units[i].y() == 0)){
                    EstimatedRed[i] = 1;
                    AfterPosition[i][5] = 1;
                }
            }
        }
        return lm[index];
    }

    std::string decideHand_Average(){
        auto legalMoves = game.getLegalMove1st();
        auto patterns = getLegalPattern(game);
        const int playout = std::max(static_cast<int>(playoutCount / patterns.size()), 1);
        std::vector<MCTNode> trees;
        for(auto&& p: patterns){
            trees.emplace_back(game, p);
        }

        for(auto&& tree:trees){
            MCTNode::totalCount = 0;
            auto legalMoves = tree.root.getLegalMove1st();
            for(auto&& lm: legalMoves){
                auto child = tree;
                child.root.move(lm);
                tree.children.emplace_back(child);
            }
            for(int i = 0; i < playout; ++i){
                tree.select();
            }
        }

        std::vector<double> visits(game.getLegalMove1st().size());
        // std::vector<double> rewards(game.getLegalMove1st().size());
        // for(auto&& tree:trees){
        //     tree.root.printBoard();
        // }
        // std::cout << "\t";
        // for(auto&& lm: game.getLegalMove1st()){
        //     std::cout << lm << "\t";
        // }
        // std::cout << "\n";
        for(size_t t = 0; t < trees.size(); ++t){
            auto& tree = trees[t];
            // std::cout << patterns[t] << "\t";
            for(int i = 0; i < tree.children.size(); ++i){
                visits[i] += tree.children[i].visitCount;
                // rewards[i] += tree.children[i].reward;
                // tree.children[i].root.printBoard();
                // std::cout << tree.children[i].reward << "/" << tree.children[i].visitCount << "\t";
            }
            // std::cout << "\n";
        }
        // std::cout << "\t";
        // for(size_t i = 0; i < visits.size(); ++i){
        //     std::cout << rewards[i] << "/" << visits[i] << "\t";
        // }
        // std::cout << "\n";

        int index = std::distance(visits.begin(), std::max_element(visits.begin(), visits.end()));
        // int index = std::distance(rewards.begin(), std::max_element(rewards.begin(), rewards.end()));
        return legalMoves[index];
    }

    std::string decideHand_MinMax(){
        auto legalMoves = game.getLegalMove1st();
        auto legalPattern = getLegalPattern(game);
        const int playout = std::max(static_cast<int>(playoutCount / legalPattern.size()), 1);
        std::vector<MCTNode> trees;
        for(auto&& p: legalPattern){
            trees.push_back(MCTNode(game, p));
        }

        // int x = 0;
        // std::cout << "\t";
        // for(int i = 0; i < legalMoves.size(); ++i){
        //     std::cout << legalMoves[i] << "\t";
        // }
        // std::cout << std::endl;
        
        for(auto&& tree:trees){
            auto treeLegalMoves = tree.root.getLegalMove1st();
            for(auto&& lm: treeLegalMoves){
                auto child = tree;
                child.root.move(lm);
                tree.children.push_back(child);
            }
            for(int i = 0; i < playout*treeLegalMoves.size(); ++i){
                tree.select();
            }
            
            // std::cout << legalPattern[x] << "\t";
            // for(int i = 0; i < legalMoves.size(); ++i){
            //     std::cout << tree.children[i].visitCount << "\t";
            // }
            // std::cout << std::endl;
            // x++;
        }

        std::vector<double> visits(legalMoves.size());
        // std::vector<double> rewards(game.getLegalMove1st().size());
        for(int i = 0; i < legalMoves.size(); ++i){
            visits[i] = trees[0].children[i].visitCount;
            // rewards[i] = trees[0].children[i].reward;
            for(int t = 1; t < trees.size(); ++t){
                auto visit = trees[t].children[i].visitCount;
                if( visit < visits[i]){
                    visits[i] = visit;
                }
                // if(auto reward = trees[t].children[i].reward; reward < rewards[i]){
                //     rewards[i] = reward;
                // }
            }
        }
        // int index = 0;
        // double maxVisit = visits[0];
        // double maxReward = rewards[0];
        // for(int i = 1; i < visits.size(); ++i){
        //     if(double visit = visits[i]; visit > maxVisit){
        //         maxVisit = visit;
        //         index = i;
        //     }
        // }
        int index = std::distance(visits.begin(), std::max_element(visits.begin(), visits.end()));
        // int index = std::distance(rewards.begin(), std::max_element(rewards.begin(), rewards.end()));

        return game.getLegalMove1st()[index];
    }

    std::string decideHand_Vote(){

        // 合法手の列挙
        auto legalPattern = getLegalPattern(game);
        std::vector<MCTNode> trees;
        for(auto&& p: legalPattern){
            trees.push_back(MCTNode(game, p));
        }

        MCTNode::totalCount = 0;

        
        for(auto&& tree:trees){
            
            auto legalMoves = tree.root.getLegalMove1st();
            for(auto&& lm: legalMoves){
                auto child = tree;
                child.root.move(lm);
                tree.children.push_back(child);
            }
            while(MCTNode::totalCount < playoutCount){
                if(MCTNode::totalCount%100 == 0 && MCTNode::totalCount != 0) {
                    printf("%d\n",MCTNode::totalCount);
                    fflush(stdout);
                }
                tree.select();
            }
        }

        std::vector<double> visits(game.getLegalMove1st().size());
        for(auto&& tree:trees){
            for(int i = 0; i < tree.children.size(); ++i){
                visits[i] += tree.children[i].visitCount;
            }
        }
        int index = 0;
        double maxVisit = visits[0];
        for(int i = 0; i < visits.size(); i++){
            double visit = visits[i];
            if( visit > maxVisit){
                maxVisit = visit;
                index = i;
            }
        }

       auto lm = game.getLegalMove1st();

        auto copy = game;
        copy.move(lm[index]);
        copy.printBoard();
        fflush(stdout);

        return lm[index];
    }

    //脱出可能かどうかチェック
    int CheckCanEscape(Geister currentGame){
        auto legalMoves = currentGame.getLegalMove1st();
        //出口との距離が0の場合
        if (currentGame.IsExistUnit(0, 0) == 1){
            printf("脱出可能なコマがあります\n");
            return 1;
        }
        if (currentGame.IsExistUnit(5, 0) == 1){
            printf("脱出可能なコマがあります\n");
            return 1;
        }

        //出口との距離が1の場合
        if (currentGame.IsExistUnit(5, 5) != 3 && currentGame.IsExistUnit(0, 5) != 3){
            if (!(currentGame.IsExistUnit(0, 0) == 3 && currentGame.takenCount(UnitColor::Red) == 3)){
                if (currentGame.IsExistUnit(0, 1) == 1 && currentGame.IsExistUnit(1, 0) == 0){
                    return 3;
                }
                if (currentGame.IsExistUnit(1, 0) == 1 && currentGame.IsExistUnit(0, 1) == 0){
                    return 3;
                }
            }

            if (!(currentGame.IsExistUnit(5, 0) == 3 && currentGame.takenCount(UnitColor::Red) == 3)){
                if (currentGame.IsExistUnit(5, 1) == 1 && currentGame.IsExistUnit(4, 0) == 0){
                        return 3;
                }
                if (currentGame.IsExistUnit(4, 0) == 1 && currentGame.IsExistUnit(5, 1) == 0){
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