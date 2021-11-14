#include <string>
#include <vector>
#include "random.hpp"
#include "player.hpp"
#include "numpy.hpp"
#include <iostream>
#include "Encode.hpp"
#include <array>
#include <algorithm>
#include <map>
//#include "Encode2.cpp"

class MCTS_withProb: public Player{
    cpprefjp::random_device rd;
    std::mt19937 mt;
    std::uniform_int_distribution<int> choiceRandom;
public:
    MCTS_withProb():
    mt(rd()),
    choiceRandom(0, 1)
    {}

    //n番目の着手
    int moveNum;
    //着手のインデックス
    int MoveIndex;
    //テスト用
    float temprand;

    bool isMyturn;
    bool isGoodHandFound;
    
    //最大深さ
    int MaxDepth;

    int temporarycount = 0;

    std::vector<Hand> lastChoicedHands;
    std::vector<std::vector<float>> previousHs;
    Geister currentSimulate;

    //std::vector<Hand> legalMoves; //合法手のリスト
    std::vector<int> NextS; //次の一手
    std::vector<int> NextT; //次の一手デバッグ
    std::vector<std::vector<int>> NextsList; //次の一手の集合 sList
    std::vector<std::vector<int>> NextxList; //次の一手の特徴量の集合 xList
    std::vector<int> NextX;  //特徴量x
    //std::vector<float> theta;
    std::vector<double> theta;   //行動パラメータθ
    //std::vector<float> pri_h;
    std::vector<double> pri_h; //行動優先度h
    float max_h; //hの最大値(オーバーフロー抑制目的)
    float next_h; //hの準最大値(オーバーフロー抑制目的)
    //std::vector<float> poss_pi; //行動確率π
    //std::vector<float> round_pi; //πの偏微分
    std::vector<double> poss_pi; //行動確率π
    std::vector<double> round_pi; //πの偏微分
    int isTookUnit = 0; //Playout中に駒を取った回数(累計)
    std::vector<std::vector<float>> eval_list;

    constexpr static std::array<const char*, 70> pattern = {
            "ABCD", "ABCE", "ABCF", "ABCG", "ABCH", "ABDE", "ABDF",
            "ABDG", "ABDH", "ABEF", "ABEG", "ABEH", "ABFG", "ABFH",
            "ABGH", "ACDE", "ACDF", "ACDG", "ACDH", "ACEF", "ACEG",
            "ACEH", "ACFG", "ACFH", "ACGH", "ADEF", "ADEG", "ADEH",
            "ADFG", "ADFH", "ADGH", "AEFG", "AEFH", "AEGH", "AFGH",
            "BCDE", "BCDF", "BCDG", "BCDH", "BCEF", "BCEG", "BCEH",
            "BCFG", "BCFH", "BCGH", "BDEF", "BDEG", "BDEH", "BDFG",
            "BDFH", "BDGH", "BEFG", "BEFH", "BEGH", "BFGH", "CDEF",
            "CDEG", "CDEH", "CDFG", "CDFH", "CDGH", "CEFG", "CEFH",
            "CEGH", "CFGH", "DEFG", "DEFH", "DEGH", "DFGH", "EFGH"
        };


    //MCTSのノードのクラス化
    class MCTNode: public SIMULATOR{
        //メンバ変数
        int visitCount;
        static int totalCount;
        double reward;
        Geister currentBoard;

        //コンストラクタ
        MCTNode(Geister g):  visitCount(0), reward(0)
        {
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
                res = run_with_prob();
            }
            reward += res;
            return -res;
        }
    };

    std::array<char*, 70> possible_pattern;
    //駒配置セット
    virtual std::string decideRed(){
        lastChoicedHands.reserve(200);
        moveNum = 0;
        std::vector<std::vector<int>> NextsList;
        //theta = std::vector<float> (8128);
        theta = std::vector<double> (8128);
        std::string path_theta = "Player/data.csv";
        //Load_CSV_float(path_theta, theta);
        Load_CSV_double(path_theta, theta);
        std::string filepath = "Player/theta900.bin";
        //write_binary_float(filepath, theta);
        //read_binary_float(filepath, theta);
        //read_binary_double(filepath, theta_d);
        /* std::vector<int> s;
        std::vector<double> data;
        //aoba::LoadArrayFromNumpy(path_theta, s, data);
        //LoadNpyF(path_theta, theta);
        */
        cpprefjp::random_device rd;
        std::mt19937 mt(rd());

        std::uniform_int_distribution<int> serector(0, pattern.size() - 1);
        
        return pattern[serector(mt)];
        //return "ADFG";
        //return "GCHD";
    }

    virtual std::vector<Hand> candidateHand(){
        return game.getLegalMove1st();
    }

    #pragma region  行動優先度の計算

    std::vector<int> crrstate(Geister MovedGame, Geister copiedGame){
        std::vector<std::vector<int>> crr_state (3, std::vector<int> (42));
        std::vector<int> side_list{0, 0, 0, 0, 0, 0};
        std::vector<int> states_1ht;
        states_1ht.reserve(130);
        const std::array<Unit, 16>& CurrentUnits = copiedGame.allUnit();
        int count0;
        int count1;
        int count2;

        //駒の位置のベクトル表現
        const std::array<Unit, 16>& units = MovedGame.allUnit();
            for(const Unit& u: CurrentUnits){
                if (u.x() == 9 && u.y() == 9) { //取られた駒
                    if (u.color() == UnitColor::Blue){
                        if(side_list[2] < 3){
                            side_list[2] += 1;
                        }
                    } if (u.color() == (UnitColor::Red || UnitColor::Purple)){
                        if(side_list[3] < 3){
                            side_list[3] += 1;
                        }
                    } if (u.color() == UnitColor::blue){
                        if(side_list[0] < 3){
                            side_list[0] += 1;
                        }
                    } if (u.color() == UnitColor::red){
                        if(side_list[1] < 3){
                            side_list[1] += 1;
                        }
                    } if (u.color() == UnitColor::purple){
                        if(side_list[1] < 3){
                            side_list[1] += 1;
                        } else {
                            //side_list[0] += 1;
                        }
                    }
                }
            }
            count0 = 0;
            for(const Unit& u: units){
                if (u.x() != 9 && u.y() != 9){
                    if (u.x() == 8 && u.y() == 8) {//脱出駒
                        side_list[5] += 1; 
                    } else if (u.color() == UnitColor::Blue){
                        crr_state[0][u.x()+6*u.y()] = 1;
                    } else if (u.color() == UnitColor::Red){
                        crr_state[1][u.x()+6*u.y()] = 1;
                    } else if (u.color() == UnitColor::unknown ||
                               u.color() == UnitColor::purple ||
                               u.color() == UnitColor::red ||
                               u.color() == UnitColor::blue){
                        crr_state[2][u.x()+6*u.y()] = 1;
                    } else if (u.color() == UnitColor::Purple){
                        if (count0 % 2 == 0)  
                            crr_state[0][u.x()+6*u.y()] = 1;
                        else
                            crr_state[1][u.x()+6*u.y()] = 1;
                        count0++;
                    }
                }
            }

            for (int i = 0; i < side_list.size(); i++){
                int side_num = side_list[i];
                if (side_num > 0){
                    if (side_num > 3){
                        copiedGame.printBoard();
                        copiedGame.printInfo();
                        printf("There are over 4 lines in side_list, //%d lines, in listNo.%d\n", side_num, i);
                    }
                //printf("%d,%d\n", i, side_num);
                crr_state[side_num-1][36+i] = 1;
                }
            }

            count1 = 0;
            count2 = 0;
            for(const Unit& u: units){
                if (u.x() == 9 && u.y() == 9) count1+=1 ;
            }
            for(const Unit& u: CurrentUnits){
                if (u.x() == 9 && u.y() == 9) count2+=1 ;
            }
            if (count1 > count2) crr_state[0][6*6+4] = 1;
            
            for(int i = 0 ; i < 3 ; i++){
                for(int j = 0 ; j < 42 ; j++){
                states_1ht.push_back(crr_state[i][j]);
                }
            }
            states_1ht.push_back(1);

        return states_1ht;
    }

    //特徴量Xの計算
    std::vector<int> DecideNextX(int bias){
        std::vector<int> nextX;
        nextX.reserve(8129);
        for(int i = 0; i < NextS.size(); i++){
            for(int j = 0; j <= i; j++){
                    nextX.push_back(NextS[i]*NextS[j]);
                }
            }
        for(int i = 1; i < bias; i++){ //2021/09/21 biasからbias-1にしたが結局戻した
            nextX[nextX.size()-i] = 1;
        }
        return nextX;
    }
    
    //行動優先度の計算
    void DecidePriority_h(int l, std::vector<int> X){
        for(int i = 0; i < theta.size(); i++){ //内積(もっと速いの使いたい)
            pri_h[l] += theta[i]*X[i];
        }
        if (max_h < pri_h[l]) {
            max_h = pri_h[l];
        }
    }

    //オーバーフロー抑制目的でhを補正
    void adjust_h(int l){
        for(int j = 0; j < l; j++){
            pri_h[j] = pri_h[j] - max_h; 
        }
    }

    void DecidePi(std::vector<std::vector<int>> NextsList, std::vector<std::vector<int>> NextxList){
        int Moves_size = NextsList.size();
        //poss_pi = std::vector<float> (Moves_size);
        //pri_h = std::vector<float> (Moves_size);
        poss_pi = std::vector<double> (Moves_size);
        pri_h = std::vector<double> (Moves_size);
        double pi_sum = 0;
        max_h = -INFINITY;
        for(int l = 0; l < Moves_size; l++){
            //次の一手を更新
            DecidePriority_h(l, NextxList[l]);
        }
        adjust_h(Moves_size);
        for (int l = 0; l < Moves_size; l++){
            pi_sum += exp(pri_h[l]);
        }
        for (int l = 0; l < Moves_size; l++){
            poss_pi[l] = exp(pri_h[l])/pi_sum;
        }
    }
    
    //行動確率に従って手を選択
    int DecideIndex(){
        double tempSum = 0.000000000000000;
        std::uniform_real_distribution<> selecter(0.000000000000000, 1.000000000000000);
        double rand = selecter(rd);
        temprand = rand;
        double compare;
        int picked;
        //printf("%f\n", rand);
        for(int i = 0; i < poss_pi.size(); i++){
            if(i == 0){
                if((0.000000000000000 <= rand)&& (rand < poss_pi[0])){
                     return 0;
                }
            }
            if (i != 0){
                tempSum += poss_pi[i-1];
                float compare = tempSum+poss_pi[i];
                if(tempSum <= rand < compare)
                    return i-1; 
            }
            if (i == poss_pi.size()-1){
                return i;
            }
        }
    }

    //優先度が高い手を選択
    int ReturnHand(){
        std::vector<double>::iterator iter = std::max_element(poss_pi.begin(), poss_pi.end());
        int index = std::distance(poss_pi.begin(), iter);
        return index;
    }

    int ReturnHand_forSimulate(){
        std::vector<double>::iterator iter = std::max_element(poss_pi.begin(), poss_pi.end());
        int index = std::distance(poss_pi.begin(), iter);
        return index;
    }

    int randomHand(Geister current){
        auto legalMoves = current.getLegalMove1st();
        std::uniform_int_distribution<int> serector1(0, legalMoves.size() - 1);
        auto action = serector1(mt) % legalMoves.size();
        return action;
    }
    
    //脱出可能かどうかチェック
    bool CanEscape(Geister currentGame){
        auto legalMoves = currentGame.getLegalMove1st();
        for( auto move : legalMoves) {
            Unit u = move.unit;
            if(u.isBlue() && (u.x() == 0 && u.y() == 0) ) { return true; }
            if(u.isBlue() && (u.x() == 5 && u.y() == 0) ) { return true; }
        }
        return false;
    }

    Hand EscapeHand(Geister currentGame){
        auto legalMoves = currentGame.getLegalMove1st();
        for( auto move : legalMoves) {
            Unit u = move.unit;
            if(u.isBlue() && (u.x() == 0 && u.y() == 0) ) { return Hand{u, 'W'}; }
            if(u.isBlue() && (u.x() == 5 && u.y() == 0) ) { return Hand{u, 'E'}; }
        }
    }

    #pragma endregion
    
    #pragma region シミュレータの実装 

    //モンテカルロシミュレーションで着手
    int SimulatedHand(bool isCheckon, Geister currentgame){

        auto legalMoves = currentgame.getLegalMove1st();
        std::vector<Geister> simulators;
        for( auto m : legalMoves ) {
            Geister s = currentgame;
            s.move(m);
            simulators.push_back(s);
        }

        std::vector<double> rewards(legalMoves.size(), 0.0);
        for(int i = 0; i < legalMoves.size(); i++) {
            currentSimulate = simulators[i];
            rewards[i] += run_with_prob(100, simulators[i]);
        }

        int index_max = 0;
        for (int i = 1; i < legalMoves.size(); i++) {
            if(rewards[index_max] < rewards[i]) index_max = i;
        }

        for (i = 0; i < legalMoves.size(); i++)            
        {
            printf("モンテカルロは、%lf ： %s \n", rewards[i], legalMoves[i].toString().c_str());
            //simulators[i].printBoard();
            
        } 
        
        int tempcounter = 0;
        
        isGoodHandFound = true;

/*         if(isCheckon){
            for (int r = 0; r < rewards.size(); r++){
                if (rewards[index_max] == rewards[r])
                    tempcounter++;
                    if (tempcounter > 6){
                        isGoodHandFound = false;
                    }
            }
        } */

        return index_max;
    }

    // 未判明の相手駒色を適当に仮定
    std::vector<std::string> getLegalPattern()
    {
        std::vector<std::string> res(70);
        std::vector<char> blue(4);
        std::vector<char> red(4);

        size_t bsize = std::distance(blue.begin(), std::copy_if(Unit::nameList.begin(), Unit::nameList.begin()+8, blue.begin(), [&](const auto u){ return currentSimulate.allUnit()[u-'A'+8].color().isBlue();}));
        blue.resize(bsize);
        size_t rsize = std::distance(red.begin(), std::copy_if(Unit::nameList.begin(), Unit::nameList.begin()+8, red.begin(), [&](const auto u){ return currentSimulate.allUnit()[u-'A'+8].color().isRed();}));
        red.resize(rsize);

        size_t resSize = std::distance(res.begin(), std::copy_if(pattern.begin(), pattern.end(), res.begin(), [&](const char* p){ return 
            std::none_of(blue.begin(), blue.end(), [&](const char b){ return p[0] == b || p[1] == b || p[2] == b || p[3] == b; }) // 青と分かっている駒を含まないパターンである
            && std::all_of(red.begin(), red.end(), [&](const char r){ return p[0] == r || p[1] == r || p[2] == r || p[3] == r; }) // 赤と分かっている駒を含むパターンである
            ; }
        ));
        res.resize(resSize);
        return res;
    }

    // 未判明の相手駒色を適当に仮定
    std::string getRandomPattern(std::vector<std::string> legalPattern)
    {
        std::uniform_int_distribution<int> selector(0, legalPattern.size() - 1);
        return legalPattern[selector(mt)];
    }

    // 未判明の相手駒色を適当に仮定
    void setColor(std::string_view ptn){
        currentSimulate.setColor("", ptn);
    }

    // 未判明の相手駒色を適当に仮定
    void setColorRandom(){
        static std::uniform_int_distribution<int> BorR(0,1);
        std::string red;
        int assumeTakeBlue = 4;
        int assumeTakeRed = 4;
        for(int i = 8; i < 16; ++i){
            UnitColor color = currentSimulate.allUnit()[i].color();
            if(color == UnitColor::blue)
                assumeTakeBlue -= 1;
            if(color == UnitColor::red){
                assumeTakeRed -= 1;
                red += std::toupper(currentSimulate.allUnit()[i].name());
            }
        }
        for(int i = 8; i < 16; ++i){
            if(currentSimulate.allUnit()[i].color() == UnitColor::unknown){
                if(assumeTakeBlue > 0 && BorR(mt)){
                    assumeTakeBlue -= 1;
                }
                else if(assumeTakeRed > 0){
                    red += std::toupper(currentSimulate.allUnit()[i].name());
                    assumeTakeRed -= 1;
                }
                else{
                    break;
                }
            }
        }
        currentSimulate.setColor("", red);
    }

    Geister setUnknown(Geister boardforpi){
        const std::array<Unit, 16>& units = boardforpi.allUnit();
            for(const Unit& u: units){
                if(u.color() == UnitColor::blue || u.color() == UnitColor::red){
                    boardforpi.setColor(u, UnitColor::unknown);
                }
            }
        return boardforpi;
        
    }

    virtual double evaluate()
    {
        if (!isMyturn) { currentSimulate.changeSide(); }
        if (currentSimulate.result() == Result::OnPlay || currentSimulate.result() == Result::Draw) 
            return 0;
        if (static_cast<int>(currentSimulate.result()) > 0)         
            return 1.0;
        if (static_cast<int>(currentSimulate.result()) < 0)
            return -1.0;
    }

    double playout_withProb(){
        std::vector<Hand> lm;
        std::vector<Hand> enemylm;
        lm.reserve(32);
        enemylm.reserve(32);
        isMyturn = true;
        Hand m1, m2;
        for(int c = 0; c < MaxDepth; c++){
            
            if(currentSimulate.isEnd()){ //終了判定
                isMyturn = true;
                break;
            }
            // 相手の手番
            currentSimulate.changeSide();
            GetCurrentPi(currentSimulate);

            int index = DecideIndex();
            enemylm = currentSimulate.getLegalMove1st();
            m2 = enemylm[index];
            if(!CanEscape(currentSimulate)){
                m2 = enemylm[index];
            } else {
                m2 = EscapeHand(currentSimulate);
            } 

            currentSimulate.move(m2);

            if(currentSimulate.isEnd()){ //終了判定
                isMyturn = false;
                break;
            }

            // 自分の手番
            currentSimulate.changeSide();
            GetCurrentPi(currentSimulate);

            int index2 = DecideIndex();
            lm = currentSimulate.getLegalMove1st();
            if(!CanEscape(currentSimulate)){
                m1 = lm[index2];
            } else {
                m1 = EscapeHand(currentSimulate);
            } 
            currentSimulate.move(m1);

        }
        return evaluate();
    }

    double run_with_prob(const size_t count, Geister root){
        double result = 0.0;
        std::vector<std::string> legalPattern = getLegalPattern();

        for(size_t i = 0; i < count; ++i){
            currentSimulate = root;
            setColor(getRandomPattern(legalPattern));
            result += playout_withProb();
        }
        
        return result;
    }

    #pragma endregion

    //行動確率πの取得
    void GetCurrentPi(Geister current){
        Geister copiedGame, MovedGame;
        std::vector<Hand> legalMoves;
        legalMoves.reserve(32);
        legalMoves = current.getLegalMove1st();
        NextS.reserve(127);
        NextX.reserve(8129);
        NextsList = std::vector<std::vector<int>> {legalMoves.size()};
        NextxList = std::vector<std::vector<int>> {legalMoves.size()};
        NextsList.reserve(32);
        NextxList.reserve(32);
        
        
        for(int l = 0; l < legalMoves.size(); l++){
            copiedGame = current;
            MovedGame = current;
            MovedGame.move(legalMoves[l]);
            NextS = crrstate(MovedGame, copiedGame);
            NextX = DecideNextX(legalMoves.size());
            NextsList[l] = NextS;
            NextxList[l] = NextX;
        }
        DecidePi(NextsList, NextxList);
    }

    //着手
    virtual std::string decideHand(std::string_view res){
        game.setState(res);
        moveNum++;
        auto legalMoves = game.getLegalMove1st();
        if(CanEscape(game)){
            auto lastHand = EscapeHand(game);
            return lastHand;
        }


        bool isCheckon = false;
        MaxDepth = 50;

        //モンテカルロシミュレーションで着手を決定
        MoveIndex = SimulatedHand(isCheckon, game);
        
        auto nexthand = legalMoves[MoveIndex];
        return nexthand;
        
        int counter = 0;


        for(int l = 0; l < legalMoves.size(); l++){
            if (Hand_equal(legalMoves[l], nexthand)){
                    lastChoicedHands.push_back(nexthand);
                    for(int h = 0; h < lastChoicedHands.size(); h++){
                        if (Hand_equal(lastChoicedHands[h], nexthand) == true){
                            counter++;
                        }
                    }
                    if (counter < 10) {
                        return nexthand;
                    } else {
                        poss_pi[MoveIndex] = 0.0;
                        MoveIndex = ReturnHand();
                        nexthand = legalMoves[MoveIndex];
                        return nexthand;
                    }  
            }
        }

        //以下猪突プレイヤー
        std::vector<Hand> lastHands;
        Hand lastHand;
        
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
        for(const Unit& u: units){
            if(u.color() == UnitColor::Blue && u.y() == 0){
                if(u.x() < 3 && u.x() > 0){
                    lastHands.push_back(Hand{u, Direction::West});
                }
                else if(u.x() < 5){
                    lastHands.push_back(Hand{u, Direction::East});
                }
            }
        }

        int mostFrontPos = units[0].y();
        int mostFrontIndex = 0;
        for(int u = 0; u < 8; u++){
            const Unit& unit = units[u];
            if(unit.color() == UnitColor::Blue && unit.y() <= mostFrontPos && unit.y() > 0){
                if(unit.y() < mostFrontPos || choiceRandom(mt)){
                    mostFrontIndex = u;
                    mostFrontPos = unit.y();
                }
            }
        }
        Unit MovingUnit = units[mostFrontIndex];
        
        lastHands.push_back(Hand{MovingUnit, Direction::North});
        
        std::uniform_int_distribution<int> serector0(0, lastHands.size() - 1);
        lastHand = legalMoves[serector0(mt) % lastHands.size()];
        
        for(int l = 0; l < legalMoves.size(); l++){
                if (Hand_equal(legalMoves[l], lastHand)){
                    return lastHand;
                }
        }

        std::uniform_int_distribution<int> serector1(0, legalMoves.size() - 1);
        auto action = legalMoves[serector1(mt) % legalMoves.size()];
        return action;
        //return Hand{units[mostFrontIndex], Direction::North};
    }

    #pragma region デバッグ
    
    void poss_pi_debug(std::vector<Hand> lm){
        for(int i = 0; i < poss_pi.size(); i++){
            printf("%d：%lf: %s\n", i, poss_pi[i], lm[i].toString().c_str());
        }
    }
    
    void lm_debug(std::vector<Hand> lm){
        for(int i = 0; i < lm.size(); i++){
            printf("%d: %s\n", i, lm[i].toString().c_str());
        }
    }

    void theta_debug(){
        for(int i = 0; i < theta.size(); i++){
            double delta = theta[i]-theta[i];
            printf("差分%d：%lf\n", i, delta);
        }
    }

    void hs_debug(std::vector<Hand> lm){
        for(int i = 0; i < pri_h.size(); i++){
            printf("%d：%lf: %s\n", i, pri_h[i], lm[i].toString().c_str());
        }
    }

};