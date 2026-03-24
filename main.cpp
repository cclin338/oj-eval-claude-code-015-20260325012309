#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <cstring>
#include <sys/stat.h>

using namespace std;

class FileStorage {
private:
    string storage_dir;
    static const int NUM_FILES = 10;

    // Hash function to determine which file to use
    int get_file_index(const string& index) {
        hash<string> hasher;
        size_t hash_value = hasher(index);
        return hash_value % NUM_FILES;
    }

    // Get filename for a given file index
    string get_filename(int file_index) {
        return storage_dir + "/storage_" + to_string(file_index) + ".dat";
    }

    // Read all entries from a file
    struct Entry {
        string index;
        vector<int> values;
    };

    vector<Entry> read_all_entries(int file_index) {
        vector<Entry> entries;
        string filename = get_filename(file_index);

        ifstream file(filename, ios::binary);
        if (!file.is_open()) {
            return entries;
        }

        while (file.peek() != EOF) {
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
            for (size_t i = 0; i < value_count; ++i) {
                file.read(reinterpret_cast<char*>(&entry.values[i]), sizeof(entry.values[i]));
            }
            if (!file) break;

            entries.push_back(entry);
        }

        file.close();
        return entries;
    }

    // Write all entries to a file
    void write_all_entries(int file_index, const vector<Entry>& entries) {
        string filename = get_filename(file_index);

        ofstream file(filename, ios::binary);
        if (!file.is_open()) {
            return;
        }

        for (const auto& entry : entries) {
            size_t key_len = entry.index.size();
            file.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
            file.write(entry.index.c_str(), key_len);

            size_t value_count = entry.values.size();
            file.write(reinterpret_cast<const char*>(&value_count), sizeof(value_count));
            for (int val : entry.values) {
                file.write(reinterpret_cast<const char*>(&val), sizeof(val));
            }
        }

        file.close();
    }

public:
    FileStorage() {
        storage_dir = "storage";
        // Create storage directory if it doesn't exist
        struct stat st;
        memset(&st, 0, sizeof(st));
        if (stat(storage_dir.c_str(), &st) == -1) {
            mkdir(storage_dir.c_str(), 0755);
        }
    }

    // Insert a key-value pair
    void insert(const string& index, int value) {
        int file_index = get_file_index(index);
        vector<Entry> entries = read_all_entries(file_index);

        // Find if the key already exists
        bool found = false;
        for (auto& entry : entries) {
            if (entry.index == index) {
                // Check if value already exists
                for (int val : entry.values) {
                    if (val == value) {
                        return; // Already exists, don't insert
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
            entries.push_back(new_entry);
        }

        write_all_entries(file_index, entries);
    }

    // Delete a key-value pair
    void delete_entry(const string& index, int value) {
        int file_index = get_file_index(index);
        vector<Entry> entries = read_all_entries(file_index);

        bool modified = false;
        for (auto& entry : entries) {
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
            entries.erase(remove_if(entries.begin(), entries.end(),
                [](const Entry& e) { return e.values.empty(); }), entries.end());

            write_all_entries(file_index, entries);
        }
    }

    // Find all values for a key
    void find(const string& index) {
        int file_index = get_file_index(index);
        vector<Entry> entries = read_all_entries(file_index);

        for (const auto& entry : entries) {
            if (entry.index == index) {
                if (entry.values.empty()) {
                    cout << "null" << endl;
                    return;
                }

                vector<int> sorted_values = entry.values;
                sort(sorted_values.begin(), sorted_values.end());

                for (size_t i = 0; i < sorted_values.size(); ++i) {
                    if (i > 0) cout << " ";
                    cout << sorted_values[i];
                }
                cout << endl;
                return;
            }
        }

        cout << "null" << endl;
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