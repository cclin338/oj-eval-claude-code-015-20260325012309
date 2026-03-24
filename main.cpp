#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <cstring>
#include <sys/stat.h>

using namespace std;

class FileStorage {
private:
    string storage_dir;
    static const int NUM_FILES = 10;
    static const int COMPACTION_THRESHOLD = 1000;
    int op_count;

    string get_filename(int file_index) {
        return storage_dir + "/storage_" + to_string(file_index) + ".dat";
    }

    int get_file_index(const string& index) {
        hash<string> hasher;
        size_t hash_value = hasher(index);
        return hash_value % NUM_FILES;
    }

    // Compact a single file
    void compact_file(int file_index) {
        string filename = get_filename(file_index);

        // Read all entries and compute current state
        unordered_map<string, vector<int>> current_state;

        ifstream file(filename, ios::binary);
        if (file.is_open()) {
            while (file.peek() != EOF) {
                char op_type;
                file.read(&op_type, sizeof(op_type));
                if (!file) break;

                string key;
                size_t key_len;
                file.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));
                if (!file) break;

                key.resize(key_len);
                file.read(&key[0], key_len);
                if (!file) break;

                int value;
                file.read(reinterpret_cast<char*>(&value), sizeof(value));
                if (!file) break;

                if (op_type == 'I') {
                    // Check if value already exists
                    bool exists = false;
                    for (int v : current_state[key]) {
                        if (v == value) {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists) {
                        current_state[key].push_back(value);
                    }
                } else if (op_type == 'D') {
                    // Remove value
                    auto it = std::find(current_state[key].begin(), current_state[key].end(), value);
                    if (it != current_state[key].end()) {
                        current_state[key].erase(it);
                    }
                }
            }
            file.close();
        }

        // Write compacted state
        ofstream outfile(filename, ios::binary);
        if (outfile.is_open()) {
            for (const auto& entry : current_state) {
                if (!entry.second.empty()) {
                    for (int value : entry.second) {
                        outfile.write("I", 1);

                        size_t key_len = entry.first.size();
                        outfile.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
                        outfile.write(entry.first.c_str(), key_len);

                        outfile.write(reinterpret_cast<const char*>(&value), sizeof(value));
                    }
                }
            }
            outfile.close();
        }
    }

public:
    FileStorage() {
        storage_dir = "storage";
        op_count = 0;
        struct stat st;
        memset(&st, 0, sizeof(st));
        if (stat(storage_dir.c_str(), &st) == -1) {
            mkdir(storage_dir.c_str(), 0755);
        }
    }

    // Append operation to log
    void append_op(int file_index, char op_type, const string& index, int value = 0) {
        string filename = get_filename(file_index);
        ofstream file(filename, ios::binary | ios::app);
        if (file.is_open()) {
            file.write(&op_type, sizeof(op_type));

            size_t key_len = index.size();
            file.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
            file.write(index.c_str(), key_len);

            file.write(reinterpret_cast<const char*>(&value), sizeof(value));
            file.close();
        }
    }

    void insert(const string& index, int value) {
        int file_index = get_file_index(index);
        append_op(file_index, 'I', index, value);

        op_count++;
        if (op_count >= COMPACTION_THRESHOLD) {
            for (int i = 0; i < NUM_FILES; ++i) {
                compact_file(i);
            }
            op_count = 0;
        }
    }

    void delete_entry(const string& index, int value) {
        int file_index = get_file_index(index);
        append_op(file_index, 'D', index, value);

        op_count++;
        if (op_count >= COMPACTION_THRESHOLD) {
            for (int i = 0; i < NUM_FILES; ++i) {
                compact_file(i);
            }
            op_count = 0;
        }
    }

    void find(const string& index) {
        int file_index = get_file_index(index);
        string filename = get_filename(file_index);

        vector<int> values;

        ifstream file(filename, ios::binary);
        if (file.is_open()) {
            while (file.peek() != EOF) {
                char op_type;
                file.read(&op_type, sizeof(op_type));
                if (!file) break;

                string key;
                size_t key_len;
                file.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));
                if (!file) break;

                key.resize(key_len);
                file.read(&key[0], key_len);
                if (!file) break;

                int value;
                file.read(reinterpret_cast<char*>(&value), sizeof(value));
                if (!file) break;

                if (key == index) {
                    if (op_type == 'I') {
                        // Check if value already exists
                        bool exists = false;
                        for (int v : values) {
                            if (v == value) {
                                exists = true;
                                break;
                            }
                        }
                        if (!exists) {
                            values.push_back(value);
                        }
                    } else if (op_type == 'D') {
                        // Remove value
                        auto it = std::find(values.begin(), values.end(), value);
                        if (it != values.end()) {
                            values.erase(it);
                        }
                    }
                }
            }
            file.close();
        }

        if (values.empty()) {
            cout << "null" << endl;
            return;
        }

        sort(values.begin(), values.end());

        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) cout << " ";
            cout << values[i];
        }
        cout << endl;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    FileStorage storage;

    int n;
    cin >> n;
    cin.ignore();

    for (int i = 0; i < n; ++i) {
        string command;
        cin >> command;

        if (command == "insert") {
            string index;
            int value;
            cin >> index >> value;
            storage.insert(index, value);
        } else if (command == "delete") {
            string index;
            int value;
            cin >> index >> value;
            storage.delete_entry(index, value);
        } else if (command == "find") {
            string index;
            cin >> index;
            storage.find(index);
        }
    }

    return 0;
}
