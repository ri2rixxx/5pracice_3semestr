#include "database.h"
#include "structures/queue.h"
#include <sstream>
#include <vector>

using namespace std;

void Database::set(const string& key, const string& value) {
    lock_guard<mutex> lock(mtx);
    simple_storage[key] = value;
}

string Database::get(const string& key) {
    lock_guard<mutex> lock(mtx);
    auto it = simple_storage.find(key);
    return (it != simple_storage.end()) ? it->second : "";
}

bool Database::del(const string& key) {
    lock_guard<mutex> lock(mtx);
    return simple_storage.erase(key) > 0;
}

int Database::incr(const string& key) {
    lock_guard<mutex> lock(mtx);
    int value = 0;
    auto it = simple_storage.find(key);
    if (it != simple_storage.end()) {
        try {
            value = stoi(it->second);
        } catch (...) {
            value = 0;
        }
    }
    value++;
    simple_storage[key] = to_string(value);
    return value;
}

void Database::lpush(const string& key, const string& value) {
    lock_guard<mutex> lock(mtx);
    if (queues.find(key) == queues.end()) {
        queues[key] = new Queue();
        initQueue(*queues[key], key);
    }
    enqueue(*queues[key], value);
}

string Database::rpop(const string& key) {
    lock_guard<mutex> lock(mtx);
    if (queues.find(key) == queues.end() || isEmptyQueue(*queues[key])) {
        return "";
    }
    return dequeue(*queues[key]);
}

string Database::lrange(const string& key, int start, int stop) {
    lock_guard<mutex> lock(mtx);
    if (queues.find(key) == queues.end()) {
        return "[]";
    }
    
    Queue* q = queues[key];
    vector<string> elements;

    for (int i = 0; i < q->size; i++) {
        int index = (q->front + i) % q->capacity;
        elements.push_back(q->data[index]);
    }

    int n = elements.size();
    if (start < 0) start = n + start;
    if (stop < 0) stop = n + stop;
    if (start < 0) start = 0;
    if (stop >= n) stop = n - 1;
    if (start > stop) return "[]";
    
    ostringstream oss;
    oss << "[";
    for (int i = start; i <= stop; i++) {
        if (i > start) oss << ", ";
        oss << elements[i];
    }
    oss << "]";
    return oss.str();
}

void Database::save() {
    saveToFile();
}

string Database::keys() {
    lock_guard<mutex> lock(mtx);
    ostringstream oss;
    bool first = true;
    for (const auto& pair : simple_storage) {
        if (!first) oss << "\n";
        oss << pair.first;
        first = false;
    }
    return oss.str();
}
