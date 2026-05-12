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

    /**
     * Конструктор из C-строки.
     *
     * \param topic Указатель на нуль-терминированную строку.
     */
    mqtt_topic(const char *topic);

    /**
     * Конструктор из string_view.
     *
     * \param topic Срез строки с именем топика.
     */
    mqtt_topic(std::string_view topic);

    /**
     * Конструктор-перемещение из std::string.
     *
     * \param topic Готовая строка-имя топика.
     */
    mqtt_topic(std::string topic);

    /** \return true, если топик пуст. */
    bool empty() const noexcept;

    /** \return Длина имени топика в байтах. */
    std::size_t size() const noexcept;

    /** \return Указатель на байты имени (не обязательно нуль-терминирован). */
    const char *data() const noexcept;

    /** \return Указатель на нуль-терминированную C-строку имени топика. */
    const char *c_str() const noexcept;

    /** \return Ссылка на хранящуюся std::string. */
    const std::string &str() const noexcept;

    /** \return string_view имени топика. */
    std::string_view view() const noexcept;

    /** Неявное приведение к string_view для API, ожидающих view. */
    operator std::string_view() const noexcept;

    /**
     * Возвращает последний уровень топика (всё после последнего '/').
     *
     * \return Срез последнего уровня. Если '/' нет — весь топик.
     *         Пустая строка — если сам топик пуст.
     */
    std::string_view last_level() const noexcept;

    /**
     * Возвращает родительский топик (без последнего уровня).
     *
     * \return Топик без последнего уровня; пустой, если в исходном не было '/'.
     */
    tim::mqtt_topic parent() const;

    /**
     * Добавляет уровень к топику (mutating).
     *
     * \param level Имя нового уровня; не должно содержать '/' (assert).
     * \return *this для chaining.
     */
    tim::mqtt_topic &operator/=(std::string_view level);

    /** Сравнение по содержимому строки. */
    bool operator==(const tim::mqtt_topic &other) const noexcept;
    /** Сравнение по содержимому строки. */
    bool operator!=(const tim::mqtt_topic &other) const noexcept;

private:

    /** Хранилище имени топика. */
    std::string _value;
};

/**
 * Свободная функция-перегруз / для построения новых топиков.
 *
 * \param lhs Базовый топик (передаётся по значению — будет модифицирован).
 * \param level Новый уровень.
 * \return Новый топик lhs + '/' + level.
 */
tim::mqtt_topic operator/(tim::mqtt_topic lhs, std::string_view level);

/**
 * Проверка соответствия MQTT-фильтру согласно семантике MQTT 3.1.1:
 * '+' — ровно один уровень, '#' — остаток (включая ноль уровней)
 * и должен быть последним токеном фильтра.
 *
 * \param topic Конкретный топик публикации.
 * \param filter Фильтр подписки (могут быть метасимволы '+' и '#').
 * \return true, если \a topic соответствует \a filter.
 */
bool topic_matches(std::string_view topic, std::string_view filter);

}
