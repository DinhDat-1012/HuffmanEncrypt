#ifndef HUFFMAN_COMPRESS_SEQ_H
#define HUFFMAN_COMPRESS_SEQ_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <chrono>
#include <fstream>
#include "huffmanCommon.h"


class HuffmanCompressor {
private:
    std::map<char, int> freqMap;
    std::map<char, std::string> huffmanCode;
    Node* root;

    void encode(Node* root, std::string str);
    void deleteTree(Node* node);

    // Hàm hỗ trợ ghi Header (để sau này có thể giải nén)
    void writeHeader(std::ofstream& outFile);

    // Hàm hỗ trợ ghi body (dữ liệu đã nén)
    void writeBody(std::ofstream& outFile, const std::string& encodedStr);

public:
    HuffmanCompressor() : root(nullptr) {}
    ~HuffmanCompressor() { deleteTree(root); }

    // Cập nhật: Nhận thêm đường dẫn file đầu ra
    void compress(const std::string& inputFilePath, const std::string& outputFilePath);
};

#endif