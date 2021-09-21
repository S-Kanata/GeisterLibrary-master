#include <string>
#include <vector>
#include "../include/random.hpp"
#include "../include/player.hpp"
#include "numpy.hpp"
#include <iostream>
#include "Encode.hpp"

cpprefjp::random_device rd;
std::mt19937 mt;

std::vector<Hand> legalMoves; //合法手のリスト
std::vector<int> NextS; //次の一手
std::vector<int> NextT; //次の一手デバッグ
std::vector<std::vector<int>> NextsList; //次の一手の集合 sList
std::vector<std::vector<int>> NextxList; //次の一手の特徴量の集合 xList
std::vector<int> NextX;  //特徴量x
std::vector<float> theta;  //行動パラメータθ
std::vector<float> pri_h; //行動優先度h
float max_h; //hの最大値(オーバーフロー抑制目的)
float next_h; //hの準最大値(オーバーフロー抑制目的)
std::vector<float> poss_pi; //行動確率π
std::vector<float> round_pi; //πの偏微分
int isTookUnit = 0; //Playout中に駒を取った回数(累計)
std::vector<std::vector<float>> eval_list;
float temprand;

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
    std::vector<float>::iterator iter = std::max_element(poss_pi.begin(), poss_pi.end());
    int index = std::distance(poss_pi.begin(), iter);
    next_h = -INFINITY;
    for(int p = 0; p < poss_pi.size(); p++){
        if(p != index){
            if(poss_pi[p] > next_h){
                next_h = poss_pi[p];
            }
        }
    }
    printf("行動優先度：%f\n 差分：%f\n", poss_pi[index], poss_pi[index]-next_h);
}

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

void GetCurrentPi(Geister current){
    std::vector<std::vector<int>> NextsList;
    theta = std::vector<float> (8128);
    std::string path_theta = "Player/data.csv";
    //Load_CSV_float(path_theta, theta);
    std::string filepath = "Player/theta900.bin";
    //write_binary_float(filepath, theta);
    read_binary_float(filepath, theta);
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