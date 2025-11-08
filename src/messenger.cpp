#include <iostream>
#include <string>
#include <thread>
#include <sstream>
#include <atomic>
#include <ixwebsocket/IXWebSocketServer.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXNetSystem.h>

void run_server(int port)
{
    std::string host = "0.0.0.0";
    if (port == 0) {
        port = 8080; 
        host = "127.0.0.1";
    }
    ix::WebSocketServer server(port, host);
    ix::WebSocket* client_socket = nullptr; 
    std::atomic<bool> running(true);

    server.setOnClientMessageCallback(
        [&](std::shared_ptr<ix::ConnectionState> connectionState,
            ix::WebSocket& webSocket,
            const ix::WebSocketMessagePtr& msg)
        {
            switch (msg->type)
            {
                case ix::WebSocketMessageType::Open:
                    std::cout << "[state] Client connected: " << connectionState->getRemoteIp() << std::endl;
                    client_socket = &webSocket;
                    break;

                case ix::WebSocketMessageType::Message:
                    std::cout << "[peer] " << msg->str << std::endl;
                    break;

                case ix::WebSocketMessageType::Close:
                    std::cout << "[state] Client disconnected\n";
                    client_socket = nullptr;
                    break;

                default:
                    break;
            }
        });

    server.listenAndStart();
    std::cout << "[Server running] Type 'exit' to stop\n";

    std::thread inputThread([&]()
    {
        std::string line;
        while (running)
        {
            std::getline(std::cin, line);
            if (line == "exit")
            {
                running = false;
                break;
            }
            if (client_socket)
                client_socket->send(line);
        }
    });

    while (running)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    server.stop();
    inputThread.join();
    std::cout << "[Server stopped]\n";
}

void run_client(const std::string& ip, int port)
{
    ix::WebSocket ws;
    std::string url = "ws://" + ip + ":" + std::to_string(port) + "/";
    ws.setUrl(url);

    ws.setOnMessageCallback(
        [&](const ix::WebSocketMessagePtr& msg)
        {
            if (msg->type == ix::WebSocketMessageType::Open)
            {
                std::cout << "[state] Connected to the server " << ip << ":" << port << std::endl;
            }
            else if (msg->type == ix::WebSocketMessageType::Message)
            {
                std::cout << "[peer] " << msg->str << std::endl;
            }
            else if (msg->type == ix::WebSocketMessageType::Close)
            {
                std::cout << "[state] Connection is closed.\n";
            }
        });

    ws.start();
    ws.enableAutomaticReconnection();

    std::cout << "[client] Connecting to " << url << "\n";
    std::cout << "Enter the messages (use exit to close connection)\n";

    std::string line;
    while (true)
    {
        std::getline(std::cin, line);
        if (line == "exit") break;
        ws.send(line);
    }

    ws.stop();
}

int main()
{
    #if defined(_WIN32) || defined(WIN32)
        ix::initNetSystem();
    #endif
    std::cout << "=== P2P WebSocket Chat ===\n";
    std::cout << "Enter mode and parameters in one line:\n";
    std::cout << "Server: 1 <port>\nClient: 2 <ip> <port>\n> ";

    std::string line;
    std::getline(std::cin, line);
    std::istringstream iss(line);

    std::string mode;
    iss >> mode;

    if (mode == "server" || mode == "1") {
        int port;
        if (iss >> port) {
            run_server(port); 
        }
        else {
            std::cout << "[Error] Invalid port\n";
        }
    }
    else if (mode == "client" || mode == "2") {
        std::string ip;
        int port;
        if (iss >> ip >> port) {
            run_client(ip, port);  
        }
        else {
            std::cout << "[Error] Invalid IP or port\n";
        }
    }
    else {
        std::cout << "[Error] Unknown mode\n";
    }

    #if defined(_WIN32) || defined(WIN32)
        ix::uninitNetSystem();
    #endif

    return 0;
}
