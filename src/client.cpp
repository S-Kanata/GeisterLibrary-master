#include "nonsugar.hpp"
#include "geister.hpp"
#include "tcpClient.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include <fstream>
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
#if __has_include(<filesystem>)
#define FS_ENABLE
#include <filesystem>
#elif __has_include(<experimental/filesystem>)
#define FS_EXPERIMENTAL_ENABLE
#include <experimental/filesystem>
#endif

#if defined(FS_ENABLE)
namespace fs = std::filesystem;
#elif defined(FS_EXPERIMENTAL_ENABLE)
namespace fs = std::experimental::filesystem;
#endif

bool visibleResponse = false;
int output = 2;
std::array<int, 7> winreason({0,0,0,0,0,0,0});
bool logEnable = false;
std::string logRoot = "log";
std::string dllPath1, dllPath2;
std::string dllName1, dllName2;
#if defined(FS_ENABLE) || defined(FS_EXPERIMENTAL_ENABLE)
std::string logDir;
std::ofstream digestFile;
#endif

#ifdef _WIN32
using HANDLE_TYPE = HMODULE;
#else
using HANDLE_TYPE = void*;
#endif

static std::string getFileName(std::string path){
#if defined(FS_ENABLE) || defined(FS_EXPERIMENTAL_ENABLE)
            return fs::path(path).filename().generic_string();
#else
    int last = 0;
    for(size_t i = 0; i < path.size(); ++i){
        if(path[i] == '/' || path[i] == '\\'){
            last = i+1;
        }
    }
    return std::string(&path[last]);
#endif
}

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
#if defined(FS_ENABLE) || defined(FS_EXPERIMENTAL_ENABLE)
    std::ofstream logFile;
    if(logEnable){
        //時刻取得用
        char fn[256];

        //現在時刻取得
        time_t now = time(NULL);
        struct tm *pnow = localtime(&now);
        sprintf(fn, "%04d-%02d-%02d_%02d-%02d-%02d", pnow->tm_year + 1900, pnow->tm_mon + 1, pnow->tm_mday,
            pnow->tm_hour, pnow->tm_min, pnow->tm_sec);
        std::string filename(fn);

        constexpr const char* ext = ".txt";
        std::string fp = logDir + "/" + dllName1 + "_" + filename + ".0" + ext;
        fs::path filepath(fp);
        for(int i=1; fs::exists(filepath); ++i){
            filepath = fs::path(logDir + "/" + dllName1 + "_" + filename + "." + std::to_string(i) + ext);
        }
        
        logFile.open(filepath, std::ios::out);
    }
#endif
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
#if defined(FS_ENABLE) || defined(FS_EXPERIMENTAL_ENABLE)
                logFile << "SET:" + red_ptn << std::endl;
#endif
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
#if defined(FS_ENABLE) || defined(FS_EXPERIMENTAL_ENABLE)
                auto log = res.substr(4);
                logFile << log << std::endl;
#endif
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
        if(i == 6) {
            printf("%d\n", winreason[i]);
#if defined(FS_ENABLE) || defined(FS_EXPERIMENTAL_ENABLE)
            logFile << winreason[i] << std::endl;
#endif
        } else {
#if defined(FS_ENABLE) || defined(FS_EXPERIMENTAL_ENABLE)
            logFile << winreason[i] << "-";
#endif
            printf("%d-", winreason[i]);
        }
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
            .flag<'l'>({'l'}, {"log"}, "", "enable log record")
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
        if (opts.has<'l'>()) {
            logEnable = true;
        }
        if(opts.get<'d'>().size() > 0){
            libPath = opts.get<'d'>()[0];
            dllPath1 = libPath;
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

        if(port == 10000)
            dllName1 = getFileName(dllPath1) + "_First";
        if(port == 10001)
            dllName1 = getFileName(dllPath1) + "_Second";

#if defined(FS_ENABLE) || defined(FS_EXPERIMENTAL_ENABLE)
    if(logEnable){
        fs::create_directory(logRoot);

        //時刻取得用
        char dn[256];

        //現在時刻取得
        time_t now = time(NULL);
        struct tm *pnow = localtime(&now);
        sprintf(dn, "%04d-%02d-%02d_%02d-%02d-%02d(%d)", pnow->tm_year + 1900, pnow->tm_mon + 1, pnow->tm_mday,
            pnow->tm_hour, pnow->tm_min, pnow->tm_sec, (int)playCount);
        std::string dirName(dn);

        logDir = logRoot + "/" + dllName1 + "/" + dirName;
        fs::create_directories(logDir);

        digestFile.open(logDir + "/" + dllName1 + "_digest.txt", std::ios::out);
    }
#endif

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
