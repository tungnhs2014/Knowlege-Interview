#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

class Parser {
public:
    virtual std::string parse(const std::string& text) const = 0;
    virtual ~Parser() = default;
};

class JsonParser final : public Parser {
public:
    std::string parse(const std::string& text) const override {
        if (text.empty() || text.front() != '{') {
            throw std::runtime_error("expected json object");
        }
        return "json parsed";
    }
};

class ParserFactory {
public:
    virtual std::unique_ptr<Parser> create() const = 0;
    virtual ~ParserFactory() = default;
};

class JsonParserFactory final : public ParserFactory {
public:
    std::unique_ptr<Parser> create() const override {
        return std::make_unique<JsonParser>();
    }
};

int legacy_log_write(const char* text) {
    if (text == nullptr) {
        return -1;
    }
    std::cout << "legacy log: " << text << '\n';
    return 0;
}

class LogSink {
public:
    virtual void write(const std::string& text) = 0;
    virtual ~LogSink() = default;
};

class LegacyLogAdapter final : public LogSink {
public:
    void write(const std::string& text) override {
        if (legacy_log_write(text.c_str()) != 0) {
            throw std::runtime_error("legacy logger failed");
        }
    }
};

class ParserFacade {
public:
    ParserFacade(std::unique_ptr<Parser> parser, LogSink& log)
        : parser_(std::move(parser)), log_(log) {}

    bool parse_and_log(const std::string& text) {
        try {
            log_.write(parser_->parse(text));
            return true;
        } catch (const std::exception& ex) {
            log_.write(std::string("parse failed: ") + ex.what());
            return false;
        }
    }

private:
    std::unique_ptr<Parser> parser_;
    LogSink& log_;
};

int main() {
    LegacyLogAdapter log;
    JsonParserFactory factory;
    ParserFacade facade(factory.create(), log);

    const bool ok = facade.parse_and_log("{}");
    const bool bad = facade.parse_and_log("not json");

    std::cout << "ok=" << ok << " bad=" << bad << '\n';

    // Factory owns creation, Adapter translates an old API, Facade coordinates
    // parser + logging. Keep each layer thin; do not hide important errors.
}
