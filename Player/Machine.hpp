#include <string>
#include <vector>
#include "random.hpp"
#include "player.hpp"
#include "numpy.hpp"
#include <iostream>
#include "Encode.hpp"
#include "simulator.hpp"
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
    Hand lastChoicedHand;

    //合法手のリスト
    std::vector<Hand> legalMoves;
    std::vector<int> NextS; //次の一手
    std::vector<int> NextT; //次の一手デバッグ
    std::vector<std::vector<int>> NextsList; //次の一手の集合 sList
    std::vector<std::vector<int>> NextxList; //次の一手の特徴量の集合 xList
    std::vector<int> NextX;  //特徴量x
    std::vector<float> theta;  //行動パラメータθ
    std::vector<float> pri_h; //行動優先度h
    float max_h; //hの最大値(オーバーフロー抑制目的)
    std::vector<float> poss_pi; //行動確率π
    std::vector<float> round_pi; //πの偏微分
    int isTookUnit = 0; //Playout中に駒を取った回数
    std::vector<std::vector<float>> eval_list;

    //駒配置セット
    virtual std::string decideRed(){
        moveNum = 0;
        std::vector<std::vector<int>> NextsList;
        theta = std::vector<float> (8128);
        std::string path_theta = "Player/data.csv";
        //Load_CSV_float(path_theta, theta);
        std::string filepath = "Player/theta900.bin";
        //write_binary_float(filepath, theta);
        read_binary_float(filepath, theta);
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
                        side_list[2] += 1;
                    } if (u.color() == UnitColor::Red){
                        side_list[3] += 1;
                    } if (u.color() == UnitColor::blue){
                        side_list[0] += 1;
                    } if (u.color() == UnitColor::red){
                        side_list[1] += 1;
                    } if (u.color() == UnitColor::purple){
                        if(side_list[1] < 4){
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
        for(int i = 1; i <= bias; i++){
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
        poss_pi = std::vector<float> (Moves_size);
        pri_h = std::vector<float> (Moves_size);
        float pi_sum = 0;
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
        float tempSum = 0.0000000;
        std::uniform_real_distribution<> selecter(0.0000000, 1.0000000);
        float rand = selecter(rd);
        temprand = rand;
        //printf("%f\n", rand);
        for(int i=0; i < poss_pi.size(); i++){
            if(i == 0){
                if(0.0000000 <= rand < poss_pi[0]){
                     return 0;
                }
            } else {
                tempSum += poss_pi[i-1];
                if(tempSum <= rand < tempSum+poss_pi[i]){
                    return i;
                }
            }
        }
    }

    //優先度が高い手を選択
    int ReturnHand(){
        std::vector<float>::iterator iter = std::max_element(poss_pi.begin(), poss_pi.end());
        int index = std::distance(poss_pi.begin(), iter);
        return index;
    }

    //モンテカルロシミュレーションで着手
    int SimulatedHand(){

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

        return index_max;
    }

    Hand randomHand(Geister current){
        legalMoves = current.getLegalMove1st();
        std::uniform_int_distribution<int> serector1(0, legalMoves.size() - 1);
        auto action = legalMoves[serector1(mt) % legalMoves.size()];
        return action;

    }

    #pragma endregion

    #pragma region  θの学習

    void DecideRoundPi() {
        round_pi = std::vector<float> (8128);
        legalMoves = candidateHand();
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

    void GetCurrentPi(Geister current){
        Geister copiedGame, MovedGame;
        legalMoves.reserve(32);
        legalMoves = current.getLegalMove1st();
        NextS.reserve(127);
        NextX.reserve(8129);
        NextsList = std::vector<std::vector<int>> {legalMoves.size()};
        NextxList = std::vector<std::vector<int>> {legalMoves.size()};
        NextsList.reserve(32);
        NextxList.reserve(32);
        for(int l = 0; l < legalMoves.size(); l++){
            copiedGame = game;
            MovedGame = game;
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

        //int depth = 1;
        //GetCurrentPi(game);
        //MoveIndex = ReturnHand();

        GetCurrentPi(game);
        MoveIndex = ReturnHand();
        legalMoves = game.getLegalMove1st();
        auto nexthand = legalMoves[MoveIndex];
        
        
        for(int l = 0; l < legalMoves.size(); l++){
            if (Hand_equal(legalMoves[l], nexthand)){
                    //string handa = nexthand.toString();
                    //printf("%s\n", handa.c_str());
                    return nexthand;
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


};