#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

class Engine {
public:
    int run(const char* input) {
        if (input == nullptr) {
            throw std::invalid_argument("input must not be null");
        }

        last_input_ = input;
        return static_cast<int>(last_input_.size());
    }

private:
    std::string last_input_;
};

extern "C" {

struct EngineHandle;

int engine_create(EngineHandle** out);
void engine_destroy(EngineHandle* handle);
int engine_run(EngineHandle* handle, const char* input, int* out_length);

}

struct EngineHandle {
    Engine engine;
};

int engine_create(EngineHandle** out) {
    if (out == nullptr) {
        return -1;
    }

    try {
        *out = new EngineHandle{};
        return 0;
    } catch (...) {
        *out = nullptr;
        return -2;
    }
}

void engine_destroy(EngineHandle* handle) {
    delete handle;
}

int engine_run(EngineHandle* handle, const char* input, int* out_length) {
    if (handle == nullptr || out_length == nullptr) {
        return -1;
    }

    try {
        *out_length = handle->engine.run(input);
        return 0;
    } catch (const std::invalid_argument&) {
        return -3;
    } catch (const std::exception&) {
        return -4;
    } catch (...) {
        return -5;
    }
}

int main() {
    EngineHandle* handle = nullptr;
    if (engine_create(&handle) != 0) {
        std::cerr << "engine_create failed\n";
        return 1;
    }

    int length = 0;
    int status = engine_run(handle, "abc", &length);
    std::cout << "engine_run status: " << status
              << ", length: " << length << '\n';

    status = engine_run(handle, nullptr, &length);
    std::cout << "engine_run null-input status: " << status << '\n';

    engine_destroy(handle);
    return 0;
}

