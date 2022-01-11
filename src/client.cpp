#include "nonsugar.hpp"
#include "geister.hpp"
#include "tcpClient.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include <map>
#include "random.hpp"
#include "hand.hpp"
#include <memory>
#include "player.hpp"
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

bool visibleResponse = false;
int output = 2;
std::array<int, 7> winreason({0,0,0,0,0,0,0});

#ifdef _WIN32
using HANDLE_TYPE = HMODULE;
#else
using HANDLE_TYPE = void*;
#endif

template<class T>
T dynamicLink(HANDLE_TYPE& handle, const char* funcName){
#ifdef _WIN32
    T func=(T)GetProcAddress(handle, funcName);
#else
    T func=(T)dlsym(handle, funcName);
#endif
    if(!func){
        std::cerr << "cant call " << funcName << std::endl;
        exit(1);
    }
    return func;
}

int run(TCPClient& client, std::shared_ptr<Player> player){
    client.connect(10);
    
    Geister game;
    int turn = 0;
    player->initialize();

    std::string res;
    while(true){
        res = client.recv();
        res.erase(res.size()-2);
        std::string_view header(res.data(), 3);
        if(visibleResponse && output > 1)
            std::cout << res << std::endl;
        if(header == "SET"){
            std::string red_ptn = player->decideRed();
            if(output > 1)
                std::cout << red_ptn << std::endl;
            client.send("SET:" + red_ptn + "\r\n");
        }
        if(header == "WON" || header == "LST" || header == "DRW"){
            break;
        }
        if(header == "MOV"){
            auto res_temp = res.substr(0, 28);
            auto res_temp_sub = res.substr(28);
            std::replace(res_temp.begin(), res_temp.end(), 'r', 'R');
            std::replace(res_temp.begin(), res_temp.end(), 'b', 'B');
            res = res_temp + res_temp_sub;
            game.setState(res.substr(4));
            turn += 1;
            if(output > 2)
                game.printBoard();
            Hand hand = Hand(player->decideHand(game));
            std::string name{hand.unit.name()};
            std::string direct{hand.direct.toChar()};
            if(output > 1)
                std::cout << name << " " << direct << " " << game.toString() << std::endl;
            client.move(name, direct);
        }
    }
    std::string_view result(res.data(), 3);
    game.setState(std::string_view(&(res[4])));
    auto enemy = game;
    Result reason = enemy.result();
    int resultInt = reason == Result::Draw ? 0 : static_cast<int>(reason);
    if(resultInt == 1) winreason[0]++;
    if(resultInt == 2) winreason[1]++;
    if(resultInt == 3) winreason[2]++;
    if(resultInt == -1){
        if(enemy.exist1st(8,8)) winreason[0]++;
        else if(enemy.exist2nd(8,8)) winreason[3]++;
    } 
    if(resultInt == -2) winreason[4]++;
    if(resultInt == -3) winreason[5]++;
    player->finalize(game);
    std::map<char, double> score = {{'W', 1}, {'L', 0}, {'D', 0.1}};
    if(output > 1)
        game.printBoard();
    if(output)
        std::cout << result << ": " << turn << std::endl;
    
    for (int i = 0; i < 7; i++){
        if(i == 6)
            printf("%d\n", winreason[i]);
        else 
            printf("%d-", winreason[i]);
        fflush(stdout);
        //std::cout << winreason[i];
    }
    client.close();
    return score[result[0]];
}

int main(int argc, char** argv){
    using namespace nonsugar;

    std::string host = "localhost";
    int port = 10001;
    std::string libPath;
    
    uint64_t playCount = 1;

    try {
        auto const cmd = command<char>("client", "geister client")
            .flag<'h'>({'h'}, {"help"}, "", "produce help message")
            .flag<'v'>({'v'}, {"version"}, "", "print version string")
            .flag<'p', int>({'p'}, {"port"}, "N", "connect port")
            .flag<'H', std::string>({'H'}, {"host"}, "HOST_NAME", "connect host")
            .flag<'c', uint64_t>({'c'}, {"play"}, "N", "play count")
            .flag<'r'>({'r'}, {"visible"}, "", "visible response")
            .flag<'o', int>({'o'}, {"output"}, "N", "output level")
            .argument<'d', std::vector<std::string>>("Library-Path")
            ;
        auto const opts = parse(argc, argv, cmd, argument_order::flexible);
        if (opts.has<'h'>()) {
            std::cout << usage(cmd);
            return 0;
        }
        if (opts.has<'v'>()) {
            std::cout << "client, version 1.0\n";
            return 0;
        }
        if (opts.has<'p'>()) {
            port = opts.get<'p'>();
        }
        if (opts.has<'H'>()) {
            host = opts.get<'H'>();
        }
        if (opts.has<'c'>()) {
            playCount = opts.get<'c'>();
        }
        if (opts.has<'o'>()) {
            output = opts.get<'o'>();
        }
        if (opts.has<'r'>()) {
            visibleResponse = true;
        }
        if(opts.get<'d'>().size() > 0){
            libPath = opts.get<'d'>()[0];
        }
        else{
            std::cerr << usage(cmd);
            return 1;
        }
        std::cout << "\n";
    } catch (error const &e) {
        std::cerr << e.message() << "\n";
        return 1;
    }

    HANDLE_TYPE handle;
#ifdef _WIN32
    handle=LoadLibrary(libPath.c_str());
#else
    handle=dlopen(libPath.c_str(), RTLD_LAZY);
#endif
    if(!handle){
        std::cerr << "cant open lib file" << std::endl;
        exit(1);
    }

    {
        using T= Player* (*)();
        T createPlayer = dynamicLink<T>(handle, "createPlayer");
        std::shared_ptr<Player> player(createPlayer());

        TCPClient client(host, port);

        for (size_t i = 0; i < playCount; ++i)
        {
            run(client, player);
            std::this_thread::sleep_for(std::chrono::microseconds(10000));
        }
    }


#ifdef _WIN32
    FreeLibrary(handle);
#else
    dlclose(handle);
#endif
}
