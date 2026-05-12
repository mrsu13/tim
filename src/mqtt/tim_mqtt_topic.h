#pragma once

#include <cstddef>
#include <string>
#include <string_view>


namespace tim
{

// Имя или фильтр MQTT-топика, разделяемое символом '/'.
// На любой платформе использует '/' в качестве разделителя уровней —
// в отличие от std::filesystem::path, который применяет разделитель,
// предпочитаемый платформой, и преобразует строку через активную
// кодовую страницу в Windows.
class mqtt_topic
{

public:

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

    // Последний уровень, отделённый '/'. Весь топик, если '/' отсутствует.
    // Пустая строка, если сам топик пуст.
    std::string_view last_level() const noexcept;

    // Топик с удалённым последним уровнем.
    // Пустой, если в топике нет '/'.
    tim::mqtt_topic parent() const;

    // Добавляет уровень. Уровень не должен содержать '/'; проверяется ассертом.
    tim::mqtt_topic &operator/=(std::string_view level);

    bool operator==(const tim::mqtt_topic &other) const noexcept;
    bool operator!=(const tim::mqtt_topic &other) const noexcept;

private:

    std::string _value;
};

tim::mqtt_topic operator/(tim::mqtt_topic lhs, std::string_view level);

// Возвращает true тогда и только тогда, когда `topic` соответствует
// MQTT-фильтру `filter`. Семантика MQTT 3.1.1: '+' — ровно один уровень,
// '#' — остаток (включая ноль уровней) и должен быть последним токеном.
bool topic_matches(std::string_view topic, std::string_view filter);

}
