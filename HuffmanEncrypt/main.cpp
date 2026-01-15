#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <queue>
#include <vector>
#include <bitset>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <omp.h>
#include "Sampletxtfile/huffmanCompress.h"
#include "Sampletxtfile/huffmanCompressPar.h"

int main() {

    // File .txt đầu vào và file .huff đầu ra
    std::string input = "C:\\Users\\dinhd\\OneDrive\\Desktop\\HuffmanEncrypt\\Sampletxtfile\\1MB.txt";
    std::string output = "C:\\Users\\dinhd\\OneDrive\\Desktop\\HuffmanEncrypt\\OutputCompressed\\compressed_data.huff";

    HuffmanCompressor hc;
    hc.compress(input, output);

    HuffmanCompressorPar  parCompressor;
    parCompressor.compress(input, output);
    return 0;
}