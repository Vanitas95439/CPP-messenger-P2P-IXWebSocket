#include <iostream>
#include <string>
#include <thread>
#include <sstream>
#include <atomic>
#include <ixwebsocket/IXWebSocketServer.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXHttpClient.h> 
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>

 std::string getPublicIP()
{
    ix::HttpClient httpClient;
    ix::HttpRequestArgsPtr args = std::make_shared<ix::HttpRequestArgs>();

    auto response = httpClient.get("http://api.ipify.org", args);

    if (response && response->statusCode == 200) {
        return response->body;
    }
    else {
        std::cerr << "[error] Can't get public IP. Status: "
                  << (response ? std::to_string(response->statusCode) : "null");
        return " ";
    }
} 

bool openPort(const int& port, std::string& external_ip, UPNPUrls& urls, IGDdatas& data)
{
    int error;
    UPNPDev* devlist = upnpDiscover(2000, nullptr, nullptr, 0, 0, 2, &error);
    if (!devlist) {
        std::cerr << "[UPnP] Router not found. Error code:" << error << '\n';
        return false;
    }

    char lanaddr[64] = {};

    int r = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr), nullptr, 0);
    freeUPNPDevlist(devlist);

    if (r == 0) {
        std::cerr << "[UPnP] No valid IGD (router) found.\n";
        return false;
    }

    char ext_ip[40];
    if (UPNP_GetExternalIPAddress(urls.controlURL, data.first.servicetype, ext_ip) != UPNPCOMMAND_SUCCESS) {
        std::cerr << "[UPnP] Cannot get external IP.\n";
        return false;
    }

    external_ip = ext_ip;
    std::cout << "[UPnP] External IP: " << external_ip << '\n';
    std::cout << "[UPnP] Local IP: " << lanaddr << '\n';

    std::string port_str = std::to_string(port);
    int res = UPNP_AddPortMapping(urls.controlURL,
                                  data.first.servicetype,
                                  port_str.c_str(),   // external port
                                  port_str.c_str(),   // internal port
                                  lanaddr,
                                  "IXWebSocket Chat Server",
                                  "TCP", nullptr, nullptr);

    if (res != UPNPCOMMAND_SUCCESS) {
        std::cerr << "[UPnP] Failed to open port: " << port << '\n';
        FreeUPNPUrls(&urls);
        return false;
    }

    std::cout << "[UPnP] Port " << port << " successfully opened!" << '\n';

    FreeUPNPUrls(&urls);
    return true;
}

void run_server(int port)
{
    std::string host = "0.0.0.0";
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
                    std::cout << "[state] Client connected: " << connectionState->getRemoteIp() << '\n'
                    << "Enter the messages (use exit to close connection)\n";;
                    client_socket = &webSocket;
                    break;

                case ix::WebSocketMessageType::Message:
                    std::cout << "[peer] " << msg->str << '\n';
                    break;

                case ix::WebSocketMessageType::Close:
                    std::cout << "[state] Client disconnected\n";
                    client_socket = nullptr;
                    break;

                default:
                    break;
            }
        });

    UPNPUrls urls;
    IGDdatas data;
    std::string external_ip;
    if (!openPort(port, external_ip, urls, data)) {
        std::cerr << "[Error] Failed to open port using UPnP.\n";
    }

    if (external_ip.empty()) {
        external_ip = getPublicIP();
    }

    server.listenAndStart();
    std::cout << "[server running] Public IP: " << external_ip << "\n"
              << "Listening on " << host << ":" << port << "\n";

    std::thread inputThread([&]()
    {
        std::string line;
        while (running) {
            std::getline(std::cin, line);
            if (line == "exit") {
                running = false;
                break;
            }
            if (client_socket)
                client_socket->send(line);
        }
    });

    while (running)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, std::to_string(port).c_str(), "TCP", nullptr);
    server.stop();
    inputThread.join();
    std::cout << "[server stopped]\n";
}

void run_client(const std::string& ip, int port)
{
    ix::WebSocket ws;
    std::string url = "ws://" + ip + ":" + std::to_string(port) + "/";
    ws.setUrl(url);

    ws.setOnMessageCallback(
        [&](const ix::WebSocketMessagePtr& msg)
        {
            if (msg->type == ix::WebSocketMessageType::Open) {
                std::cout << "[state] Connected to the server " << ip << ":" << port << '\n'
                << "Enter the messages (use exit to close connection)\n";
            }
            else if (msg->type == ix::WebSocketMessageType::Message) {
                std::cout << "[peer] " << msg->str << '\n';
            }
            else if (msg->type == ix::WebSocketMessageType::Close) {
                std::cout << "[state] Connection is closed.\n";
            }
        });

    ws.start();
    ws.enableAutomaticReconnection();

    std::cout << "[client] Connecting to " << url << "\n";

    std::string line;
    while (true) {
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
    std::cout << "=== P2P WebSocket Chat ===\n"
    << "Enter mode and parameters in one line:\n"
    << "Server: 1 <port>\n" 
    << "Client: 2 <ip> <port>\n"
    << "> ";

    std::string line;
    std::getline(std::cin, line);
    std::istringstream iss(line);

    std::string mode;
    iss >> mode;
    for (auto& x : mode) x = std::tolower(x);

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
