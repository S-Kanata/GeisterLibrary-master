#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#pragma once

using namespace std;

void write_binary_int(std::string filepath, std::vector<int>& vec){
    std::ofstream fout(filepath, std::ios::out | std::ios::binary);
    fout.write((char*)&vec[0], vec.size()*sizeof(vec[0]));
    fout.close();
    printf("Write Successful!\n");
    return;
}

void read_binary_int(std::string filepath, std::vector<int>& vec){
    int datanum = vec.size();
    vec.resize(datanum);
    std::ifstream fin(filepath, std::ios::in | std::ios::binary);
    fin.read((char*)&vec[0], vec.size()*sizeof(vec[0]));
    fin.close();
    printf("Read Successful!\n");
    return;
}

void write_binary_float(std::string filepath, std::vector<float>& vec){
    std::ofstream fout(filepath, std::ios::out | std::ios::binary);
    fout.write((char*)&vec[0], vec.size()*sizeof(vec[0]));
    fout.close();
    printf("Write Successful!\n");
    return;
}

void read_binary_float(std::string filepath, std::vector<float>& vec){
    int datanum = vec.size();
    vec.resize(datanum);
    std::ifstream fin(filepath, std::ios::in | std::ios::binary);
    fin.read((char*)&vec[0], vec.size()*sizeof(vec[0]));
    fin.close();
    //printf("Read Successful!\n");
    return;
}

void Load_CSV_float(std::string filepath, std::vector<float>& vec){

    //ファイルの読み込み
    std::ifstream ifs(filepath);

    //開かなかったらエラー
    if (!ifs)
    {
        cout << "Error! File can not be opened" << endl;
        return ;
    }

    std::string str = "";
    std::vector<string> strList;
    strList.reserve(8129);
    
    while (getline(ifs, str))
    {      
        strList.push_back(str);
    }

    for(int i = 0; i < strList.size(); i++){
        vec[i] = std::stof(strList[i]);
    }

    //printf("Read Successful!\n");
    return;
}

void Load_CSV_double(std::string filepath, std::vector<double>& vec){

    //ファイルの読み込み
    std::ifstream ifs(filepath);

    //開かなかったらエラー
    if (!ifs)
    {
        cout << "Error! File can not be opened" << endl;
        return ;
    }

    std::string str = "";
    std::vector<string> strList;
    strList.reserve(8129);
    
    while (getline(ifs, str))
    {      
        strList.push_back(str);
    }

    for(int i = 0; i < strList.size(); i++){
        vec[i] = std::stof(strList[i]);
    }

    printf("Read Successful!\n");
    return;
}


/*
bool LoadNpy(const std::string& filePath, std::vector<float>& data)
{
    std::ifstream fs(filePath.c_str(), std::ios::in | std::ios::binary);
    if (fs.fail()) { return false; }

    // header 6 byte = 0x93NUMPY
    unsigned char magicString[6];
    fs.read((char*)&magicString, sizeof(unsigned char) * 6);
    if ((unsigned char)(magicString[0]) != (unsigned char)0x93 ||
        magicString[1] != 'N' || magicString[2] != 'U' ||
        magicString[3] != 'M' || magicString[4] != 'P' ||
        magicString[5] != 'Y') {
        std::cout << "[ERROR] Not NPY file" << std::endl;
        return false;
    }

    unsigned char major, minor;
    fs.read((char*)&major, sizeof(char) * 1);
    fs.read((char*)&minor, sizeof(char) * 1);

    unsigned short headerLength;
    fs.read((char*)&headerLength, sizeof(unsigned short));

    char* header = new char[headerLength];
    fs.read((char*)header, sizeof(char) * headerLength);

    const std::string headerString(header);

    bool checkType = false;
    {
        const size_t pos = headerString.find("'descr': ");
        if (pos != std::string::npos) {
            const size_t typePos = headerString.find("'<f4'", pos);
            if (typePos != std::string::npos) {
                checkType = true;
            } else {
                std::cout << "[ERROR] Type Error." << std::endl;
            }
        }
    }

    bool checkAxis = false;
    int axis1 = -1;
    {
        const size_t pos = headerString.find("'shape': ");
        if (pos != std::string::npos) {
            const size_t shapeStartPos = headerString.find("(", pos);
            const size_t shapeEndPos   = headerString.find(")", pos);

            if (shapeStartPos != std::string::npos && shapeEndPos != std::string::npos) {
                const std::string shapeString = headerString.substr(shapeStartPos, (shapeEndPos - shapeStartPos) + 1);
                ::sscanf(shapeString.c_str(), "(%d,)", &axis1);

                if (axis1 > 0) {
                    checkAxis = true;
                } else {
                    std::cout << "[ERROR] Axis Error." << std::endl;
                }
            }
        }
    }

    if (checkType && checkAxis) {
        data.resize(axis1);

        fs.read((char*)&data[0], sizeof(float) * axis1);
    }

    delete header;
    fs.close();

    return true;
}

bool LoadNpy(const std::string& filePath, std::vector<std::vector<double> >& data)
{
    std::ifstream fs(filePath.c_str(), std::ios::in | std::ios::binary);
    if (fs.fail()) { return false; }

    // header 6 byte = 0x93NUMPY
    unsigned char magicString[6];
    fs.read((char*)&magicString, sizeof(unsigned char) * 6);
    if ((unsigned char)(magicString[0]) != (unsigned char)0x93 ||
        magicString[1] != 'N' || magicString[2] != 'U' ||
        magicString[3] != 'M' || magicString[4] != 'P' ||
        magicString[5] != 'Y') {
        std::cout << "[ERROR] Not NPY file" << std::endl;
        return false;
    }

    unsigned char major, minor;
    fs.read((char*)&major, sizeof(char) * 1);
    fs.read((char*)&minor, sizeof(char) * 1);

    unsigned short headerLength;
    fs.read((char*)&headerLength, sizeof(unsigned short));

    char* header = new char[headerLength];
    fs.read((char*)header, sizeof(char) * headerLength);

    const std::string headerString(header);

    bool checkType = false;
    {
        const size_t pos = headerString.find("'descr': ");
        if (pos != std::string::npos) {
            const size_t typePos = headerString.find("'<f8'", pos);
            if (typePos != std::string::npos) {
                checkType = true;
            } else {
                std::cout << "[ERROR] Type Error." << std::endl;
            }
        }
    }

    bool checkAxis = false;
    int axis1 = -1, axis2 = -1;
    {
        const size_t pos = headerString.find("'shape': ");
        if (pos != std::string::npos) {
            const size_t shapeStartPos = headerString.find("(", pos);
            const size_t shapeEndPos   = headerString.find(")", pos);

            if (shapeStartPos != std::string::npos && shapeEndPos != std::string::npos) {
                const std::string shapeString = headerString.substr(shapeStartPos, (shapeEndPos - shapeStartPos) + 1);
                ::sscanf(shapeString.c_str(), "(%d, %d)", &axis1, &axis2);

                if (axis1 > 0 && axis2 > 0) {
                    checkAxis = true;
                } else {
                    std::cout << "[ERROR] Axis Error." << std::endl;
                }
            }
        }
    }

    if (checkType && checkAxis) {
        data.resize(axis1);
        for (int i = 0; i < axis1; i++) {
            data[i].resize(axis2);
        }

        for (int i = 0; i < axis1; i++) {
            fs.read((char*)&data[i][0], sizeof(double) * axis2);
        }
    }

    delete header;
    fs.close();

    return true;
}
*/