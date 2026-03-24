#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

class FileStorage {
private:
    string storage_dir;
    static const int NUM_FILES = 10;
    static const int BLOCK_SIZE = 4096;
    static const int MAX_ENTRIES_PER_BLOCK = 50;

    string get_filename(int file_index) {
        return storage_dir + "/storage_" + to_string(file_index) + ".dat";
    }

    int get_file_index(const string& index) {
        hash<string> hasher;
        size_t hash_value = hasher(index);
        return hash_value % NUM_FILES;
    }

    struct BlockHeader {
        int next_block;
        int entry_count;
    };

    struct Entry {
        string index;
        vector<int> values;
    };

public:
    FileStorage() {
        storage_dir = "storage";
        struct stat st;
        memset(&st, 0, sizeof(st));
        if (stat(storage_dir.c_str(), &st) == -1) {
            mkdir(storage_dir.c_str(), 0755);
        }
    }

    void insert(const string& index, int value) {
        int file_index = get_file_index(index);
        string filename = get_filename(file_index);

        vector<Entry> all_entries;

        // Read all entries from file
        fstream file(filename, ios::binary | ios::in | ios::out);
        if (file.is_open()) {
            while (file.peek() != EOF) {
                BlockHeader header;
                file.read(reinterpret_cast<char*>(&header), sizeof(header));
                if (!file) break;

                for (int i = 0; i < header.entry_count; ++i) {
                    Entry entry;
                    size_t key_len;
                    file.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));
                    if (!file) break;

                    entry.index.resize(key_len);
                    file.read(&entry.index[0], key_len);
                    if (!file) break;

                    size_t value_count;
                    file.read(reinterpret_cast<char*>(&value_count), sizeof(value_count));
                    if (!file) break;

                    entry.values.resize(value_count);
                    for (size_t j = 0; j < value_count; ++j) {
                        file.read(reinterpret_cast<char*>(&entry.values[j]), sizeof(entry.values[j]));
                    }
                    if (!file) break;

                    all_entries.push_back(entry);
                }

                // Skip to next block
                if (header.next_block != -1) {
                    file.seekg(header.next_block);
                } else {
                    break;
                }
            }
            file.close();
        }

        // Find and update the entry
        bool found = false;
        for (auto& entry : all_entries) {
            if (entry.index == index) {
                for (int val : entry.values) {
                    if (val == value) {
                        return; // Already exists
                    }
                }
                entry.values.push_back(value);
                found = true;
                break;
            }
        }

        if (!found) {
            Entry new_entry;
            new_entry.index = index;
            new_entry.values.push_back(value);
            all_entries.push_back(new_entry);
        }

        // Write all entries back to file
        ofstream outfile(filename, ios::binary);
        if (outfile.is_open()) {
            int current_block = 0;
            size_t entry_idx = 0;

            while (entry_idx < all_entries.size()) {
                BlockHeader header;
                header.next_block = -1;
                header.entry_count = 0;

                int header_pos = current_block;
                outfile.seekp(header_pos);
                outfile.write(reinterpret_cast<const char*>(&header), sizeof(header));

                int data_pos = current_block + sizeof(header);

                while (entry_idx < all_entries.size() && header.entry_count < MAX_ENTRIES_PER_BLOCK) {
                    const Entry& entry = all_entries[entry_idx];
                    size_t key_len = entry.index.size();
                    size_t value_count = entry.values.size();

                    outfile.seekp(data_pos);
                    outfile.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
                    outfile.write(entry.index.c_str(), key_len);
                    outfile.write(reinterpret_cast<const char*>(&value_count), sizeof(value_count));
                    for (int val : entry.values) {
                        outfile.write(reinterpret_cast<const char*>(&val), sizeof(val));
                    }

                    data_pos += sizeof(key_len) + key_len + sizeof(value_count) + value_count * sizeof(int);
                    header.entry_count++;
                    entry_idx++;
                }

                // Update header
                outfile.seekp(header_pos);
                outfile.write(reinterpret_cast<const char*>(&header), sizeof(header));

                // Prepare for next block
                if (entry_idx < all_entries.size()) {
                    current_block += BLOCK_SIZE;
                    header.next_block = current_block;
                    outfile.seekp(header_pos);
                    outfile.write(reinterpret_cast<const char*>(&header), sizeof(header));
                }
            }

            outfile.close();
        }
    }

    void delete_entry(const string& index, int value) {
        int file_index = get_file_index(index);
        string filename = get_filename(file_index);

        vector<Entry> all_entries;

        // Read all entries from file
        fstream file(filename, ios::binary | ios::in | ios::out);
        if (file.is_open()) {
            while (file.peek() != EOF) {
                BlockHeader header;
                file.read(reinterpret_cast<char*>(&header), sizeof(header));
                if (!file) break;

                for (int i = 0; i < header.entry_count; ++i) {
                    Entry entry;
                    size_t key_len;
                    file.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));
                    if (!file) break;

                    entry.index.resize(key_len);
                    file.read(&entry.index[0], key_len);
                    if (!file) break;

                    size_t value_count;
                    file.read(reinterpret_cast<char*>(&value_count), sizeof(value_count));
                    if (!file) break;

                    entry.values.resize(value_count);
                    for (size_t j = 0; j < value_count; ++j) {
                        file.read(reinterpret_cast<char*>(&entry.values[j]), sizeof(entry.values[j]));
                    }
                    if (!file) break;

                    all_entries.push_back(entry);
                }

                if (header.next_block != -1) {
                    file.seekg(header.next_block);
                } else {
                    break;
                }
            }
            file.close();
        }

        // Find and delete the value
        bool modified = false;
        for (auto& entry : all_entries) {
            if (entry.index == index) {
                auto it = std::find(entry.values.begin(), entry.values.end(), value);
                if (it != entry.values.end()) {
                    entry.values.erase(it);
                    modified = true;
                }
                break;
            }
        }

        if (modified) {
            // Remove empty entries
            all_entries.erase(remove_if(all_entries.begin(), all_entries.end(),
                [](const Entry& e) { return e.values.empty(); }), all_entries.end());

            // Write all entries back to file
            ofstream outfile(filename, ios::binary);
            if (outfile.is_open()) {
                int current_block = 0;
                size_t entry_idx = 0;

                while (entry_idx < all_entries.size()) {
                    BlockHeader header;
                    header.next_block = -1;
                    header.entry_count = 0;

                    int header_pos = current_block;
                    outfile.seekp(header_pos);
                    outfile.write(reinterpret_cast<const char*>(&header), sizeof(header));

                    int data_pos = current_block + sizeof(header);

                    while (entry_idx < all_entries.size() && header.entry_count < MAX_ENTRIES_PER_BLOCK) {
                        const Entry& entry = all_entries[entry_idx];
                        size_t key_len = entry.index.size();
                        size_t value_count = entry.values.size();

                        outfile.seekp(data_pos);
                        outfile.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
                        outfile.write(entry.index.c_str(), key_len);
                        outfile.write(reinterpret_cast<const char*>(&value_count), sizeof(value_count));
                        for (int val : entry.values) {
                            outfile.write(reinterpret_cast<const char*>(&val), sizeof(val));
                        }

                        data_pos += sizeof(key_len) + key_len + sizeof(value_count) + value_count * sizeof(int);
                        header.entry_count++;
                        entry_idx++;
                    }

                    outfile.seekp(header_pos);
                    outfile.write(reinterpret_cast<const char*>(&header), sizeof(header));

                    if (entry_idx < all_entries.size()) {
                        current_block += BLOCK_SIZE;
                        header.next_block = current_block;
                        outfile.seekp(header_pos);
                        outfile.write(reinterpret_cast<const char*>(&header), sizeof(header));
                    }
                }

                outfile.close();
            }
        }
    }

    void find(const string& index) {
        int file_index = get_file_index(index);
        string filename = get_filename(file_index);

        vector<int> values;

        fstream file(filename, ios::binary | ios::in);
        if (file.is_open()) {
            while (file.peek() != EOF) {
                BlockHeader header;
                file.read(reinterpret_cast<char*>(&header), sizeof(header));
                if (!file) break;

                for (int i = 0; i < header.entry_count; ++i) {
                    string key;
                    size_t key_len;
                    file.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));
                    if (!file) break;

                    key.resize(key_len);
                    file.read(&key[0], key_len);
                    if (!file) break;

                    size_t value_count;
                    file.read(reinterpret_cast<char*>(&value_count), sizeof(value_count));
                    if (!file) break;

                    if (key == index) {
                        values.resize(value_count);
                        for (size_t j = 0; j < value_count; ++j) {
                            file.read(reinterpret_cast<char*>(&values[j]), sizeof(values[j]));
                        }
                    } else {
                        // Skip values
                        int val;
                        for (size_t j = 0; j < value_count; ++j) {
                            file.read(reinterpret_cast<char*>(&val), sizeof(val));
                        }
                    }
                }

                if (header.next_block != -1) {
                    file.seekg(header.next_block);
                } else {
                    break;
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
    cin.ignore(); // Ignore newline after n

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