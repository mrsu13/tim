#pragma once

#include <cstddef>
#include <string>
#include <string_view>


namespace tim
{

/**
 * Значение-имя или фильтр MQTT-топика, разделяемое символом '/'.
 *
 * На любой платформе использует '/' в качестве разделителя уровней —
 * в отличие от std::filesystem::path, который применяет разделитель,
 * предпочитаемый платформой, и преобразует строку через активную
 * кодовую страницу в Windows.
 */
class mqtt_topic
{

public:

    /** Конструирует пустой топик. */
    mqtt_topic() = default;

    mqtt_topic(const char *topic);

    mqtt_topic(std::string_view topic);

    mqtt_topic(std::string topic);

    bool empty() const noexcept;

    std::size_t size() const noexcept;

    const char *data() const noexcept;

    const char *c_str() const noexcept;

    const std::string &str() const noexcept;

    std::string_view view() const noexcept;

    operator std::string_view() const noexcept;

    std::string_view last_level() const noexcept;

    tim::mqtt_topic parent() const;

    tim::mqtt_topic &operator/=(std::string_view level);

    bool operator==(const tim::mqtt_topic &other) const noexcept;

    bool operator!=(const tim::mqtt_topic &other) const noexcept;

private:

    /** Хранилище имени топика. */
    std::string _value;
};

tim::mqtt_topic operator/(tim::mqtt_topic lhs, std::string_view level);

bool topic_matches(std::string_view topic, std::string_view filter);

}
