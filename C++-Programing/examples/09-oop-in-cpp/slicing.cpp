#include <iostream>
#include <string_view>

class Message {
public:
    virtual ~Message() = default;

    virtual std::string_view name() const
    {
        return "Message";
    }
};

class AlarmMessage final : public Message {
public:
    std::string_view name() const override
    {
        return "AlarmMessage";
    }
};

std::string_view name_by_value(Message message)
{
    return message.name();
}

std::string_view name_by_reference(const Message& message)
{
    return message.name();
}

int main()
{
    const AlarmMessage message;

    const std::string_view sliced = name_by_value(message);
    const std::string_view polymorphic = name_by_reference(message);
    const bool passed =
        sliced == "Message" && polymorphic == "AlarmMessage";

    std::cout
        << "by-value=" << sliced
        << " by-reference=" << polymorphic
        << " result=" << (passed ? "passed" : "failed")
        << '\n';

    return passed ? 0 : 1;
}
