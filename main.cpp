#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <cstring>
#include <sys/stat.h>

using namespace std;

class FileStorage {
private:
    string storage_dir;

    // Create a safe filename from the index
    string get_filename(const string& index) {
        string filename = storage_dir + "/";
        for (char c : index) {
            if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||
                c == '"' || c == '<' || c == '>' || c == '|') {
                filename += '_';
            } else {
                filename += c;
            }
        }
        filename += ".dat";
        return filename;
    }

    // Check if file exists
    bool file_exists(const string& filename) {
        struct stat buffer;
        return (stat(filename.c_str(), &buffer) == 0);
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
        string filename = get_filename(index);
        vector<int> values;

        // Read existing values if file exists
        if (file_exists(filename)) {
            ifstream file(filename, ios::binary);
            if (file.is_open()) {
                int val;
                while (file.read(reinterpret_cast<char*>(&val), sizeof(val))) {
                    values.push_back(val);
                }
                file.close();
            }
        }

        // Check if value already exists (no duplicates allowed)
        for (int val : values) {
            if (val == value) {
                return; // Already exists, don't insert
            }
        }

        // Add new value
        values.push_back(value);

        // Write all values back to file
        ofstream file(filename, ios::binary);
        if (file.is_open()) {
            for (int val : values) {
                file.write(reinterpret_cast<const char*>(&val), sizeof(val));
            }
            file.close();
        }
    }

    // Delete a key-value pair
    void delete_entry(const string& index, int value) {
        string filename = get_filename(index);

        if (!file_exists(filename)) {
            return; // File doesn't exist, nothing to delete
        }

        vector<int> values;
        bool found = false;

        // Read existing values
        ifstream file(filename, ios::binary);
        if (file.is_open()) {
            int val;
            while (file.read(reinterpret_cast<char*>(&val), sizeof(val))) {
                if (val == value) {
                    found = true;
                } else {
                    values.push_back(val);
                }
            }
            file.close();
        }

        if (!found) {
            return; // Value not found, nothing to delete
        }

        // Write remaining values back or delete file if empty
        if (values.empty()) {
            remove(filename.c_str());
        } else {
            ofstream file(filename, ios::binary);
            if (file.is_open()) {
                for (int val : values) {
                    file.write(reinterpret_cast<const char*>(&val), sizeof(val));
                }
                file.close();
            }
        }
    }

    // Find all values for a key
    void find(const string& index) {
        string filename = get_filename(index);

        if (!file_exists(filename)) {
            cout << "null" << endl;
            return;
        }

        vector<int> values;

        // Read all values
        ifstream file(filename, ios::binary);
        if (file.is_open()) {
            int val;
            while (file.read(reinterpret_cast<char*>(&val), sizeof(val))) {
                values.push_back(val);
            }
            file.close();
        }

        if (values.empty()) {
            cout << "null" << endl;
            return;
        }

        // Sort values in ascending order
        sort(values.begin(), values.end());

        // Output values
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