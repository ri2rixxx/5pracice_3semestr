#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <map>
#include <mutex>
#include "structures/hash_table.h"
#include "structures/queue.h"
#include "structures/set.h"
#include "structures/stack.h"

using namespace std;

class Database {
private:
    map<string, string> simple_storage;
    map<string, HashTable*> hash_tables;
    map<string, Set*> sets;
    map<string, Stack*> stacks;
    map<string, Queue*> queues;
    string db_file;
    mutex mtx;
    
    void loadFromFile();
    void saveToFile();

public:
    Database(const string& filename = "database.db");
    ~Database();
    
    string processCommand(const std::string& command);

    void set(const string& key, const string& value);
    string get(const string& key);
    bool del(const string& key);
    int incr(const string& key);
    void lpush(const string& key, const string& value);
    string rpop(const string& key);
    string lrange(const string& key, int start, int stop);
    void save();
    string keys();

    string hset(const string& table_name, const string& key, const string& value);
    string hget(const string& table_name, const string& key);
    string hgetall(const string& table_name);
    string hdel(const string& table_name, const string& key);

    string sadd(const string& set_name, const string& value);
    string srem(const string& set_name, const string& value);
    string sismember(const string& set_name, const string& value);
    string smembers(const string& set_name);

    string spush(const string& stack_name, const string& value);
    string spop(const string& stack_name);

    string qpush(const string& queue_name, const string& value);
    string qpop(const string& queue_name);
};

#endif
