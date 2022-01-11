#include "geister.hpp"
#include "unit.hpp"
#include "simulator.hpp"
#include "result.hpp"
#include "Actionprob.hpp"
#pragma once
#include <array>
#include <algorithm>
#include <map>
#ifndef MAXDEPTH
#define MAXDEPTH 50
#endif

cpprefjp::random_device Simulator::rd;
std::mt19937 Simulator::mt(rd());
int MoveCount; //手数
int MoveIndex; //手数

Simulator::Simulator(const Simulator& s): root(s.root), depth(s.depth)
{
};

Simulator::Simulator(const Geister& g): root(g), depth(0)
{
}

Simulator::Simulator(const Geister& g, std::string_view ptn): root(g, "", ptn), depth(0)
{
}

// 可能性のあるすべての相手駒パターンを列挙
std::vector<std::string> Simulator::getLegalPattern() const
{
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
    
    std::vector<std::string> res(70);
    std::vector<char> blue(4);
    std::vector<char> red(4);

    size_t bsize = std::distance(blue.begin(), std::copy_if(Unit::nameList.begin(), Unit::nameList.begin()+8, blue.begin(), [&](const auto u){ return current.allUnit()[u-'A'+8].color().isBlue();}));
    blue.resize(bsize);
    size_t rsize = std::distance(red.begin(), std::copy_if(Unit::nameList.begin(), Unit::nameList.begin()+8, red.begin(), [&](const auto u){ return current.allUnit()[u-'A'+8].color().isRed();}));
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
std::string Simulator::getRandomPattern() const
{
    std::vector<std::string> legalPattern = getLegalPattern();
    std::uniform_int_distribution<int> selector(0, legalPattern.size() - 1);
    return legalPattern[selector(mt)];
}

// 未判明の相手駒色を適当に仮定
void Simulator::setColor(std::string_view ptn){
    current.setColor("", ptn);
}
    
// 未判明の相手駒色を適当に仮定
void Simulator::setColorRandom(){
    static std::uniform_int_distribution<int> BorR(0,1);
    std::string red;
    int assumeTakeBlue = 4;
    int assumeTakeRed = 4;
    for(int i = 8; i < 16; ++i){
        UnitColor color = current.allUnit()[i].color();
        if(color == UnitColor::blue)
            assumeTakeBlue -= 1;
        if(color == UnitColor::red){
            assumeTakeRed -= 1;
            red += std::toupper(current.allUnit()[i].name());
        }
    }
    for(int i = 8; i < 16; ++i){
        if(current.allUnit()[i].color() == UnitColor::unknown){
            if(assumeTakeBlue > 0 && BorR(mt)){
                assumeTakeBlue -= 1;
            }
            else if(assumeTakeRed > 0){
                red += std::toupper(current.allUnit()[i].name());
                assumeTakeRed -= 1;
            }
            else{
                break;
            }
        }
    }
    current.setColor("", red);
}
    
double Simulator::playout(){
    static std::uniform_int_distribution<> selector;
    std::array<Hand, 32> lm;
    while(true){
        if(current.isEnd())
            break;
        // 相手の手番
        // std::vector<Hand> lm2 = current.getLegalMove2nd();
        // selector.param(std::uniform_int_distribution<>::param_type(0, lm2.size() - 1));
        selector.param(std::uniform_int_distribution<>::param_type(0, current.setLegalMove2nd(lm) - 1));
        Hand& m2 = lm[selector(mt)];
        current.move(m2);
        if(current.isEnd())
            break;
        // 自分の手番
        // std::vector<Hand> lm1 = current.getLegalMove1st();
        // selector.param(std::uniform_int_distribution<>::param_type(0, lm1.size() - 1));
        selector.param(std::uniform_int_distribution<>::param_type(0, current.setLegalMove1st(lm) - 1));
        Hand& m1 = lm[selector(mt)];
        current.move(m1);
    }
    return evaluate();
}

double Simulator::playout_WithProb(){
    std::vector<Hand> lm;
    std::vector<Hand> enemylm;
    std::vector<double> currentpies;
    currentpies.reserve(32);
    lm.reserve(32);
    enemylm.reserve(32);
    Hand m1, m2;
    MaxDepth = MAXDEPTH;
    isMyturn = true;
    int canEscape = 0;

    for(int c = 0; c < MaxDepth; c++){
        
        if(current.isEnd()){ //終了判定
            isMyturn = true;
            break;
        }
        // 相手の手番
        current.changeSide();
        currentpies = GetCurrentPi(current);
        int index = DecideIndex(currentpies);
        enemylm = current.getLegalMove1st();

        canEscape = CanEscape(current);

        m1 = enemylm[index];
        if(canEscape > 0) {
            m1 = EscapeHand(current, canEscape);
        } 

        current.move(m1);

        if(current.isEnd()){ //終了判定
            isMyturn = false;
            break;
        }

        // 自分の手番
        current.changeSide();
        currentpies = GetCurrentPi(current);

        int index2 = DecideIndex(currentpies);
        lm = current.getLegalMove1st();

        canEscape = CanEscape(current);
        
        m2 = lm[index2];
        if(canEscape > 0){
            m2 = EscapeHand(current, canEscape);
        } 


        current.move(m2);

    }
    return evaluate_forProb(isMyturn);
}

double Simulator::playout_WithNotCheckEscape(){
    std::vector<Hand> lm;
    std::vector<Hand> enemylm;
    std::vector<double> currentpies;
    currentpies.reserve(32);
    lm.reserve(32);
    enemylm.reserve(32);
    Hand m1, m2;
    MaxDepth = MAXDEPTH;
    isMyturn = true;

    for(int c = 0; c < MaxDepth; c++){
        
        if(current.isEnd()){ //終了判定
            isMyturn = true;
            break;
        }
        // 相手の手番
        current.changeSide();
        currentpies = GetCurrentPi(current);
        int index = DecideIndex(currentpies);
        enemylm = current.getLegalMove1st();
        m1 = enemylm[index];

        current.move(m1);

        if(current.isEnd()){ //終了判定
            isMyturn = false;
            break;
        }

        // 自分の手番
        current.changeSide();
        currentpies = GetCurrentPi(current);

        int index2 = DecideIndex(currentpies);
        lm = current.getLegalMove1st();
        
        m2 = lm[index2];

        current.move(m2);

    }
    return evaluate_forProb(isMyturn);
}

double Simulator::run(const size_t count){
    double result = 0.0;
    for(size_t i = 0; i < count; ++i){
        current = root;
        setColor(getRandomPattern());
        // current.printBoard();
        // std::cout << current.result() << "\n";
        // setColorRandom();
        result += playout();
    }
        return result;
}

double Simulator::run_WithPlob(const size_t count){
    settheta();
    double result = 0.0;
    for(size_t i = 0; i < count; ++i){
        current = root;
        setColor(getRandomPattern());
        result += playout_WithProb();
    }
        return result;
}


double Simulator::run(std::string_view ptn, const size_t count){
    double result = 0.0;
    for(size_t i = 0; i < count; ++i){
        current = root;
        setColor(ptn);
        result += playout();
    }
    return result;
}

double Simulator::run_WithPlob(std::string_view ptn, const size_t count){
    double result = 0.0;
    for(size_t i = 0; i < count; ++i){
        current = root;
        setColor(ptn);
        result += playout_WithProb();
    }
    return result;
}

double Simulator::run_WithNotCheckEscape(const size_t count){
    settheta();
    double result = 0.0;
    for(size_t i = 0; i < count; ++i){
        current = root;
        setColor(getRandomPattern());
        result += playout_WithNotCheckEscape();
    }
        return result;
}

double Simulator::run_WithNotCheckEscape(std::string_view ptn, const size_t count){
    double result = 0.0;
    for(size_t i = 0; i < count; ++i){
        current = root;
        setColor(ptn);
        result += playout_WithNotCheckEscape();
    }
    return result;
}





