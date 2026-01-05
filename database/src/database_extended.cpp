#include "database.h"
#include <sstream>
#include <iostream>
#include <vector>

using namespace std;

string Database::hset(const string& table_name, const string& key, const string& value) {
    lock_guard<mutex> lock(mtx);
    
    if (hash_tables.find(table_name) == hash_tables.end()) {
        hash_tables[table_name] = new HashTable();
        initHashTable(*hash_tables[table_name], table_name);
    }
    
    bool result = hashSet(*hash_tables[table_name], key, value);
    return result ? "OK" : "ERROR";
}

string Database::hget(const string& table_name, const string& key) {
    lock_guard<mutex> lock(mtx);
    
    if (hash_tables.find(table_name) == hash_tables.end()) {
        return "(nil)";
    }
    
    string result = hashGet(*hash_tables[table_name], key);
    return result.empty() ? "(nil)" : result;
}

string Database::hgetall(const string& table_name) {
    lock_guard<mutex> lock(mtx);
    
    if (hash_tables.find(table_name) == hash_tables.end()) {
        return "(empty)";
    }
    
    ostringstream oss;
    HashTable* table = hash_tables[table_name];
    bool first = true;
    
    for (int i = 0; i < table->capacity; i++) {
        HashEntry* entry = table->buckets[i];
        while (entry != nullptr) {
            if (!first) oss << "\n";
            oss << entry->key << ": " << entry->value;
            first = false;
            entry = entry->next;
        }
    }
    
    return oss.str();
}

string Database::hdel(const string& table_name, const string& key) {
    lock_guard<mutex> lock(mtx);
    
    if (hash_tables.find(table_name) == hash_tables.end()) {
        return "0";
    }
    
    bool result = hashDelete(*hash_tables[table_name], key);
    return result ? "1" : "0";
}

string Database::sadd(const string& set_name, const string& value) {
    lock_guard<mutex> lock(mtx);
    
    if (sets.find(set_name) == sets.end()) {
        sets[set_name] = new Set();
        initSet(*sets[set_name], set_name);
    }
    
    bool result = setAdd(*sets[set_name], value);
    return result ? "1" : "0";
}

string Database::srem(const string& set_name, const string& value) {
    lock_guard<mutex> lock(mtx);
    
    if (sets.find(set_name) == sets.end()) {
        return "0";
    }
    
    bool result = setRemove(*sets[set_name], value);
    return result ? "1" : "0";
}

string Database::sismember(const string& set_name, const string& value) {
    lock_guard<mutex> lock(mtx);
    
    if (sets.find(set_name) == sets.end()) {
        return "0";
    }
    
    bool result = setContains(*sets[set_name], value);
    return result ? "1" : "0";
}

string Database::smembers(const string& set_name) {
    lock_guard<mutex> lock(mtx);
    
    if (sets.find(set_name) == sets.end()) {
        return "(empty)";
    }
    
    ostringstream oss;
    Set* set = sets[set_name];
    bool first = true;
    
    for (int i = 0; i < set->capacity; i++) {
        SetNode* node = set->buckets[i];
        while (node != nullptr) {
            if (!first) oss << " ";
            oss << node->value;
            first = false;
            node = node->next;
        }
    }
    
    return oss.str();
}

string Database::spush(const string& stack_name, const string& value) {
    lock_guard<mutex> lock(mtx);
    
    if (stacks.find(stack_name) == stacks.end()) {
        stacks[stack_name] = new Stack();
        initStack(*stacks[stack_name], stack_name);
    }
    
    pushStack(*stacks[stack_name], value);
    return "OK";
}

string Database::spop(const string& stack_name) {
    lock_guard<mutex> lock(mtx);
    
    if (stacks.find(stack_name) == stacks.end() || isEmptyStack(*stacks[stack_name])) {
        return "(nil)";
    }
    
    return popStack(*stacks[stack_name]);
}

string Database::qpush(const string& queue_name, const string& value) {
    lock_guard<mutex> lock(mtx);
    
    if (queues.find(queue_name) == queues.end()) {
        queues[queue_name] = new Queue();
        initQueue(*queues[queue_name], queue_name);
    }
    
    enqueue(*queues[queue_name], value);
    return "OK";
}

string Database::qpop(const string& queue_name) {
    lock_guard<mutex> lock(mtx);
    
    if (queues.find(queue_name) == queues.end() || isEmptyQueue(*queues[queue_name])) {
        return "(nil)";
    }
    
    return dequeue(*queues[queue_name]);
}
