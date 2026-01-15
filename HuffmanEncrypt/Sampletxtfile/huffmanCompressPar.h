//
// Created by dinhd on 12/17/2025.
//

#ifndef HUFFMANENCRYPT_HUFFMANCOMPRESSPAR_H
#define HUFFMANENCRYPT_HUFFMANCOMPRESSPAR_H


#ifndef HUFFMAN_COMPRESS_PAR_H
#define HUFFMAN_COMPRESS_PAR_H

#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <fstream>
#include <chrono>
#include <omp.h> // Thư viện OpenMP
#include "huffmanCommon.h"


class HuffmanCompressorPar {
private:
    // Dùng mảng 256 phần tử thay vì Map để tối ưu tốc độ truy cập mảng song song
    long freqArray[256] = {0};
    std::string huffmanCode[256]; // Bảng mã dạng mảng để tra cứu nhanh (O(1))
    Node* root;

    void encode(Node* root, std::string str);
    void deleteTree(Node* node);

    void writeHeader(std::ofstream& outFile);
    void writeBody(std::ofstream& outFile, const std::vector<std::string>& encodedChunks);

public:
    HuffmanCompressorPar() : root(nullptr) {}
    ~HuffmanCompressorPar() { deleteTree(root); }

    void compress(const std::string& inputFilePath, const std::string& outputFilePath);
};

#endif


#endif //HUFFMANENCRYPT_HUFFMANCOMPRESSPAR_H