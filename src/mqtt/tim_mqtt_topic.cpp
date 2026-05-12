#include "tim_mqtt_topic.h"

#include <cassert>
#include <utility>


// Открытые

tim::mqtt_topic::mqtt_topic(const char *topic)
    : _value(topic ? topic : "")
{
}

tim::mqtt_topic::mqtt_topic(std::string_view topic)
    : _value(topic)
{
}

tim::mqtt_topic::mqtt_topic(std::string topic)
    : _value(std::move(topic))
{
}

bool tim::mqtt_topic::empty() const noexcept
{
    return _value.empty();
}

std::size_t tim::mqtt_topic::size() const noexcept
{
    return _value.size();
}

const char *tim::mqtt_topic::data() const noexcept
{
    return _value.data();
}

const char *tim::mqtt_topic::c_str() const noexcept
{
    return _value.c_str();
}

const std::string &tim::mqtt_topic::str() const noexcept
{
    return _value;
}

std::string_view tim::mqtt_topic::view() const noexcept
{
    return _value;
}

tim::mqtt_topic::operator std::string_view() const noexcept
{
    return _value;
}

std::string_view tim::mqtt_topic::last_level() const noexcept
{
    const std::size_t slash = _value.rfind('/');
    if (slash == std::string::npos)
        return _value;
    return std::string_view(_value).substr(slash + 1);
}

tim::mqtt_topic tim::mqtt_topic::parent() const
{
    const std::size_t slash = _value.rfind('/');
    if (slash == std::string::npos)
        return tim::mqtt_topic();
    return tim::mqtt_topic(_value.substr(0, slash));
}

tim::mqtt_topic &tim::mqtt_topic::operator/=(std::string_view level)
{
    assert(level.find('/') == std::string_view::npos
           && "An mqtt_topic level must not contain '/'.");

    if (!_value.empty())
        _value.push_back('/');
    _value.append(level);
    return *this;
}

bool tim::mqtt_topic::operator==(const tim::mqtt_topic &other) const noexcept
{
    return _value == other._value;
}

bool tim::mqtt_topic::operator!=(const tim::mqtt_topic &other) const noexcept
{
    return _value != other._value;
}


// Свободные функции

tim::mqtt_topic tim::operator/(tim::mqtt_topic lhs, std::string_view level)
{
    lhs /= level;
    return lhs;
}

bool tim::topic_matches(std::string_view topic, std::string_view filter)
{
    // Топик делится на уровни по '/'. N слэшей дают N+1 уровней
    // (включая пустые). Поэтому "post/" — это два уровня: "post" и ""
    // и должен матчиться "post/+" по спецификации MQTT 3.1.1.
    auto take_level = [](std::string_view &s, bool &has_more)
    {
        const std::size_t slash = s.find('/');
        std::string_view level;
        if (slash == std::string_view::npos)
        {
            level = s;
            s = std::string_view();
            has_more = false;
        }
        else
        {
            level = s.substr(0, slash);
            s.remove_prefix(slash + 1);
            // has_more остаётся true — после слэша всегда есть как минимум
            // ещё один уровень, пусть и пустой.
        }
        return level;
    };

    bool topic_has_more = true;
    bool filter_has_more = true;

    while (filter_has_more)
    {
        const std::string_view f = take_level(filter, filter_has_more);

        if (f == "#")
            return !filter_has_more; // '#' должен быть последним токеном.

        if (!topic_has_more)
            return false;

        const std::string_view t = take_level(topic, topic_has_more);
        if (f == "+")
            continue;
        if (f != t)
            return false;
    }
    return !topic_has_more;
}
