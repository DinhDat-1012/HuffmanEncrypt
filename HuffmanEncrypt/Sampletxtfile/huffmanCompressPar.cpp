#include "huffmanCompressPar.h"
#include <iomanip>
#include <vector>
#include <cstring> // cho memset

using namespace std;
using namespace std::chrono;

void HuffmanCompressorPar::encode(Node* root, string str) {
    if (root == nullptr) return;

    if (!root->left && !root->right) {
        // Lưu mã vào mảng tra cứu direct access thay vì map
        huffmanCode[(unsigned char)root->ch] = str;
    }

    encode(root->left, str + "0");
    encode(root->right, str + "1");
}

void HuffmanCompressorPar::deleteTree(Node* node) {
    if (node == nullptr) return;
    deleteTree(node->left);
    deleteTree(node->right);
    delete node;
}

void HuffmanCompressorPar::writeHeader(ofstream& outFile) {
    // Đếm số lượng ký tự có tần suất > 0
    int mapSize = 0;
    for(int i=0; i<256; i++) {
        if(freqArray[i] > 0) mapSize++;
    }

    outFile.write(reinterpret_cast<char*>(&mapSize), sizeof(mapSize));

    for (int i = 0; i < 256; i++) {
        if (freqArray[i] > 0) {
            char ch = (char)i;
            int freq = freqArray[i];
            outFile.write(&ch, sizeof(ch));
            outFile.write(reinterpret_cast<char*>(&freq), sizeof(freq));
        }
    }
}

void HuffmanCompressorPar::writeBody(ofstream& outFile, const vector<string>& encodedChunks) {
    // Phần này vẫn phải làm tuần tự để ghi đúng thứ tự byte vào file
    char buffer = 0;
    int count = 0;

    // Tính tổng độ dài bit để tính padding trước
    long long totalBits = 0;
    for(const auto& s : encodedChunks) totalBits += s.length();

    int padding = (8 - (totalBits % 8)) % 8;
    outFile.write(reinterpret_cast<char*>(&padding), sizeof(padding));

    // Duyệt qua từng chunk (được tạo ra bởi các luồng) và ghi vào file
    for (const string& chunk : encodedChunks) {
        for (char bit : chunk) {
            buffer = buffer << 1;
            if (bit == '1') buffer = buffer | 1;
            count++;
            if (count == 8) {
                outFile.put(buffer);
                buffer = 0;
                count = 0;
            }
        }
    }

    if (count > 0) {
        buffer = buffer << (8 - count);
        outFile.put(buffer);
    }
}

void HuffmanCompressorPar::compress(const string& inputFilePath, const string& outputFilePath) {
    cout << "--- BAT DAU QUA TRINH NEN HUFFMAN (SONG SONG - OPENMP) ---" << endl;

    // Thiết lập số lượng luồng (Threads)
    int numThreads = omp_get_max_threads();
    cout << "So luong luong (Threads) su dung: " << numThreads << endl;
    cout << "Input:  " << inputFilePath << endl;
    cout << "Output: " << outputFilePath << endl << endl;

    // --- BƯỚC 1: ĐỌC FILE ---
    auto start = high_resolution_clock::now();
    ifstream inFile(inputFilePath, ios::binary); // Đọc binary để an toàn với mọi ký tự
    if (!inFile) { cerr << "Loi mo file input" << endl; return; }

    // Di chuyển con trỏ đến cuối để lấy kích thước
    inFile.seekg(0, ios::end);
    long fileSize = inFile.tellg();
    inFile.seekg(0, ios::beg);

    // Đọc toàn bộ file vào buffer bộ nhớ
    string content;
    content.resize(fileSize);
    inFile.read(&content[0], fileSize);
    inFile.close();

    if (content.empty()) return;
    auto end = high_resolution_clock::now();
    cout << "[1] Doc file vao RAM: " << duration_cast<microseconds>(end - start).count() << " us" << endl;


    // --- BƯỚC 2: TÍNH TẦN SUẤT (SONG SONG) ---
    // KỸ THUẬT: Data Parallelism (Phân chia dữ liệu) + Reduction (Gộp kết quả)
    start = high_resolution_clock::now();

    #pragma omp parallel
    {
        long localFreq[256] = {0}; // Mảng cục bộ cho mỗi luồng (Private)

        // Phân chia vòng lặp cho các luồng
        #pragma omp for schedule(static)
        for (long i = 0; i < fileSize; i++) {
            localFreq[(unsigned char)content[i]]++;
        }

        // Gộp kết quả cục bộ vào mảng toàn cục (Critical Section)
        #pragma omp critical
        {
            for (int i = 0; i < 256; i++) {
                freqArray[i] += localFreq[i];
            }
        }
    }

    end = high_resolution_clock::now();
    cout << "[2] Tinh tan suat (Song song): " << duration_cast<microseconds>(end - start).count() << " us" << endl;


    // --- BƯỚC 3: XÂY DỰNG CÂY VÀ BẢNG MÃ (TUẦN TỰ) ---
    // Bước này rất nhanh và khó song song hóa hiệu quả do phụ thuộc dữ liệu
    start = high_resolution_clock::now();
    priority_queue<Node*, vector<Node*>, compare> pq;
    for (int i = 0; i < 256; i++) {
        if (freqArray[i] > 0)
            pq.push(new Node((char)i, freqArray[i]));
    }

    if (pq.empty()) return;

    while (pq.size() != 1) {
        Node *left = pq.top(); pq.pop();
        Node *right = pq.top(); pq.pop();
        Node* newNode = new Node('\0', left->freq + right->freq);
        newNode->left = left; newNode->right = right;
        pq.push(newNode);
    }
    root = pq.top();
    encode(root, ""); // Tạo bảng mã
    end = high_resolution_clock::now();
    cout << "[3] Xay dung cay & bang ma (Tuan tu): " << duration_cast<microseconds>(end - start).count() << " us" << endl;


    // --- BƯỚC 4: MÃ HÓA DỮ LIỆU (SONG SONG) ---
    // KỸ THUẬT: Data Decomposition (Chia nhỏ chuỗi đầu vào)
    start = high_resolution_clock::now();

    // Mỗi luồng sẽ lưu kết quả mã hóa của phần dữ liệu mình phụ trách vào đây
    vector<string> partialResults(numThreads);

    #pragma omp parallel
    {
        int tid = omp_get_thread_num(); // Lấy ID của luồng hiện tại
        int n_threads = omp_get_num_threads();

        // Chia file thành các đoạn (chunk) cho mỗi luồng
        long chunkSize = fileSize / n_threads;
        long startIdx = tid * chunkSize;
        long endIdx = (tid == n_threads - 1) ? fileSize : startIdx + chunkSize;

        string localEncoded = "";
        // Dự trữ bộ nhớ để tránh cấp phát liên tục (Heuristic: 1 char ~ 8 bits nhưng trung bình Huffman < 8)
        localEncoded.reserve((endIdx - startIdx) * 5);

        for (long i = startIdx; i < endIdx; i++) {
            // Tra cứu trong mảng huffmanCode (nhanh hơn map)
            localEncoded += huffmanCode[(unsigned char)content[i]];
        }

        partialResults[tid] = localEncoded;
    }

    end = high_resolution_clock::now();
    cout << "[4] Ma hoa du lieu (Song song): " << duration_cast<microseconds>(end - start).count() << " us" << endl;


    // --- BƯỚC 5: GHI FILE (TUẦN TỰ) ---
    start = high_resolution_clock::now();
    ofstream outFile(outputFilePath, ios::binary);
    writeHeader(outFile);
    writeBody(outFile, partialResults); // Hàm này sẽ ghép các mảnh lại
    outFile.close();
    end = high_resolution_clock::now();
    cout << "[5] Ghi file Output: " << duration_cast<microseconds>(end - start).count() << " us" << endl;

    cout << "----------------------------------------------" << endl;
}