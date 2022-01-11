#include "geister.hpp"
#include <random>
#include "random.hpp"
#include "result.hpp"
#include "Player/Encode.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#pragma once


static cpprefjp::random_device rd;
static std::mt19937 mt;

bool isMyturn;
int MaxDepth;
float temprand;
Geister root;

std::vector<int> NextS; //次の一手
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
bool isLoaded;

void settheta(){
    if(!isLoaded){
        theta = std::vector<double> (8129);
        std::string path_theta = "Player/data.csv";
        Load_CSV_double(path_theta, theta);
        isLoaded = true;
    }
}

double evaluate() {
    if (!isMyturn) { root.changeSide(); }
    if (root.result() == Result::OnPlay || root.result() == Result::Draw) 
        return 0;
    if (static_cast<int>(root.result()) > 0)         
        return 1.0;
    if (static_cast<int>(root.result()) < 0)
        return -1.0;
}

//脱出可能かどうかチェック
int CanEscape(Geister currentGame){

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

        if (!(currentGame.IsExistUnit(5, 0) != 6 && currentGame.takenCount(UnitColor::Red) == 3)){
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

    
    /*
    auto legalMoves = currentGame.getLegalMove1st();
    
    //出口との距離が0の場合
    if (currentGame.IsExistUnit(0, 0) == 1){
        return 1;
    }
    if (currentGame.IsExistUnit(5, 0) == 1){
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
    */

Hand EscapeHand(Geister currentGame, int depth){

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
    
    /*
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
    */
}

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

std::vector<double> DecidePi(std::vector<std::vector<int>> NextsList, std::vector<std::vector<int>> NextxList){
    int Moves_size = NextsList.size();
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
    return poss_pi;
}

//行動確率に従って手を選択
int DecideIndex(std::vector<double> pies){
    double tempSum = 0.000000000000000;
    std::uniform_real_distribution<> selecter(0.000000000000000, 1.000000000000000);
    double rand = selecter(rd);
    temprand = rand;
    double compare;
    int picked;
    //printf("%f\n", rand);
    for(int i = 0; i < pies.size(); i++){
        if(i == 0){
            if((0.000000000000000 <= rand)&& (rand < pies[0])){
                return 0;
            }
        }
        if (i != 0){
            tempSum += pies[i-1];
            float compare = tempSum+pies[i];
            if(tempSum <= rand < compare)
                return i-1; 
        }
        if (i == pies.size()-1){
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

//行動確率πの取得
std::vector<double> GetCurrentPi(Geister current){
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
    auto pies = DecidePi(NextsList, NextxList);
    return pies;
}

