#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <mutex>
#include <fstream>
#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

using namespace std;

struct Metrics {
    atomic<int> success_count{0};
    atomic<int> fail_count{0};
    vector<long long> latencies;
    mutex lat_mtx;
};

int connectServer(const string &ip, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return -1;

    sockaddr_in servaddr{};
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &servaddr.sin_addr);

    if (connect(sockfd, (sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        close(sockfd);
        return -1;
    }
    return sockfd;
}

void clientWorker(int id, const string &ip, int port, int duration_sec, Metrics &metrics) {
    int sockfd = connectServer(ip, port);
    struct timeval send_timeout{1, 0};
    struct timeval recv_timeout{2, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));
    if (sockfd < 0) {
        cerr << "Client " << id << " failed to connect\n";
        return;
    }

    auto end_time = chrono::steady_clock::now() + chrono::seconds(duration_sec);

    while (chrono::steady_clock::now() < end_time) {
        string msg = "{\"msgid\":6,\"id\":1,\"to\":2,\"msg\":\"hello\"}";
        auto start = chrono::steady_clock::now();

        if (send(sockfd, msg.c_str(), msg.size(), 0) < 0) {
            metrics.fail_count++;
            continue;
        }

        char buffer[1024];
        int n = recv(sockfd, buffer, sizeof(buffer), 0);
        if (n > 0) {
            auto end = chrono::steady_clock::now();
            long long latency = chrono::duration_cast<chrono::microseconds>(end - start).count();

            {
                lock_guard<mutex> lock(metrics.lat_mtx);
                metrics.latencies.push_back(latency);
            }
            metrics.success_count++;
        } else {
            metrics.fail_count++;
        }
    }

    close(sockfd);
}

void saveResults(const Metrics &metrics, int clients, int duration_sec, const string &filename) {
    bool exists_file = ifstream(filename).good();
    ofstream ofs(filename, ios::app);
    if(!exists_file)
        ofs << "clients,duration,success,fail,avg_latency_us,p95_latency_us,p99_latency_us\n";

    vector<long long> lat = metrics.latencies;
    if (lat.empty()) {
        ofs << clients << "," << duration_sec << "," << metrics.success_count << "," << metrics.fail_count
            << ",0,0,0\n";
        return;
    }

    sort(lat.begin(), lat.end());
    long long sum = 0;
    for (auto v : lat) sum += v;

    long long avg = sum / lat.size();
    long long p95 = lat[lat.size() * 95 / 100];
    long long p99 = lat[lat.size() * 99 / 100];

    ofs << clients << "," << duration_sec << "," << metrics.success_count << "," << metrics.fail_count
        << "," << avg << "," << p95 << "," << p99 << "\n";

    cout << "Test finished: success=" << metrics.success_count
         << ", fail=" << metrics.fail_count
         << ", avg=" << avg << "us, p95=" << p95 << "us, p99=" << p99 << "us\n";
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        cerr << "Usage: " << argv[0] << " <server_ip> <port> <clients> <duration_sec>\n";
        return 1;
    }

    string ip = argv[1];
    int port = stoi(argv[2]);
    int clients = stoi(argv[3]);
    int duration = stoi(argv[4]);

    Metrics metrics;
    vector<thread> threads;

    for (int i = 0; i < clients; i++) {
        threads.emplace_back(clientWorker, i, ip, port, duration, ref(metrics));
    }

    for (auto &t : threads) t.join();

    saveResults(metrics, clients, duration, "result.csv");
    return 0;
}
