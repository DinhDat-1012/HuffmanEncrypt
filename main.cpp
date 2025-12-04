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

using namespace std;

// Cấu trúc Node cho cây Huffman
struct HuffmanNode {
    char ch;
    int freq;
    HuffmanNode *left, *right;

    HuffmanNode(char c, int f) : ch(c), freq(f), left(nullptr), right(nullptr) {}
};

// So sánh để dùng trong priority_queue (min-heap)
struct Compare {
    bool operator()(HuffmanNode* a, HuffmanNode* b) {
        return a->freq > b->freq;
    }
};

// Đọc file
string readFile(const string& filename) {
    double start_time = omp_get_wtime();

    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Lỗi: Không thể mở file " << filename << endl;
        return "";
    }

    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    double end_time = omp_get_wtime();
    cout << "Thời gian đọc file: " << (end_time - start_time) << " giây" << endl;

    return content;
}

// Ghi file
bool writeFile(const string& filename, const string& content) {
    ofstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Lỗi: Không thể ghi file " << filename << endl;
        return false;
    }

    file << content;
    file.close();
    return true;
}

// Đếm tần suất ký tự với OpenMP
map<char, int> countFrequency(const string& data, int num_threads) {
    double start_time = omp_get_wtime();

    map<char, int> freq;
    int data_size = data.size();

    // Tạo mảng local cho mỗi thread
    vector<map<char, int>> local_freq(num_threads);

    #pragma omp parallel num_threads(num_threads)
    {
        int thread_id = omp_get_thread_num();

        #pragma omp for schedule(static)
        for (int i = 0; i < data_size; i++) {
            local_freq[thread_id][data[i]]++;
        }
    }

    // Gộp kết quả từ các thread
    for (int i = 0; i < num_threads; i++) {
        for (auto& pair : local_freq[i]) {
            freq[pair.first] += pair.second;
        }
    }

    double end_time = omp_get_wtime();
    cout << "Thời gian đếm tần suất: " << (end_time - start_time) << " giây" << endl;

    return freq;
}

// Xây dựng cây Huffman
HuffmanNode* buildHuffmanTree(map<char, int>& freq) {
    double start_time = omp_get_wtime();

    priority_queue<HuffmanNode*, vector<HuffmanNode*>, Compare> pq;

    // Tạo node lá cho mỗi ký tự
    for (auto& pair : freq) {
        pq.push(new HuffmanNode(pair.first, pair.second));
    }

    // Xử lý trường hợp đặc biệt: chỉ có 1 ký tự
    if (pq.size() == 1) {
        HuffmanNode* root = new HuffmanNode('\0', pq.top()->freq);
        root->left = pq.top();
        pq.pop();
        double end_time = omp_get_wtime();
        cout << "Thời gian xây dựng cây Huffman: " << (end_time - start_time) << " giây" << endl;
        return root;
    }

    // Xây dựng cây
    while (pq.size() > 1) {
        HuffmanNode* left = pq.top(); pq.pop();
        HuffmanNode* right = pq.top(); pq.pop();

        HuffmanNode* parent = new HuffmanNode('\0', left->freq + right->freq);
        parent->left = left;
        parent->right = right;

        pq.push(parent);
    }

    double end_time = omp_get_wtime();
    cout << "Thời gian xây dựng cây Huffman: " << (end_time - start_time) << " giây" << endl;

    return pq.top();
}

// Sinh bảng mã Huffman
void buildCodes(HuffmanNode* root, string code, map<char, string>& huffmanCodes) {
    if (!root) return;

    // Node lá chứa ký tự
    if (!root->left && !root->right) {
        huffmanCodes[root->ch] = code.empty() ? "0" : code;
        return;
    }

    buildCodes(root->left, code + "0", huffmanCodes);
    buildCodes(root->right, code + "1", huffmanCodes);
}

// Mã hóa dữ liệu
string encodeData(const string& data, map<char, string>& huffmanCodes, int num_threads) {
    double start_time = omp_get_wtime();

    int data_size = data.size();
    vector<string> encoded_parts(data_size);

    // Mã hóa song song
    #pragma omp parallel for num_threads(num_threads) schedule(static)
    for (int i = 0; i < data_size; i++) {
        encoded_parts[i] = huffmanCodes[data[i]];
    }

    // Ghép các phần lại
    string encoded = "";
    for (const auto& part : encoded_parts) {
        encoded += part;
    }

    double end_time = omp_get_wtime();
    cout << "Thời gian mã hóa: " << (end_time - start_time) << " giây" << endl;

    return encoded;
}

// Chuyển chuỗi bit thành bytes để lưu file
string bitStringToBytes(const string& bitString) {
    string result;

    // Lưu độ dài chuỗi bit ban đầu (8 bytes)
    size_t bitLength = bitString.size();
    result.append(reinterpret_cast<const char*>(&bitLength), sizeof(bitLength));

    // Chuyển từng 8 bit thành 1 byte
    for (size_t i = 0; i < bitString.size(); i += 8) {
        string byte = bitString.substr(i, 8);
        while (byte.size() < 8) byte += '0'; // Padding

        bitset<8> bits(byte);
        result += static_cast<char>(bits.to_ulong());
    }

    return result;
}

// Chuyển bytes thành chuỗi bit
string bytesToBitString(const string& bytes) {
    if (bytes.size() < sizeof(size_t)) return "";

    // Đọc độ dài chuỗi bit ban đầu
    size_t bitLength;
    memcpy(&bitLength, bytes.data(), sizeof(bitLength));

    string bitString = "";
    for (size_t i = sizeof(size_t); i < bytes.size(); i++) {
        bitset<8> bits(static_cast<unsigned char>(bytes[i]));
        bitString += bits.to_string();
    }

    // Cắt về đúng độ dài
    return bitString.substr(0, bitLength);
}

// Giải mã dữ liệu
string decodeData(const string& encodedBits, HuffmanNode* root) {
    double start_time = omp_get_wtime();

    string decoded = "";
    HuffmanNode* current = root;

    for (char bit : encodedBits) {
        if (bit == '0') {
            current = current->left;
        } else {
            current = current->right;
        }

        // Đến node lá
        if (!current->left && !current->right) {
            decoded += current->ch;
            current = root;
        }
    }

    double end_time = omp_get_wtime();
    cout << "Thời gian giải mã: " << (end_time - start_time) << " giây" << endl;

    return decoded;
}

// Serialize cây Huffman để lưu vào file
void serializeTree(HuffmanNode* root, string& result) {
    if (!root) return;

    if (!root->left && !root->right) {
        result += '1'; // Đánh dấu node lá
        result += root->ch;
    } else {
        result += '0'; // Đánh dấu node trong
        serializeTree(root->left, result);
        serializeTree(root->right, result);
    }
}

// Deserialize cây Huffman từ chuỗi
HuffmanNode* deserializeTree(const string& data, int& index) {
    if (index >= data.size()) return nullptr;

    if (data[index] == '1') {
        index++;
        char ch = data[index];
        index++;
        return new HuffmanNode(ch, 0);
    } else {
        index++;
        HuffmanNode* node = new HuffmanNode('\0', 0);
        node->left = deserializeTree(data, index);
        node->right = deserializeTree(data, index);
        return node;
    }
}

// Giải phóng bộ nhớ cây
void deleteTree(HuffmanNode* root) {
    if (!root) return;
    deleteTree(root->left);
    deleteTree(root->right);
    delete root;
}

// Draw line separator
void printLine(char symbol = '=', int length = 70) {
    cout << string(length, symbol) << endl;
}

// Print beautiful header
void printHeader(const string& title) {
    printLine('=');
    int padding = (70 - title.length()) / 2;
    cout << string(padding, ' ') << title << endl;
    printLine('=');
}

// Print section
void printSection(const string& title) {
    cout << "\n";
    printLine('-');
    cout << "  " << title << endl;
    printLine('-');
}

// Format file size
string formatSize(size_t bytes) {
    if (bytes < 1024) return to_string(bytes) + " bytes";
    if (bytes < 1024 * 1024) return to_string(bytes / 1024) + " KB";
    return to_string(bytes / (1024 * 1024)) + " MB";
}

int main() {
    // Cấu hình
    int num_threads = 4;

    // Header chính
    printHeader("HUFFMAN COMPRESSION PROGRAM");
    cout << "\n  [*] OpenMP Threads: " << num_threads << endl;
    cout << "  [*] Algorithm: Huffman Coding" << endl;
    cout << "  [*] Mode: Parallel Processing" << endl;

    // Nhập tên file
    string input_filename;
    cout << "\n  Enter input file path: ";
    input_filename = "C:\\Users\\dinhd\\Downloads\\1KB.txt";

    // Nếu user không nhập gì, dùng default
    if (input_filename.empty()) {
        input_filename = "input.txt";
        cout << "  [!] Using default: " << input_filename << endl;
    }

    // ===== BƯỚC 1: ĐỌC DỮ LIỆU =====
    printSection("STEP 1: READING INPUT FILE");
    string data = readFile(input_filename);

    if (data.empty()) {
        cout << "\n  [ERROR] File is empty or cannot be read!" << endl;
        printLine('=');
        return 1;
    }

    cout << "  [+] Original size: " << formatSize(data.size()) << endl;

    // ===== BƯỚC 2: NÉN DỮ LIỆU =====
    printSection("STEP 2: COMPRESSION");

    // Đếm tần suất
    cout << "\n  [>] Counting character frequencies..." << endl;
    map<char, int> freq = countFrequency(data, num_threads);
    cout << "  [+] Unique characters: " << freq.size() << endl;

    // Xây dựng cây Huffman
    cout << "\n  [>] Building Huffman tree..." << endl;
    HuffmanNode* root = buildHuffmanTree(freq);

    // Sinh bảng mã
    map<char, string> huffmanCodes;
    buildCodes(root, "", huffmanCodes);

    // In bảng mã (giới hạn 10 ký tự đầu)
    cout << "\n  [+] Huffman Codes (showing first 10):" << endl;
    int count = 0;
    for (auto& pair : huffmanCodes) {
        if (count++ >= 10) {
            cout << "      ... (and " << (huffmanCodes.size() - 10) << " more)" << endl;
            break;
        }
        char display = pair.first;
        if (display == '\n') cout << "      '\\n'";
        else if (display == '\t') cout << "      '\\t'";
        else if (display == ' ') cout << "      ' ' ";
        else cout << "      '" << display << "' ";
        cout << " -> " << pair.second << endl;
    }

    // Mã hóa
    cout << "\n  [>] Encoding data..." << endl;
    string encodedBits = encodeData(data, huffmanCodes, num_threads);
    size_t compressedSize = encodedBits.size() / 8;
    double ratio = (1.0 - (double)compressedSize / data.size()) * 100;

    cout << "  [+] Compressed size: " << formatSize(compressedSize) << " (" << encodedBits.size() << " bits)" << endl;
    cout << "  [+] Compression ratio: " << fixed << setprecision(2) << ratio << "%" << endl;
    cout << "  [+] Space saved: " << formatSize(data.size() - compressedSize) << endl;

    // Serialize cây và dữ liệu nén
    string treeData = "";
    serializeTree(root, treeData);
    string compressedData = treeData + "|" + bitStringToBytes(encodedBits);

    // Ghi file nén
    cout << "\n  [>] Saving compressed file..." << endl;
    writeFile("output.huff", compressedData);
    cout << "  [+] Output: output.huff (" << formatSize(compressedData.size()) << ")" << endl;

    // ===== BƯỚC 3: GIẢI NÉN DỮ LIỆU =====
    printSection("STEP 3: DECOMPRESSION");

    // Đọc file nén
    cout << "\n  [>] Reading compressed file..." << endl;
    string compressedRead = readFile("output.huff");

    // Tách cây và dữ liệu
    size_t delimiter = compressedRead.find('|');
    string treeDataRead = compressedRead.substr(0, delimiter);
    string encodedDataRead = compressedRead.substr(delimiter + 1);

    // Deserialize cây
    cout << "  [>] Rebuilding Huffman tree..." << endl;
    int index = 0;
    HuffmanNode* rootDecoded = deserializeTree(treeDataRead, index);

    // Chuyển bytes về bits
    string encodedBitsRead = bytesToBitString(encodedDataRead);

    // Giải mã
    cout << "  [>] Decoding data..." << endl;
    string decodedData = decodeData(encodedBitsRead, rootDecoded);

    cout << "  [+] Decompressed size: " << formatSize(decodedData.size()) << endl;

    // Kiểm tra tính đúng đắn
    cout << "\n  [>] Verifying integrity..." << endl;
    if (decodedData == data) {
        cout << "  [SUCCESS] Data integrity verified! ✓" << endl;
    } else {
        cout << "  [ERROR] Data mismatch! ✗" << endl;
    }

    // Ghi file giải nén
    writeFile("decoded.txt", decodedData);
    cout << "  [+] Output: decoded.txt" << endl;

    // Hiển thị một phần nội dung
    printSection("PREVIEW (First 100 characters)");
    string preview = decodedData.substr(0, min(100, (int)decodedData.size()));
    cout << "  " << preview;
    if (decodedData.size() > 100) cout << "...";
    cout << endl;

    // Giải phóng bộ nhớ
    deleteTree(root);
    deleteTree(rootDecoded);

    // Footer
    cout << "\n";
    printLine('=');
    cout << "  COMPRESSION COMPLETED SUCCESSFULLY!" << endl;
    printLine('=');

    return 0;
}