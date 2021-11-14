#include "simulator.hpp"
#include "player.hpp"
#include <string>
#include <vector>

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
        constexpr double alpha = 1.5;
        return (reward / std::max(visitCount, 1))
            + (alpha * sqrt(log((double)totalCount) / std::max(visitCount, 1)));
    }

    void expand(){
        root.changeSide();
        auto legalMoves = root.getLegalMove1st();
        for(auto&& lm: legalMoves){
            auto child = *this;
            child.root.move(lm);
            children.push_back(child);
        }
        root.changeSide();
    }

    double select(){
        visitCount++;
        if(auto res = static_cast<int>(root.getResult()); res != 0){
            totalCount++;
            reward += res;
            return -res;
        }
        double res;
        if(!children.empty()){
            std::vector<double> ucbs(children.size());
            std::transform(children.begin(), children.end(), ucbs.begin(), [](auto& x){return x.calcUCB();});
            auto&& max_iter = std::max_element(ucbs.begin(), ucbs.end());
            int index = std::distance(ucbs.begin(), max_iter);
            res = children[index].select();

        }
        // 訪問回数が規定数を超えていたら子ノードをたどる
        else if(visitCount > expandCount){
            // 子ノードがなければ展開
            expand();
            for(auto&& child: children){
                res = child.select();
            }
        }
        // プレイアウトの実行
        else{
            totalCount++;
            res = run();
        }
        reward += res;
        return -res;
    }
};