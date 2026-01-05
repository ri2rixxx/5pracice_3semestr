#include <iostream>
#include "server.h"

using namespace std;

int main() {
    try {
        Server server(6379);
        server.start();
    } catch (const exception& e) {
        cerr << "Ошибка: " << e.what() << endl;
        return 1;
    }
    return 0;
}
