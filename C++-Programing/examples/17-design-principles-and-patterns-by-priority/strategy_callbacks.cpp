#include <iostream>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

template <typename Checksum>
int verify_with_template(const std::vector<int>& packet, Checksum checksum) {
    return checksum(packet);
}

class ChecksumStrategy {
public:
    virtual int compute(const std::vector<int>& packet) const = 0;
    virtual ~ChecksumStrategy() = default;
};

class SumChecksum final : public ChecksumStrategy {
public:
    int compute(const std::vector<int>& packet) const override {
        return std::accumulate(packet.begin(), packet.end(), 0);
    }
};

class XorChecksum final : public ChecksumStrategy {
public:
    int compute(const std::vector<int>& packet) const override {
        int result = 0;
        for (int byte : packet) {
            result ^= byte;
        }
        return result;
    }
};

class PacketVerifier {
public:
    explicit PacketVerifier(std::unique_ptr<ChecksumStrategy> strategy)
        : strategy_(std::move(strategy)) {}

    int verify(const std::vector<int>& packet) const {
        return strategy_->compute(packet);
    }

private:
    std::unique_ptr<ChecksumStrategy> strategy_;
};

int main() {
    const std::vector<int> packet{1, 2, 3, 4};

    const int lambda_result = verify_with_template(packet, [](const auto& data) {
        return std::accumulate(data.begin(), data.end(), 0);
    });

    PacketVerifier sum_verifier(std::make_unique<SumChecksum>());
    PacketVerifier xor_verifier(std::make_unique<XorChecksum>());

    std::cout << "template/lambda checksum: " << lambda_result << '\n';
    std::cout << "runtime sum checksum: " << sum_verifier.verify(packet) << '\n';
    std::cout << "runtime xor checksum: " << xor_verifier.verify(packet) << '\n';

    // Production-style shape, simplified:
    // - template/lambda is simple and inline-friendly;
    // - runtime Strategy is useful when selected from configuration;
    // - unique_ptr makes ownership explicit.
}
