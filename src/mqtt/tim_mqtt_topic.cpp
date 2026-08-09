#include "tim_mqtt_topic.h"

#include <cassert>
#include <utility>


// Открытые

/**
 * Конструктор из C-строки.
 *
 * \param topic Указатель на нуль-терминированную строку.
 */
tim::mqtt_topic::mqtt_topic(const char *topic)
    : _value(topic ? topic : "")
{
}

/**
 * Конструктор из string_view.
 *
 * \param topic Срез строки с именем топика.
 */
tim::mqtt_topic::mqtt_topic(std::string_view topic)
    : _value(topic)
{
}

/**
 * Конструктор-перемещение из std::string.
 *
 * \param topic Готовая строка-имя топика.
 */
tim::mqtt_topic::mqtt_topic(std::string topic)
    : _value(std::move(topic))
{
}

/** \return true, если топик пуст. */
bool tim::mqtt_topic::empty() const noexcept
{
    return _value.empty();
}

/** \return Длина имени топика в байтах. */
std::size_t tim::mqtt_topic::size() const noexcept
{
    return _value.size();
}

/** \return Указатель на байты имени (не обязательно нуль-терминирован). */
const char *tim::mqtt_topic::data() const noexcept
{
    return _value.data();
}

/** \return Указатель на нуль-терминированную C-строку имени топика. */
const char *tim::mqtt_topic::c_str() const noexcept
{
    return _value.c_str();
}

/** \return Ссылка на хранящуюся std::string. */
const std::string &tim::mqtt_topic::str() const noexcept
{
    return _value;
}

/** \return string_view имени топика. */
std::string_view tim::mqtt_topic::view() const noexcept
{
    return _value;
}

/** Неявное приведение к string_view для API, ожидающих view. */
tim::mqtt_topic::operator std::string_view() const noexcept
{
    return _value;
}

/**
 * Возвращает последний уровень топика (всё после последнего '/').
 *
 * \return Срез последнего уровня. Если '/' нет — весь топик.
 *         Пустая строка — если сам топик пуст.
 */
std::string_view tim::mqtt_topic::last_level() const noexcept
{
    const std::size_t slash = _value.rfind('/');
    if (slash == std::string::npos)
        return _value;
    return std::string_view(_value).substr(slash + 1);
}

/**
 * Возвращает родительский топик (без последнего уровня).
 *
 * \return Топик без последнего уровня; пустой, если в исходном не было '/'.
 */
tim::mqtt_topic tim::mqtt_topic::parent() const
{
    const std::size_t slash = _value.rfind('/');
    if (slash == std::string::npos)
        return tim::mqtt_topic();
    return tim::mqtt_topic(_value.substr(0, slash));
}

/**
 * Добавляет уровень к топику (mutating).
 *
 * \param level Имя нового уровня; не должно содержать '/' (assert).
 * \return *this для chaining.
 */
tim::mqtt_topic &tim::mqtt_topic::operator/=(std::string_view level)
{
    assert(level.find('/') == std::string_view::npos
           && "An mqtt_topic level must not contain '/'.");

    if (!_value.empty())
        _value.push_back('/');
    _value.append(level);
    return *this;
}

/** Сравнение по содержимому строки. */
bool tim::mqtt_topic::operator==(const tim::mqtt_topic &other) const noexcept
{
    return _value == other._value;
}

/** Сравнение по содержимому строки. */
bool tim::mqtt_topic::operator!=(const tim::mqtt_topic &other) const noexcept
{
    return _value != other._value;
}


// Свободные функции

/**
 * Свободная функция-перегруз / для построения новых топиков.
 *
 * \param lhs Базовый топик (передаётся по значению — будет модифицирован).
 * \param level Новый уровень.
 * \return Новый топик lhs + '/' + level.
 */
tim::mqtt_topic tim::operator/(tim::mqtt_topic lhs, std::string_view level)
{
    lhs /= level;
    return lhs;
}

/**
 * Проверка соответствия MQTT-фильтру согласно семантике MQTT 3.1.1:
 * '+' — ровно один уровень, '#' — остаток (включая ноль уровней)
 * и должен быть последним элементом фильтра. Топик делится на уровни
 * по '/' — N слэшей дают N+1 уровней (включая пустые); поэтому
 * "post/" — это два уровня: "post" и "", и он должен соответствовать "post/+".
 *
 * \param topic Конкретный топик публикации.
 * \param filter Фильтр подписки (могут быть метасимволы '+' и '#').
 * \return true, если \a topic соответствует \a filter.
 */
bool tim::topic_matches(std::string_view topic, std::string_view filter)
{
    // Лямбда-функция разбора: отделяет один уровень от \a s, обновляя \a has_more.
    // Возвращает текст уровня (string_view на данные s — он стал бы
    // недействителен при изменении s, однако мы его не сохраняем).
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
            return !filter_has_more; // '#' должен быть последним элементом.

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
