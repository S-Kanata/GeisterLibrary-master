#include <string>
#include <vector>
#include "random.hpp"
#include "player.hpp"
#include "numpy.hpp"
#include <iostream>
#include "Encode.hpp"
#include "simulator.hpp"
#include <array>
#include <algorithm>
#include <map>
//#include "Encode2.cpp"

class MachinePlayer: public Player{
    cpprefjp::random_device rd;
    std::mt19937 mt;
    std::uniform_int_distribution<int> choiceRandom;
public:
    MachinePlayer():
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
                            side_list[0] += 1;
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

        /*for(int i = 0 ; i < states_1ht.size(); i++){
            printf("%d, %d\n", i, states_1ht[i]);    
        }*/
        return states_1ht;
    }
    //次の一手のベクトル表現s
    void DecideNextS(Geister copiedGame) {
            //次の一手のベクトル表現12
        std::vector<int> allyBlue = std::vector<int> (36);
        std::vector<int> allyRed = std::vector<int> (36);
        std::vector<int> enemyObj = std::vector<int> (36);
        std::vector<int> takenBlueAlly = std::vector<int> (3);
        std::vector<int> takenBlueEnemy = std::vector<int> (3);
        std::vector<int> takenRedAlly = std::vector<int> (3);
        std::vector<int> takenRedEnemy = std::vector<int> (3);
        std::vector<int> IstakeObj{0};
        std::vector<int> IsEscapeBlue{0};
        NextS.clear();
        NextS.reserve(127);
      //for(int l = 0; l < i; ++l)
      //for(int l = 0; l < legalMoves.size(); ++l){
        

    //copiedGame.move(legalMoves[0]);

        //駒の位置のベクトル表現
        const std::array<Unit, 16>& units = copiedGame.allUnit();
            for(const Unit& u: units){
                if(u.x() < 8 && u.y() < 8){
                    auto index = u.x()+6*u.y();
                if(u.color() == UnitColor::Blue){
                    allyBlue[index] = 1;
                } else if (u.color() == UnitColor::Red) {
                  allyRed[index] = 1;
                } else if (u.color() == UnitColor::unknown) {
                   enemyObj[index] = 1;
                }
            }
        }

        //取られた駒のone-hot表現
        int n = game.takenCount(UnitColor::Blue);
        if(n != 0) {
            takenBlueAlly[n-1] = 1;
        }
        n = game.takenCount(UnitColor::Red);
        if(n != 0) {
            takenRedAlly[n-1] = 1;
        }
        n = game.takenCount(UnitColor::blue);
        if(n != 0) {
            takenBlueEnemy[n-1] = 1;
        }
        n = game.takenCount(UnitColor::red);
        if(n != 0) {
            takenRedEnemy[n-1] = 1;
        }

        //コマを取るor取らない
        const std::array<Unit, 16>& CurrentUnits = game.allUnit();
        int count1 = 0;
        int count2 = 0;
        for(const Unit& u: units){
            if (u.x() == 9 ) count1++ ;
        }
        for(const Unit& u: CurrentUnits){
            if (u.x() == 9) count2++ ;
        }
        if (count1 != count2) IstakeObj[0] = 1;

        //コマが脱出するorしない
        for(const Unit& u: units){
            if (u.x() == 8) IsEscapeBlue[0] = 1 ;
        }

        //ベクトルを結合してsに追加
        NextS.insert(NextS.end(),allyBlue.begin(), allyBlue.end());
        NextS.insert(NextS.end(),allyRed.begin(), allyRed.end());
        NextS.insert(NextS.end(),enemyObj.begin(), enemyObj.end());
        NextS.insert(NextS.end(),takenBlueAlly.begin(), takenBlueAlly.end());
        NextS.insert(NextS.end(),IstakeObj[0]);
        NextS.insert(NextS.end(),takenRedAlly.begin(), takenRedAlly.end());
        NextS.insert(NextS.end(),takenBlueEnemy.begin(), takenBlueEnemy.end());
        NextS.insert(NextS.end(),takenRedEnemy.begin(), takenRedEnemy.end());
        NextS.insert(NextS.end(),IsEscapeBlue[0]);


        for(int i = 0; i < 5; i++)
        {
            NextS.push_back(0);
        }
        //auto itr = NextS.end();
        //NextS.erase(itr);
    }
    /*

    std::vector<int> DecideNextS2(Geister copiedGame) {
        std::vector<int> NextS2 = std::vector<int> {0};
        //次の一手のベクトル表現12
        std::vector<int> allyBlue = std::vector<int> (36);
        std::vector<int> allyRed = std::vector<int> (36);
        std::vector<int> enemyObj = std::vector<int> (36);
        std::vector<int> takenBlueAlly = std::vector<int> (3);
        std::vector<int> takenBlueEnemy = std::vector<int> (3);
        std::vector<int> takenRedAlly = std::vector<int> (3);
        std::vector<int> takenRedEnemy = std::vector<int> (3);
        std::vector<int> IstakeObj{0};
        std::vector<int> IsEscapeBlue{0};
      //for(int l = 0; l < i; ++l)
      //for(int l = 0; l < legalMoves.size(); ++l){
        

    //copiedGame.move(legalMoves[0]);

        //駒の位置のベクトル表現
        const std::array<Unit, 16>& units = copiedGame.allUnit();
            for(const Unit& u: units){
                if(u.x() < 8 && u.y() < 8){
                    auto index = u.x()*6+u.y();
                    if(u.color() == UnitColor::Blue){
                        allyBlue[index] = 1;
                    } else if (u.color() == UnitColor::Red) {
                        allyRed[index] = 1;
                    } else if (u.color() == UnitColor::unknown) {
                       enemyObj[index] = 1;
                    }
                }
            }
        

        //取られた駒のone-hot表現
        int n = copiedGame.takenCount(UnitColor::Blue);
        if(n != 0) {
            takenBlueAlly[n-1] = 1;
        }
        n = copiedGame.takenCount(UnitColor::Red);
        if(n != 0) {
            takenRedAlly[n-1] = 1;
        }
        n = copiedGame.takenCount(UnitColor::blue);
        if(n != 0) {
            takenBlueEnemy[n-1] = 1;
        }
        n = copiedGame.takenCount(UnitColor::red);
        if(n != 0) {
            takenRedEnemy[n-1] = 1;
        }
        
        //コマを取るor取らない
        const std::array<Unit, 16>& CurrentUnits = game.allUnit();
        int count1 = 0;
        int count2 = 0;
        for(const Unit& u: units){
            if (u.x() == 9 ) count1 +=  1;
        }
        for(const Unit& u: CurrentUnits){
            if (u.x() == 9) count2 += 1 ;
        }
        if (count1 != count2) IstakeObj[0] = 1;

        //コマが脱出するorしない
        for(const Unit& u: units){
            if (u.x() == 8) IsEscapeBlue[0] = 1 ;
        }
        //ベクトルを結合してsに追加
        NextS2.insert(NextS2.end(),allyBlue.begin(), allyBlue.end());
        NextS2.insert(NextS2.end(),allyRed.begin(), allyRed.end());
        NextS2.insert(NextS2.end(),enemyObj.begin(), enemyObj.end());
        NextS2.insert(NextS2.end(),takenBlueAlly.begin(), takenBlueAlly.end());
        NextS2.insert(NextS2.end(),takenRedAlly.begin(), takenRedAlly.end());
        NextS2.insert(NextS2.end(),takenBlueEnemy.begin(), takenBlueEnemy.end());
        NextS2.insert(NextS2.end(),takenRedEnemy.begin(), takenRedEnemy.end());
        NextS2.insert(NextS2.end(), IstakeObj[0]);
        NextS2.insert(NextS2.end(), IsEscapeBlue[0]);

        //auto itr = NextS.end();
        //NextS.erase(itr);
        return NextS2;
    }*/
    

    //特徴量Xの計算
    void DecideNextX(int bias){
        NextX.clear();
        NextX.reserve(8128);
        for(int i = 0; i < NextS.size(); i++){
            for(int j = 0; j <= i; j++){
                    NextX.push_back(NextS[i]*NextS[j]);
                }
            }
        for(int i = 1; i < bias-1; i++){
            NextX[NextX.size()-i] = 1;
        }
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
/*         std::vector<float>::iterator iter = std::max_element(poss_pi.begin(), poss_pi.end());
        int index = std::distance(poss_pi.begin(), iter);
        next_h = -INFINITY;
        for(int p = 0; p < poss_pi.size(); p++){
            if(p != index){
                if(poss_pi[p] > next_h){
                    next_h = poss_pi[p];
                }
            }
        } */
    }

    /* Piの決定保存用
    void DecidePi(){
        legalMoves = candidateHand();
        int Moves_size = legalMoves.size();
        poss_pi = std::vector<float> (Moves_size);
        pri_h = std::vector<float> (Moves_size);
        float pi_sum = 0;
        max_h = -INFINITY;
        for(int l = 0; l < Moves_size; l++){
            //次の一手を更新
            Geister copiedGame = game;
            copiedGame.move(legalMoves[l]);
            DecideNextS(copiedGame);
            DecideNextX();
            DecidePriority_h(l);
        }
        adjust_h(Moves_size);
        for (int l = 0; l < Moves_size; l++){
            pi_sum += exp(pri_h[l]);
        }
        for (int l = 0; l < Moves_size; l++){
            poss_pi[l] = exp(pri_h[l])/pi_sum;
        }
    }
    */
    
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
    bool CanEscape(){
        auto legalMoves = currentSimulate.getLegalMove1st();
        for( auto move : legalMoves) {
            Unit u = move.unit;
            if(u.isBlue() && (u.x() == 0 && u.y() == 0) ) { return true; }
            if(u.isBlue() && (u.x() == 5 && u.y() == 0) ) { return true; }
        }
        return false;
    }

    Hand EscapeHand(){
        auto legalMoves = currentSimulate.getLegalMove1st();
        for( auto move : legalMoves) {
            Unit u = move.unit;
            if(u.isBlue() && (u.x() == 0 && u.y() == 0) ) { return Hand{u, 'W'}; }
            if(u.isBlue() && (u.x() == 5 && u.y() == 0) ) { return Hand{u, 'E'}; }
        }
    }

    #pragma endregion

    #pragma region  θの学習

    void DecideRoundPi() {
        round_pi = std::vector<double> (8128);
        auto legalMoves = candidateHand();
        std::vector<int> NextX_Current = NextX;
        for(int l = 0; l < legalMoves.size(); l++){
            Geister copiedGame = game;
            copiedGame.move(legalMoves[l]);
            DecideNextS(copiedGame);
            DecideNextX(legalMoves.size());
            for(int i = 0; i < NextX_Current.size(); i++)
            round_pi[i] = NextX_Current[i] - poss_pi[l]*NextX[i];
        }
    }

    /*
    void learnTheta(int result, int episodes){
        for(int i = 0; i < theta.size();)
        theta = 
        std::string filepath("Player/theta/theta%d.dat", episodes);
        write_binary_float(filepath, theta);
        //read_binary_float("Player/theta/test.dat");
    }
    */
    
    #pragma endregion
    
    #pragma region  探索
    

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
            rewards[i] += run(100, simulators[i]);
            
        }

        int index_max = 0;
        for (int i = 1; i < legalMoves.size(); i++) {
            if(rewards[index_max] < rewards[i]) index_max = i;
        }
        
        
        /* for (i = 0; i < legalMoves.size(); i++)            
        {
            //printf("モンテカルロは、%lf ： %s \n", rewards[i], legalMoves[i].toString().c_str());
            //simulators[i].printBoard();
            
        } */
        
        

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
    std::string getRandomPattern()
    {
        std::vector<std::string> legalPattern = getLegalPattern();
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


    virtual double evaluate()
    {
        if (!isMyturn) { currentSimulate.changeSide(); }
        if (currentSimulate.result() == Result::OnPlay || currentSimulate.result() == Result::Draw) 
            return 0;
        if (static_cast<int>(currentSimulate.result()) > 0)
            
            //temporarycount++;
            return 1.0;
        if (static_cast<int>(currentSimulate.result()) < 0)
            /* if(temporarycount < 5){
                
            } */
            //currentSimulate.printBoard();
            return -1.0;
    }

    double playout(){
        std::vector<Hand> lm;
        std::vector<Hand> enemylm;
        lm.reserve(32);
        enemylm.reserve(32);
        int MoveCount = 0;
        isMyturn = true;
        Hand m1, m2;
        for(int c = 0; c < MaxDepth; c++){
            if(currentSimulate.isEnd()){ //終了判定
                isMyturn = true;
                break;
            }
            // 相手の手番
            if (temporarycount < MaxDepth){
                //currentSimulate.printBoard();
            }
            currentSimulate.changeSide();
            if (temporarycount < MaxDepth){
                currentSimulate.printBoard();
            }
            GetCurrentPi(currentSimulate);
            int index = DecideIndex();
            enemylm = currentSimulate.getLegalMove1st();
            m2 = enemylm[index];
/*             if(!CanEscape()){
                enemylm = currentSimulate.getLegalMove1st();
                m2 = enemylm[index];
            } else {
                m2 = EscapeHand();
            } */

            currentSimulate.move(m2);
            if (temporarycount < MaxDepth){
                currentSimulate.printBoard();
            }
            //currentSimulate.printBoard();
            if(currentSimulate.isEnd()){ //終了判定
                isMyturn = false;
                break;
            }

            // 自分の手番
            currentSimulate.changeSide();
            GetCurrentPi(currentSimulate);
            int index2 = DecideIndex();
            lm = currentSimulate.getLegalMove1st();
            m1 = lm[index2];
/*             if(!CanEscape()){
                
            } else {
                m1 = EscapeHand();
            } */
            currentSimulate.move(m1);
            if (temporarycount < MaxDepth){
                //currentSimulate.printBoard();
            }

            if (temporarycount < 200)
                temporarycount++;
            //printf("現在は %d\n", MoveCount);
        }

        return evaluate();
    }

    double run(const size_t count, Geister root){
        double result = 0.0;
        for(size_t i = 0; i < count; ++i){
            currentSimulate = root;
            setColor(getRandomPattern());
            //currentSimulate.setColor("", "BCFG");
            //if(i == 0) currentSimulate.printBoard();
            // current.printBoard();
            // std::cout << current.result() << "\n";
            // setColorRandom();
            result += playout();
        }
        return result;
    }

    #pragma endregion

    int SearchIndex(Geister current, int depthodd){
        std::vector<float> rewards(32, -INFINITY);
        Geister MovedGame = current;
        auto legalMoves = current.getLegalMove1st();
        legalMoves.reserve(32);
        for(int l = 0; l < legalMoves.size(); l++){
            MovedGame = current;
            for(int d = 0; d < depthodd; d++){
                if (MovedGame.isEnd()) break;
                auto mylm = MovedGame.getLegalMove1st();  
                for(auto&& u: MovedGame.allUnit()){
                    if(u.color() == UnitColor::unknown)
                       MovedGame.setColor(u, UnitColor::purple);
                }
                if(d == 0 ){
                    MovedGame.move(legalMoves[l]);
                } else {  
                    GetCurrentPi(MovedGame);
                    MoveIndex = ReturnHand();
                    MovedGame.move(mylm[MoveIndex]);
                }
                MovedGame.printBoard();
                MovedGame.changeSide(); //相手の手番に変更
                for(auto&& u: MovedGame.allUnit()){
                    if(u.color() == UnitColor::unknown)
                       MovedGame.setColor(u, UnitColor::purple);
                }     
                auto enemylm = MovedGame.getLegalMove1st();     
                GetCurrentPi(MovedGame);
                MoveIndex = ReturnHand();
                MovedGame.move(enemylm[MoveIndex]); //相手の着手
                if (MovedGame.isEnd()) break;
                MovedGame.changeSide(); //自分の手番に戻す
            }
            GetCurrentPi(MovedGame);
            float reward = 0.0000000;
            int max_i = 0;
            auto lmm = MovedGame.getLegalMove1st();
            printf("prihsize %d, lmmsize %d", pri_h.size(), lmm.size());
            for(int i = 0; i < pri_h.size(); i++){
                reward += pri_h[i]*pri_h[i];
            //auto reward = *std::max_element(pri_h.begin(), pri_h.end());
            }
            printf("reward は　%f\n", reward);
            rewards[l] = reward;
        }

        std::vector<float>::iterator iter = std::min_element(rewards.begin(), rewards.end());
        int min_reward_index = std::distance(rewards.begin(), iter);
        printf("reward%d : %f", i, rewards[min_reward_index]);
        return min_reward_index;
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
            DecideNextX(legalMoves.size());
            NextsList[l] = NextS;
            NextxList[l] = NextX;
        }
        DecidePi(NextsList, NextxList);
    }

    //着手
    virtual std::string decideHand(std::string_view res){
        game.setState(res);
        moveNum++;
        if (moveNum == 1 ){
            temporarycount = 200;
        } else {
            temporarycount = 300;
        }


        bool isCheckon = false;
        MaxDepth = 50;

        //モンテカルロシミュレーションで着手を決定
        MoveIndex = SimulatedHand(isCheckon, game);

        //モンテカルロが好手を見つけられない場合
/*         if (isGoodHandFound = false){
            GetCurrentPi(game);
            MoveIndex = ReturnHand(); //通常通りポリシーに従って着手
            //printf("100, Moves:%d\n" ,moveNum);
        } */


        
        //GetCurrentPi(game);
        //MoveIndex = ReturnHand();
        auto legalMoves = game.getLegalMove1st();
        auto nexthand = legalMoves[MoveIndex];
        return nexthand;
        printf("error");

        /* currentSimulate = game;
        //currentSimulate.setColor("", "BCFG");
        currentSimulate.changeSide();
        GetCurrentPi(currentSimulate);
        currentSimulate.printBoard();
        auto str = currentSimulate.toString();
        printf("%s\n", str.c_str());
        auto lmd = currentSimulate.getLegalMove1st();
        hs_debug(lmd); */

        //theta_debug();
        //poss_pi_debug(lmd);
        /* if (moveNum < 10){
        for(int p = 0; p < poss_pi.size(); p++){
            printf("%d, %f\n", p, poss_pi[p]);
        }
        } */

        

        int counter = 0;


        for(int l = 0; l < legalMoves.size(); l++){
            if (Hand_equal(legalMoves[l], nexthand)){
                    //printf("今の盤面は\n");
                    //game.printBoard();
                    lastChoicedHands.push_back(nexthand);
                    for(int h = 0; h < lastChoicedHands.size(); h++){
                        if (Hand_equal(lastChoicedHands[h], nexthand) == true){
                            counter++;
                        }
                    }
                    //previousBoards.push_back(currentgame,move(nexthand));
                    //lastChoicedHands.push_back(nexthand); //最後に指した手を記録
                    if (counter < 10) {
                        return nexthand;
                    } else {
                        poss_pi[MoveIndex] = 0.0;
                        MoveIndex = ReturnHand();
                        nexthand = legalMoves[MoveIndex];
                        return nexthand;
                    }
                    /*
                    if(Hand_equal(lastChoicedHand, nexthand)){
                        nexthand = randomHand(game);
                        return nexthand;
                    }*/       
            }
        }

        /*
        string handa = nexthand.toString();
        printf("%s\n", handa.c_str());
        std::vector<int> test = crr_state(game);
        for(int i = 0; i < test.size(); i++){
            printf("%d ：　%d \n", i, test[i]);
        }
        */

        /*
        for(int i=0; i < poss_pi.size(); i++){
                printf("%d: %f\n Move: %d\n",i, poss_pi[i], MoveIndex);
            }
        */
        /*if(poss_pi[0]*(MoveIndex) <= temprand <= poss_pi[0]*(MoveIndex+1)){
            printf("OK\n");
        } else {
            printf("NG\n");
        }
        */
        //printf("Xsize is %d , Ssize is %d", NextX.size(), NextS.size());
        //DecidePi();

        //if (0 < moveNum < 100){
            /*DecideNextS(0);
            std::vector<int> NextT = std::vector<int>(NextS.size());
            DecideNextX();
            game.printBoard();
            write_binary("Player/theta/test.dat", NextS);
            read_binary("Player/theta/test.dat", NextT);
            //printf("%d: %d\n",120, NextT[120]);*/
            //DecidePi();
            //MoveIndex = DecideIndex();
            /*for(int i=0; i < poss_pi.size(); i++){
                printf("%d: %f\n Move: %d\n",i, poss_pi[i], MoveIndex);
            }
            if(poss_pi[0]*(MoveIndex) <= temprand <= poss_pi[0]*(MoveIndex+1)){
                printf("OK\n");
            } else {
                printf("NG\n");
            }
            DecideNextS(MoveIndex);
            NextsList.push_back(NextS);*/
            //return legalMoves[MoveIndex];


            /* デバッグ
            for(int i=0; i  <= takenBlueAlly.size(); i++){
                printf("%d: %d\n",i, takenBlueAlly[i]);
            }
            for(int i=0; i  <= takenRedAlly.size(); i++){
                printf("%d: %d\n",i, takenRedAlly[i]);
            }
            for(int i=0; i  <= takenBlueEnemy.size(); i++){
                printf("%d: %d\n",i, takenBlueEnemy[i]);
            }
            for(int i=0; i  <= takenRedEnemy.size(); i++){
                printf("%d: %d\n",i, takenRedEnemy[i]);
            } */

        //}

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
    /*    for(const Unit& u: units){
            if (MovingUnit.x() == u.x() && MovingUnit.y()-1 == u.y()) {
                    std::uniform_int_distribution<int> serector1(0, legalMoves.size() - 1);
                    auto action = legalMoves[serector1(mt) % legalMoves.size()];
                    return action;
            } 
        }*/

        
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

    //行動確率シミュレートのデバッグ
    /*
        GetCurrentPi(game);
        auto lm = game.getLegalMove1st();
        std::vector<int> Indexes = std::vector<int>(lm.size()) ;
        MoveIndex = DecideIndex();
        poss_pi_debug();
        
        for(int i = 0; i < Indexes.size(); i++){
            printf("%d, %d \n", i, Indexes[i]);
        }
        game.move(lm[MoveIndex]);
        game.printBoard();
    */
   
    //チェンジサイドのデバッグ
    /*
        currentSimulate = game;
        currentSimulate.setColor("", "BCFG");
        currentSimulate.changeSide();
        GetCurrentPi(currentSimulate);
        currentSimulate.printBoard();
        auto str = currentSimulate.toString();
        printf("%s\n", str.c_str());
        auto lmd = currentSimulate.getLegalMove1st();
        poss_pi_debug(lmd);
    */

};