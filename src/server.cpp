
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <thread>
#include <mutex>
#include <chrono>
#include <fstream>

// Database
std::unordered_map<std::string, std::string> database;

// Expiry time storage
std::unordered_map<std::string,
    std::chrono::steady_clock::time_point> expiry;

// Mutex for thread safety
std::mutex db_mutex;

void saveToAOF(const std::string& command) {

    std::ofstream file("appendonly.aof", std::ios::app);

    if (file.is_open()) {

        file << command << "\n";

        file.close();
    }
}
void loadFromAOF() {

    std::ifstream file("appendonly.aof");

    std::string line;

    while (std::getline(file, line)) {

        std::istringstream stream(line);

        std::string command;
        std::string key;
        std::string value;

        stream >> command >> key;

        if (command == "SET") {

            stream >> value;

            database[key] = value;

            expiry.erase(key);
        }

        else if (command == "DEL") {

            database.erase(key);

            expiry.erase(key);
        }
        else if (command == "FLUSHALL") {
    database.clear();
    expiry.clear();
}
    else if (command == "PERSIST") {
    expiry.erase(key);
}
        else if (command == "EXPIREAT") {

            long long expiry_timestamp;

            stream >> expiry_timestamp;

            long long current_timestamp =
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count();

            long long remaining =
                expiry_timestamp - current_timestamp;

            if (remaining > 0 &&
                database.find(key) != database.end()) {

                expiry[key] =
                    std::chrono::steady_clock::now()
                    + std::chrono::seconds(remaining);
            }

            else {

                database.erase(key);
                expiry.erase(key);
            }
        }
    }

    file.close();

    std::cout << "AOF data loaded successfully!\n";
}
// RESP Command Parser
std::vector<std::string> parseRESP(const std::string& request) {

    std::vector<std::string> args;
    std::istringstream stream(request);
    std::string line;

    if (!std::getline(stream, line))
        return args;

    if (!line.empty() && line.back() == '\r')
        line.pop_back();

    if (line.empty() || line[0] != '*')
        return args;

    int count;

    try {
        count = std::stoi(line.substr(1));
    }
    catch (...) {
        return args;
    }

    if (count < 1 || count > 100)
        return args;

    for (int i = 0; i < count; i++) {

        if (!std::getline(stream, line))
            return {};

        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (line.empty() || line[0] != '$')
            return {};

        if (!std::getline(stream, line))
            return {};

        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        args.push_back(line);
    }

    return args;
}


// Handle Client Requests
void handleClient(int client_fd) {

    char buffer[1024];

    ssize_t bytes_received = recv(
        client_fd,
        buffer,
        sizeof(buffer) - 1,
        0
    );

    if (bytes_received > 0) {

        buffer[bytes_received] = '\0';

        std::string request(buffer, bytes_received);

        std::vector<std::string> args = parseRESP(request);


        // PING Command
        if (args.size() == 1 && args[0] == "PING") {

            const char* response = "+PONG\r\n";

            send(client_fd, response, strlen(response), 0);
        }


        // SET Command
        else if (args.size() == 3 && args[0] == "SET") {

            {
                std::lock_guard<std::mutex> lock(db_mutex);

                database[args[1]] = args[2];

                // Remove old expiry if key is overwritten
                expiry.erase(args[1]);
            }
            saveToAOF("SET " + args[1] + " " + args[2]);

            const char* response = "+OK\r\n";

            send(client_fd, response, strlen(response), 0);
        }
        // MSET Command
else if (args.size() >= 3 &&
         args[0] == "MSET" &&
         args.size() % 2 == 1) {

    {
        std::lock_guard<std::mutex> lock(db_mutex);

        for (int i = 1; i < args.size(); i += 2) {

            database[args[i]] = args[i + 1];

            // Remove old expiry if key is overwritten
            expiry.erase(args[i]);
        }
    }

    // Save all key-value pairs in AOF
    for (int i = 1; i < args.size(); i += 2) {

        saveToAOF(
            "SET " + args[i] + " " + args[i + 1]
        );
    }

    const char* response = "+OK\r\n";

    send(client_fd, response, strlen(response), 0);
}

        // GET Command
        else if (args.size() == 2 && args[0] == "GET") {

            std::string response;

            {
                std::lock_guard<std::mutex> lock(db_mutex);

                auto it = database.find(args[1]);

                if (it == database.end()) {

                    response = "$-1\r\n";
                }

                else {

                    auto exp = expiry.find(args[1]);

                    // Check whether key has expired
                    if (exp != expiry.end() &&
                        std::chrono::steady_clock::now() >= exp->second) {

                        database.erase(args[1]);
                        expiry.erase(args[1]);

                        response = "$-1\r\n";
                    }

                    else {

                        std::string value = it->second;

                        response = "$" +
                            std::to_string(value.length()) +
                            "\r\n" + value + "\r\n";
                    }
                }
            }

            send(
                client_fd,
                response.c_str(),
                response.length(),
                0
            );
        }
        // STRLEN Command
else if (args.size() == 2 && args[0] == "STRLEN") {

    int length = 0;

    {
        std::lock_guard<std::mutex> lock(db_mutex);

        auto it = database.find(args[1]);

        if (it != database.end()) {

            auto exp = expiry.find(args[1]);

            if (exp != expiry.end() &&
                std::chrono::steady_clock::now() >= exp->second) {

                database.erase(it);
                expiry.erase(args[1]);
            }
            else {
                length = it->second.length();
            }
        }
    }

    std::string response = ":" + std::to_string(length) + "\r\n";

    send(client_fd, response.c_str(), response.length(), 0);
}

// APPEND Command
else if (args.size() == 3 && args[0] == "APPEND") {

    int length = 0;

    {
        std::lock_guard<std::mutex> lock(db_mutex);

        auto it = database.find(args[1]);

        if (it == database.end()) {
            database[args[1]] = args[2];
        }
        else {
            it->second += args[2];
        }

        expiry.erase(args[1]);

        length = database[args[1]].length();

        saveToAOF("SET " + args[1] + " " +
                  database[args[1]]);
    }

    std::string response = ":" + std::to_string(length) + "\r\n";

    send(client_fd, response.c_str(), response.length(), 0);
}
// TYPE Command
else if (args.size() == 2 && args[0] == "TYPE") {

    std::string response;

    {
        std::lock_guard<std::mutex> lock(db_mutex);

        auto it = database.find(args[1]);

        if (it == database.end()) {
            response = "+none\r\n";
        }
        else {
            auto exp = expiry.find(args[1]);

            if (exp != expiry.end() &&
                std::chrono::steady_clock::now() >= exp->second) {

                database.erase(it);
                expiry.erase(args[1]);

                response = "+none\r\n";
            }
            else {
                response = "+string\r\n";
            }
        }
    }

    send(client_fd, response.c_str(), response.length(), 0);
}    
// FLUSHALL Command
else if (args.size() == 1 && args[0] == "FLUSHALL") {

    {
        std::lock_guard<std::mutex> lock(db_mutex);

        database.clear();
        expiry.clear();

        saveToAOF("FLUSHALL");
    }

    const char* response = "+OK\r\n";

    send(client_fd, response, strlen(response), 0);
} 
// DBSIZE Command
else if (args.size() == 1 && args[0] == "DBSIZE") {

    int count = 0;

    {
        std::lock_guard<std::mutex> lock(db_mutex);

        auto now = std::chrono::steady_clock::now();

        for (auto it = database.begin(); it != database.end();) {

            auto exp = expiry.find(it->first);

            if (exp != expiry.end() && now >= exp->second) {
                expiry.erase(exp);
                it = database.erase(it);
            }
            else {
                count++;
                ++it;
            }
        }
    }

    std::string response = ":" + std::to_string(count) + "\r\n";

    send(client_fd, response.c_str(), response.length(), 0);
}

// RENAME Command
else if (args.size() == 3 && args[0] == "RENAME") {

    std::string response;

    {
        std::lock_guard<std::mutex> lock(db_mutex);

        auto it = database.find(args[1]);

        if (it == database.end()) {
            response = "-ERR no such key\r\n";
        }
        else {
            auto exp = expiry.find(args[1]);

            if (exp != expiry.end() &&
                std::chrono::steady_clock::now() >= exp->second) {

                database.erase(it);
                expiry.erase(args[1]);

                response = "-ERR no such key\r\n";
            }
            else {
                std::string value = it->second;

                auto expiry_time = expiry.find(args[1]);
                bool has_expiry = expiry_time != expiry.end();

                std::chrono::steady_clock::time_point old_expiry;

                if (has_expiry) {
                    old_expiry = expiry_time->second;
                }

                database[args[2]] = value;
                database.erase(args[1]);

                expiry.erase(args[1]);

                if (has_expiry) {
                    expiry[args[2]] = old_expiry;
                }

                saveToAOF("SET " + args[2] + " " + value);
                saveToAOF("DEL " + args[1]);

                response = "+OK\r\n";
            }
        }
    }

    send(client_fd, response.c_str(), response.length(), 0);
}
// RENAMENX Command
else if (args.size() == 3 && args[0] == "RENAMENX") {

    int result = 0;

    {
        std::lock_guard<std::mutex> lock(db_mutex);

        auto old_it = database.find(args[1]);
        auto new_it = database.find(args[2]);

        auto now = std::chrono::steady_clock::now();

        if (old_it != database.end()) {

            auto old_exp = expiry.find(args[1]);

            if (old_exp != expiry.end() && now >= old_exp->second) {
                database.erase(old_it);
                expiry.erase(args[1]);
            }
            else {

                if (new_it != database.end()) {
                    auto new_exp = expiry.find(args[2]);

                    if (new_exp != expiry.end() && now >= new_exp->second) {
                        database.erase(new_it);
                        expiry.erase(args[2]);
                        new_it = database.end();
                    }
                }

                if (new_it == database.end() && args[1] != args[2]) {

                    std::string value = old_it->second;

                    bool has_expiry = old_exp != expiry.end();
                    std::chrono::steady_clock::time_point old_expiry;

                    if (has_expiry) {
                        old_expiry = old_exp->second;
                    }

                    database[args[2]] = value;
                    database.erase(args[1]);

                    expiry.erase(args[1]);

                    if (has_expiry) {
                        expiry[args[2]] = old_expiry;
                    }

                    saveToAOF("SET " + args[2] + " " + value);
                    saveToAOF("DEL " + args[1]);

                    result = 1;
                }
            }
        }
    }

    std::string response = ":" + std::to_string(result) + "\r\n";
    send(client_fd, response.c_str(), response.length(), 0);
}

// PERSIST Command
else if (args.size() == 2 && args[0] == "PERSIST") {

    int result = 0;

    {
        std::lock_guard<std::mutex> lock(db_mutex);

        auto it = database.find(args[1]);

        if (it != database.end()) {

            auto exp = expiry.find(args[1]);

            if (exp != expiry.end()) {

                if (std::chrono::steady_clock::now() >= exp->second) {
                    database.erase(it);
                    expiry.erase(exp);
                }
                else {
                    expiry.erase(exp);
                    saveToAOF("PERSIST " + args[1]);
                    result = 1;
                }
            }
        }
    }

    std::string response = ":" + std::to_string(result) + "\r\n";
    send(client_fd, response.c_str(), response.length(), 0);
}
// GETSET Command
else if (args.size() == 3 && args[0] == "GETSET") {

    std::string response;

    {
        std::lock_guard<std::mutex> lock(db_mutex);

        auto it = database.find(args[1]);

        if (it == database.end()) {
            response = "$-1\r\n";
        }
        else {
            auto exp = expiry.find(args[1]);

            if (exp != expiry.end() &&
                std::chrono::steady_clock::now() >= exp->second) {

                database.erase(it);
                expiry.erase(args[1]);

                response = "$-1\r\n";
            }
            else {
                std::string old_value = it->second;

                response = "$" + std::to_string(old_value.length())
                         + "\r\n" + old_value + "\r\n";
            }
        }

        database[args[1]] = args[2];
        expiry.erase(args[1]);

        saveToAOF("SET " + args[1] + " " + args[2]);
    }

    send(client_fd, response.c_str(), response.length(), 0);
}
// SETNX Command
else if (args.size() == 3 && args[0] == "SETNX") {

    int result = 0;

    {
        std::lock_guard<std::mutex> lock(db_mutex);

        auto it = database.find(args[1]);

        if (it != database.end()) {

            auto exp = expiry.find(args[1]);

            if (exp != expiry.end() &&
                std::chrono::steady_clock::now() >= exp->second) {

                database.erase(it);
                expiry.erase(args[1]);
                it = database.end();
            }
        }

        if (it == database.end()) {

            database[args[1]] = args[2];
            expiry.erase(args[1]);

            saveToAOF("SET " + args[1] + " " + args[2]);

            result = 1;
        }
    }

    std::string response = ":" + std::to_string(result) + "\r\n";

    send(client_fd, response.c_str(), response.length(), 0);
}
// GETDEL Command
else if (args.size() == 2 && args[0] == "GETDEL") {

    std::string response;

    {
        std::lock_guard<std::mutex> lock(db_mutex);

        auto it = database.find(args[1]);

        if (it == database.end()) {
            response = "$-1\r\n";
        }
        else {
            auto exp = expiry.find(args[1]);

            if (exp != expiry.end() &&
                std::chrono::steady_clock::now() >= exp->second) {

                database.erase(it);
                expiry.erase(args[1]);

                response = "$-1\r\n";
            }
            else {
                std::string old_value = it->second;

                response = "$" + std::to_string(old_value.length())
                         + "\r\n" + old_value + "\r\n";

                database.erase(it);
                expiry.erase(args[1]);

                saveToAOF("DEL " + args[1]);
            }
        }
    }

    send(client_fd, response.c_str(), response.length(), 0);
}

// MGET Command
else if (args.size() >= 2 && args[0] == "MGET") {

    std::string response = "*";
    response += std::to_string(args.size() - 1);
    response += "\r\n";

    {
        std::lock_guard<std::mutex> lock(db_mutex);

        for (int i = 1; i < args.size(); i++) {

            auto it = database.find(args[i]);

            if (it != database.end()) {

                auto exp = expiry.find(args[i]);

                if (exp != expiry.end() &&
                    std::chrono::steady_clock::now() >= exp->second) {

                    database.erase(it);
                    expiry.erase(args[i]);

                    response += "$-1\r\n";
                }
                else {
                    response += "$" + std::to_string(it->second.length())
                                + "\r\n" + it->second + "\r\n";
                }
            }
            else {
                response += "$-1\r\n";
            }
        }
    }

    send(client_fd, response.c_str(), response.length(), 0);
}
    
    // INCR Command
else if (args.size() == 2 && args[0] == "INCR") {

    std::string response;

    {
        std::lock_guard<std::mutex> lock(db_mutex);

        int value = 0;

        auto it = database.find(args[1]);

        if (it != database.end()) {

            try {
                value = std::stoi(it->second);
            }
            catch (...) {
                response = "-ERR value is not an integer\r\n";
            }

            if (response.empty()) {
                value++;
                database[args[1]] = std::to_string(value);
            }
        }
        else {
            value = 1;
            database[args[1]] = "1";
        }

        if (response.empty()) {
            expiry.erase(args[1]);

            saveToAOF("SET " + args[1] + " " +
                      database[args[1]]);

            response = ":" + std::to_string(value) + "\r\n";
        }
    }

    send(client_fd, response.c_str(), response.length(), 0);
}

    // DECR Command
else if (args.size() == 2 && args[0] == "DECR") {

    std::string response;

    {
        std::lock_guard<std::mutex> lock(db_mutex);

        int value = 0;

        auto it = database.find(args[1]);

        if (it != database.end()) {

            try {
                value = std::stoi(it->second);
            }
            catch (...) {
                response = "-ERR value is not an integer\r\n";
            }

            if (response.empty()) {
                value--;
                database[args[1]] = std::to_string(value);
            }
        }
        else {
            value = -1;
            database[args[1]] = "-1";
        }

        if (response.empty()) {
            expiry.erase(args[1]);

            saveToAOF("SET " + args[1] + " " +
                      database[args[1]]);

            response = ":" + std::to_string(value) + "\r\n";
        }
    }

    send(client_fd, response.c_str(), response.length(), 0);
}
   
// INCRBY Command
else if (args.size() == 3 && args[0] == "INCRBY") {

    std::string response;

    try {
        int increment = std::stoi(args[2]);

        {
            std::lock_guard<std::mutex> lock(db_mutex);

            int value = 0;

            auto it = database.find(args[1]);

            if (it != database.end()) {
                value = std::stoi(it->second);
            }

            value += increment;

            database[args[1]] = std::to_string(value);
            expiry.erase(args[1]);

            saveToAOF("SET " + args[1] + " " +
                      database[args[1]]);

            response = ":" + std::to_string(value) + "\r\n";
        }
    }
    catch (...) {
        response = "-ERR value is not an integer\r\n";
    }

    send(client_fd, response.c_str(), response.length(), 0);
}
// DECRBY Command
else if (args.size() == 3 && args[0] == "DECRBY") {

    std::string response;

    try {
        int decrement = std::stoi(args[2]);

        {
            std::lock_guard<std::mutex> lock(db_mutex);

            int value = 0;

            auto it = database.find(args[1]);

            if (it != database.end()) {
                value = std::stoi(it->second);
            }

            value -= decrement;

            database[args[1]] = std::to_string(value);
            expiry.erase(args[1]);

            saveToAOF("SET " + args[1] + " " +
                      database[args[1]]);

            response = ":" + std::to_string(value) + "\r\n";
        }
    }
    catch (...) {
        response = "-ERR value is not an integer\r\n";
    }

    send(client_fd, response.c_str(), response.length(), 0);
}


// EXISTS Command
    else if (args.size() == 2 && args[0] == "EXISTS") {

    int result = 0;

    {
        std::lock_guard<std::mutex> lock(db_mutex);

        auto it = database.find(args[1]);

        if (it != database.end()) {

            auto exp = expiry.find(args[1]);

            // Check whether key has expired
            if (exp != expiry.end() &&
                std::chrono::steady_clock::now() >= exp->second) {

                database.erase(args[1]);
                expiry.erase(args[1]);
            }

            else {
                result = 1;
            }
        }
    }

    std::string response =
        ":" + std::to_string(result) + "\r\n";

    send(
        client_fd,
        response.c_str(),
        response.length(),
        0
    );
}

// TTL Command
else if (args.size() == 2 && args[0] == "TTL") {

    int result = -2;

    {
        std::lock_guard<std::mutex> lock(db_mutex);

        auto it = database.find(args[1]);

        if (it != database.end()) {

            auto exp = expiry.find(args[1]);

            // Key exists but has no expiry
            if (exp == expiry.end()) {

                result = -1;
            }

            else {

                auto now = std::chrono::steady_clock::now();

                // Check if key has expired
                if (now >= exp->second) {

                    database.erase(args[1]);
                    expiry.erase(args[1]);

                    result = -2;
                }

                else {

                    result = static_cast<int>(
                        std::chrono::duration_cast<std::chrono::seconds>(
                            exp->second - now
                        ).count()
                    );
                }
            }
        }
    }

    std::string response =
        ":" + std::to_string(result) + "\r\n";

    send(
        client_fd,
        response.c_str(),
        response.length(),
        0
    );
}
        // DEL Command
        else if (args.size() == 2 && args[0] == "DEL") {

            size_t deleted;

            {
                std::lock_guard<std::mutex> lock(db_mutex);

                deleted = database.erase(args[1]);

                // Remove expiry information too
                expiry.erase(args[1]);
            }
            if (deleted > 0) {
    saveToAOF("DEL " + args[1]);
}

            std::string response =
                ":" + std::to_string(deleted) + "\r\n";

            send(
                client_fd,
                response.c_str(),
                response.length(),
                0
            );
        }


        // EXPIRE Command
        // EXPIRE Command
else if (args.size() == 3 && args[0] == "EXPIRE") {

    int seconds;

    try {
        seconds = std::stoi(args[2]);
    }
    catch (...) {

        const char* response = "-ERR invalid integer\r\n";

        send(client_fd, response, strlen(response), 0);

        close(client_fd);
        return;
    }

    int result = 0;

    {
        std::lock_guard<std::mutex> lock(db_mutex);

        if (database.find(args[1]) != database.end()) {

            expiry[args[1]] =
                std::chrono::steady_clock::now()
                + std::chrono::seconds(seconds);

            long long expiry_timestamp =
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count() + seconds;

            // Print timestamp for debugging
            std::cout << "Expiry timestamp: "
                      << expiry_timestamp << std::endl;

            // Save expiry information in AOF
            saveToAOF(
                "EXPIREAT " + args[1] + " " +
                std::to_string(expiry_timestamp)
            );

            result = 1;
        }
    }

    std::string response =
        ":" + std::to_string(result) + "\r\n";

    send(
        client_fd,
        response.c_str(),
        response.length(),
        0
    );
}
        // Invalid Command
    else {

    const char* response = "-ERR unknown command or invalid arguments\r\n";

    send(client_fd, response, strlen(response), 0);
}   
    }

    close(client_fd);
}


// Main Function
int main() {
    loadFromAOF();

    // Step 1: Create Socket
    int server_fd = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (server_fd == -1) {

        std::cerr << "Socket creation failed\n";

        return 1;
    }


    // Step 2: Configure Server Address
    sockaddr_in address{};

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(6379);


    // Step 3: Bind Socket
    int result = bind(
        server_fd,
        (struct sockaddr*)&address,
        sizeof(address)
    );

    if (result == -1) {

        std::cerr << "Binding failed\n";

        close(server_fd);

        return 1;
    }

    std::cout << "Socket bind successful!\n";


    // Step 4: Listen for Connections
    result = listen(server_fd, 5);

    if (result == -1) {

        std::cerr << "Listen failed\n";

        close(server_fd);

        return 1;
    }

    std::cout << "Server is listening on port 6379...\n";


    // Step 5: Accept Multiple Clients
    while (true) {

        sockaddr_in client_address{};

        socklen_t client_len = sizeof(client_address);

        int client_fd = accept(
            server_fd,
            (struct sockaddr*)&client_address,
            &client_len
        );

        if (client_fd == -1) {

            std::cerr << "Client connection failed\n";

            continue;
        }

        std::cout << "Client connected successfully!\n";


        // Handle each client in a separate thread
        std::thread client_thread(
            handleClient,
            client_fd
        );

        client_thread.detach();
    }

    close(server_fd);

    return 0;
}
