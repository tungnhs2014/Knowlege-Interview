#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class TraceResource {
public:
    explicit TraceResource(std::string name) : name_(std::move(name))
    {
        ++alive_count_;
        std::cout << "acquire " << name_ << '\n';
    }

    TraceResource(const TraceResource&) = delete;
    TraceResource& operator=(const TraceResource&) = delete;

    ~TraceResource() noexcept
    {
        --alive_count_;
        std::cout << "release " << name_ << '\n';
    }

    static int alive_count() noexcept
    {
        return alive_count_;
    }

private:
    std::string name_;
    static int alive_count_;
};

int TraceResource::alive_count_ = 0;

class ConfigLines {
public:
    explicit ConfigLines(std::vector<std::string> lines)
        : lines_(std::move(lines))
    {
    }

    void replace_all_strong(std::vector<std::string> next)
    {
        validate(next);    // May throw before touching lines_.
        lines_.swap(next); // Commit after throwing work is done.
    }

    std::size_t size() const noexcept
    {
        return lines_.size();
    }

private:
    static void validate(const std::vector<std::string>& lines)
    {
        for (const auto& line : lines) {
            if (line.empty()) {
                throw std::runtime_error("empty config line");
            }
        }
    }

    std::vector<std::string> lines_;
};

static void throwing_work()
{
    TraceResource file{"file"};
    TraceResource buffer{"buffer"};
    throw std::runtime_error("simulated parse failure");
}

int main()
{
    bool caught = false;

    try {
        throwing_work();
    } catch (const std::exception& e) {
        caught = true;
        std::cout << "caught=" << e.what() << '\n';
    }

    ConfigLines config{{"port=1234", "mode=test"}};
    try {
        config.replace_all_strong({"valid", ""});
    } catch (const std::exception& e) {
        std::cout << "replace-failed=" << e.what() << '\n';
    }

    const bool passed = caught && TraceResource::alive_count() == 0 &&
                        config.size() == 2u;

    std::cout << "alive=" << TraceResource::alive_count()
              << " size-after-failure=" << config.size()
              << " result=" << (passed ? "passed" : "failed") << '\n';

    return passed ? 0 : 1;
}
