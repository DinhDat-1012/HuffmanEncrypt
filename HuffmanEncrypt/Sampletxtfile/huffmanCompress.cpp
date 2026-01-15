//
// Created by dinhd on 12/17/2025.
//

#include "huffmanCompress.h"
#include <fstream>
#include <iomanip>
#include <bitset>

using namespace std;
using namespace std::chrono;

void HuffmanCompressor::encode(Node* root, string str) {
    if (root == nullptr) return;

    if (!root->left && !root->right) {
        huffmanCode[root->ch] = str;
    }

    encode(root->left, str + "0");
    encode(root->right, str + "1");
}

void HuffmanCompressor::deleteTree(Node* node) {
    if (node == nullptr) return;
    deleteTree(node->left);
    deleteTree(node->right);
    delete node;
}

// Ghi bảng tần suất vào đầu file để sau này có thể giải nén (Header)
void HuffmanCompressor::writeHeader(ofstream& outFile) {
    // 1. Ghi số lượng ký tự khác nhau (kích thước map)
    int mapSize = freqMap.size();
    outFile.write(reinterpret_cast<char*>(&mapSize), sizeof(mapSize));

    // 2. Ghi lần lượt từng cặp (Ký tự, Tần suất)
    for (const auto& pair : freqMap) {
        char ch = pair.first;
        int freq = pair.second;
        outFile.write(&ch, sizeof(ch));
        outFile.write(reinterpret_cast<char*>(&freq), sizeof(freq));
    }
}

// Chuyển chuỗi "01011..." thành các byte thực sự và ghi ra file
void HuffmanCompressor::writeBody(ofstream& outFile, const string& encodedStr) {
    char buffer = 0;
    int count = 0;

    // Tính số bit padding (bit thêm vào cuối để đủ 1 byte)
    // Ví dụ: chuỗi bit dài 10 -> cần 2 byte (16 bit) -> padding = 6
    int padding = (8 - (encodedStr.length() % 8)) % 8;
    outFile.write(reinterpret_cast<char*>(&padding), sizeof(padding));

    for (char bit : encodedStr) {
        buffer = buffer << 1; // Dịch trái 1 bit
        if (bit == '1') {
            buffer = buffer | 1; // Bật bit cuối lên 1
        }
        count++;

        // Khi đủ 8 bit (1 byte) thì ghi vào file
        if (count == 8) {
            outFile.put(buffer);
            buffer = 0;
            count = 0;
        }
    }

    // Xử lý các bit còn dư (nếu có)
    if (count > 0) {
        buffer = buffer << (8 - count); // Dịch nốt phần còn thiếu sang trái
        outFile.put(buffer);
    }
}

void HuffmanCompressor::compress(const string& inputFilePath, const string& outputFilePath) {
    cout << "--- BAT DAU QUA TRINH NEN HUFFMAN (TUAN TU) ---" << endl;
    cout << "Input:  " << inputFilePath << endl;
    cout << "Output: " << outputFilePath << endl << endl;

    // --- BƯỚC 1: ĐỌC FILE VÀ TÍNH TẦN SUẤT ---
    auto start = high_resolution_clock::now();

    ifstream inFile(inputFilePath);
    if (!inFile) {
        cerr << "Loi: Khong the mo file input!" << endl;
        return;
    }

    string content((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();

    if (content.empty()) {
        cout << "File trong. Ket thuc." << endl;
        return;
    }

    for (char ch : content) {
        freqMap[ch]++;
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    cout << "[1] Doc file & Tinh tan suat: " << duration.count() << " us" << endl;
    cout << "    So ky tu khac nhau: " << freqMap.size() << endl;

    // --- BƯỚC 2: XÂY DỰNG CÂY HUFFMAN ---
    start = high_resolution_clock::now();

    priority_queue<Node*, vector<Node*>, compare> pq;
    for (auto pair : freqMap) {
        pq.push(new Node(pair.first, pair.second));
    }

    while (pq.size() != 1) {
        Node *left = pq.top(); pq.pop();
        Node *right = pq.top(); pq.pop();

        int sum = left->freq + right->freq;
        Node* newNode = new Node('\0', sum);
        newNode->left = left;
        newNode->right = right;
        pq.push(newNode);
    }
    root = pq.top();

    end = high_resolution_clock::now();
    duration = duration_cast<microseconds>(end - start);
    cout << "[2] Xay dung cay Huffman: " << duration.count() << " us" << endl;

    // --- BƯỚC 3: TẠO BẢNG MÃ HUFFMAN ---
    start = high_resolution_clock::now();
    encode(root, "");
    end = high_resolution_clock::now();
    duration = duration_cast<microseconds>(end - start);
    cout << "[3] Tao bang ma Huffman: " << duration.count() << " us" << endl;

    // --- BƯỚC 4: MÃ HÓA NỘI DUNG (Trong bo nho) ---
    start = high_resolution_clock::now();
    string encodedStr = "";
    for (char ch : content) {
        encodedStr += huffmanCode[ch];
    }
    end = high_resolution_clock::now();
    duration = duration_cast<microseconds>(end - start);
    cout << "[4] Ma hoa du lieu (chuoi bit): " << duration.count() << " us" << endl;

    // --- BƯỚC 5: GHI FILE NHỊ PHÂN (OUTPUT) ---
    start = high_resolution_clock::now();

    ofstream outFile(outputFilePath, ios::binary);
    if (!outFile) {
        cerr << "Loi: Khong the tao file output!" << endl;
        return;
    }

    // Ghi Header (thông tin để giải nén)
    writeHeader(outFile);
    // Ghi Body (dữ liệu nén)
    writeBody(outFile, encodedStr);

    outFile.close();

    end = high_resolution_clock::now();
    duration = duration_cast<microseconds>(end - start);
    cout << "[5] Ghi file Output (.bin): " << duration.count() << " us" << endl;


    // --- TỔNG KẾT ---
    long originalSize = content.length(); // bytes

    // Lấy kích thước file sau nén
    ifstream checkFile(outputFilePath, ios::binary | ios::ate);
    long compressedSize = checkFile.tellg();
    checkFile.close();

    cout << "\n--- KET QUA ---" << endl;
    cout << "Kich thuoc goc:     " << originalSize << " bytes" << endl;
    cout << "Kich thuoc sau nen: " << compressedSize << " bytes" << endl; // Bao gồm cả header

    double ratio = 0;
    if (originalSize > 0)
        ratio = (1.0 - (double)compressedSize / originalSize) * 100;

    cout << "Ty le nen: " << fixed << setprecision(2) << ratio << "%" << endl;
    cout << "----------------------------------------------" << endl;
}