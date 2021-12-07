#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include "geister.hpp"
#include <random>
#include "random.hpp"
#include "result.hpp"
#include <string_view>
#pragma once

class Simulator{
protected:
    static cpprefjp::random_device rd;
    static std::mt19937 mt;
public:
    Geister root;
    Geister current;
    
    int depth;

    Simulator(const Simulator&);
    Simulator(const Geister& geister);
    Simulator(const Geister& geister, std::string_view ptn);
    
    virtual std::vector<std::string> getLegalPattern() const;
    // 未判明の相手駒色を適当に仮定
    virtual std::string getRandomPattern() const;

    virtual void setColor(std::string_view ptn);
    
    virtual void setColorRandom();

    virtual double playout();
    virtual double playout_WithProb();


    
    virtual double run(const size_t count = 1);

    virtual double run_WithPlob(const size_t count = 1);

    virtual double run_WithPlob(std::string_view, const size_t count = 1);

    virtual double run(std::string_view, const size_t count = 1);


    virtual double evaluate() const
    {
        return current.result() == Result::Draw ? 0 : static_cast<int>(current.result()) > 0 ? 1.0 : -1.0;
    }

    virtual double evaluate_forProb(bool isMyturn) const
    {
        auto evalboard = current;
        if(!isMyturn) evalboard.changeSide();
        if (evalboard.result() == Result::OnPlay || evalboard.result() == Result::Draw) 
            return 0;
        if (static_cast<int>(evalboard.result()) > 0)         
            return 1.0;
        if (static_cast<int>(evalboard.result()) < 0)
            return -1.0;
    }
        

};

#endif