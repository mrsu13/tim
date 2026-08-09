#pragma once

#include <string>


namespace tim
{

/**
 * 128-битный UUID (RFC 4122) с парсингом, сериализацией и сравнениями.
 *
 * Поддерживает форматы canonical (с фигурными скобками), no_brackets
 * (классические 36 символов с дефисами) и compact (32 hex без дефисов).
 * Сравнение лексикографическое; хеш через std::hash<std::string>.
 */
class uuid
{

public:

    /**
     * Значения поля \c variant UUID. Определяют схему укладки 128 бит.
     */
    enum class variant
    {
        unknown    = -1, ///< Вариант не определён.
        ncs        =  0, ///< (0 - -) Зарезервировано под NCS (Network Computing System).
        dce        =  2, ///< (1 0 -) Distributed Computing Environment — схема, используемая TIM.
        microsoft  =  6, ///< (1 1 0) Зарезервировано под Microsoft GUID.
        reserved   =  7  ///< (1 1 1) Зарезервировано на будущее.
    };

    /**
     * Значения поля \c version UUID (имеет смысл только при variant == dce).
     */
    enum class version
    {
        unknown        = -1, ///< Версия не определена.
        time           =  1, ///< (0 0 0 1) Time-based: timestamp + clock seq + MAC.
        embedded_posix =  2, ///< (0 0 1 0) DCE Security с встроенным POSIX UID/GID.
        name           =  3, ///< (0 0 1 1) Name-based: значения из имени для всех секций.
        random         =  4  ///< (0 1 0 0) Random: случайные числа для всех секций.
    };

    uuid();

    uuid(const tim::uuid &other);

    uuid(unsigned int l, unsigned short w1, unsigned short w2,
         unsigned char b1, unsigned char b2, unsigned char b3, unsigned char b4,
         unsigned char b5, unsigned char b6, unsigned char b7, unsigned char b8);

    uuid(const std::string &text);

    uuid(const char *text);

    /**
     * Формат строкового представления UUID.
     */
    enum class format
    {
        canonical   = 1, ///< {943b573e-7a1d-4419-81b1-3308455be5f7}
        no_brackets = 2, ///< 943b573e-7a1d-4419-81b1-3308455be5f7
        compact     = 3  ///< 943b573e7a1d441981b13308455be5f7
    };

    /** \return UUID в формате canonical (с фигурными скобками). */
    inline std::string to_string() const;

    std::string to_string(const format format) const;

    bool from_string(const std::string &text);

    /** Неявное приведение к строке (через to_string() canonical). */
    inline operator std::string() const;

    bool is_null() const;

    /** \return Результат предыдущего from_string() (true, если разбор прошёл). */
    inline bool valid() const;

    /** \return true, если UUID валиден И не nil. */
    inline operator bool() const;

    void clear();

    tim::uuid &operator=(const tim::uuid &other);

    /**
     * Присваивание из строки (через from_string).
     *
     * \param text Строковое представление.
     * \return *this.
     */
    inline tim::uuid &operator=(const std::string &text);

    bool operator==(const tim::uuid &orig) const;

    /** Покомпонентное сравнение UUID (отрицание operator==). */
    inline bool operator!=(const tim::uuid &orig) const;

    bool operator<(const tim::uuid &other) const;
    bool operator>(const tim::uuid &other) const;

    static tim::uuid create();

    variant uuid_variant() const;
    version uuid_version() const;

private:

    /** Флаг успешного разбора/инициализации. */
    bool _valid;

    /** Поле data1 (4 байта). */
    unsigned int   data1;
    /** Поле data2 (2 байта). */
    unsigned short data2;
    /** Поле data3 (2 байта). */
    unsigned short data3;
    /** Поле data4 (8 байт). */
    unsigned char  data4[8];
};

}


namespace std
{

/**
 * Специализация std::hash для tim::uuid через hash от его строкового
 * представления. Подходит для unordered-контейнеров.
 */
template<>
struct hash<tim::uuid>
{

public:

    /**
     * Вычисляет хеш UUID.
     *
     * \param uuid Хешируемый UUID.
     * \return Целочисленный хеш.
     */
    inline std::size_t operator()(const tim::uuid &uuid) const
    {
        return std::hash<std::string>()(uuid.to_string());
    }
};

}


// Implementation
// Public
/**
 * Эквивалентно to_string(format::canonical).
 */
std::string tim::uuid::to_string() const
{
    return to_string(format::canonical);
}

/**
 * Неявное приведение к строке: возвращает to_string() canonical.
 *
 * \return Строковое представление UUID.
 *
 * \sa to_string()
 */
tim::uuid::operator std::string() const
{
    return to_string();
}

/**
 * \return Сохранённое значение флага valid.
 */
bool tim::uuid::valid() const
{
    return _valid;
}

/**
 * \return true только если valid() и UUID не nil.
 */
tim::uuid::operator bool() const
{
    return valid() && !is_null();
}

/**
 * Присваивание из строки: вызывает from_string().
 */
tim::uuid &tim::uuid::operator=(const std::string &text)
{
    from_string(text);
    return *this;
}

/**
 * \return true если *this и \a orig разные UUID; иначе false.
 */
bool tim::uuid::operator!=(const tim::uuid &orig) const
{
    return !(*this == orig);
}
