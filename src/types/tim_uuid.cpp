#include "tim_uuid.h"

#include "tim_endian.h"

#include <random>
#include <limits>

#include <cstring>


template<typename Integral>
void tim_to_hex(char *&dst, Integral value)
{
    static const char digits[] = "0123456789abcdef";

    value = tim::to_be(value);

    const char* p = reinterpret_cast<const char *>(&value);

    for (unsigned int i = 0; i < sizeof(Integral); ++i, dst += 2)
    {
        unsigned int j = (p[i] >> 4) & 0xf;
        dst[0] = digits[j];
        j = p[i] & 0xf;
        dst[1] = digits[j];
    }
}

template<typename Integral>
bool tim_from_hex(const char *&src, Integral &value)
{
    value = 0;

    for (unsigned int i = 0; i < sizeof(Integral) * 2; ++i)
    {
        int ch = *src++;
        int tmp;
        if (ch >= '0' && ch <= '9')
            tmp = ch - '0';
        else if (ch >= 'a' && ch <= 'f')
            tmp = ch - 'a' + 10;
        else if (ch >= 'A' && ch <= 'F')
            tmp = ch - 'A' + 10;
        else
            return false;

        value = value * 16 + tmp;
    }

    return true;
}

void tim_uuid_to_hex(char *&dst, const unsigned int &d1, const unsigned short &d2, const unsigned short &d3,
                     const unsigned char (&d4)[8], const bool curly_brackets = true)
{
    if (curly_brackets)
        *dst++ = '{';

    tim_to_hex(dst, d1);
    *dst++ = '-';
    tim_to_hex(dst, d2);
    *dst++ = '-';
    tim_to_hex(dst, d3);
    *dst++ = '-';
    for (int i = 0; i < 2; i++)
        tim_to_hex(dst, d4[i]);
    *dst++ = '-';
    for (int i = 2; i < 8; i++)
        tim_to_hex(dst, d4[i]);

    *dst = curly_brackets ? '}' : 0;
}

void tim_uuid_to_compact_hex(char *&dst,
                             const unsigned int &d1, const unsigned short &d2,
                             const unsigned short &d3, const unsigned char (&d4)[8])
{
    tim_to_hex(dst, d1);
    tim_to_hex(dst, d2);
    tim_to_hex(dst, d3);
    for (int i = 0; i < 2; i++)
        tim_to_hex(dst, d4[i]);
    for (int i = 2; i < 8; i++)
        tim_to_hex(dst, d4[i]);
    *dst = 0;
}

bool tim_uuid_from_hex(const char *&src,
                       unsigned int &d1, unsigned short &d2,
                       unsigned short &d3, unsigned char (&d4)[8])
{
    if (*src == '{')
        src++;
    if (!tim_from_hex(src, d1)
            || *src++ != '-'
            || !tim_from_hex(src, d2)
            || *src++ != '-'
            || !tim_from_hex(src, d3)
            || *src++ != '-'
            || !tim_from_hex(src, d4[0])
            || !tim_from_hex(src, d4[1])
            || *src++ != '-'
            || !tim_from_hex(src, d4[2])
            || !tim_from_hex(src, d4[3])
            || !tim_from_hex(src, d4[4])
            || !tim_from_hex(src, d4[5])
            || !tim_from_hex(src, d4[6])
            || !tim_from_hex(src, d4[7]))
        return false;

    return true;
}

/** \class tim::uuid

    \brief Класс tim::uuid хранит универсально уникальный идентификатор (UUID).

    UUID (Universally Unique IDentifier) — стандартный способ уникальной
    идентификации сущностей в распределённой вычислительной среде. UUID —
    это 16-байтное (128-битное) число, порождаемое алгоритмом, гарантирующим
    уникальность в той среде, где он используется. Часто вместо UUID
    употребляют сокращение GUID (Globally Unique IDentifier) — это то же
    самое.

    Поле variant
    ------------

    Фактически GUID — это один из вариантов UUID. В употреблении несколько
    вариантов. Каждый UUID содержит битовое поле, определяющее, какой это
    вариант. Вызовите variant(), чтобы узнать тип UUID, который содержится
    в экземпляре tim::uuid. Метод извлекает три старших бита 8-го байта из
    16. В tim::uuid 8-й байт — это tim::uuid::data4[0]. Если вы создаёте
    экземпляры tim::uuid через конструктор, принимающий все числовые
    значения параметрами, используйте следующую таблицу для задания
    трёх старших бит параметра b1, который попадает в tim::uuid::data4[0]
    и содержит поле variant в этих трёх старших битах. В таблице 'x'
    означает «не имеет значения».

    msb0 | msb1 | msb2 | Variant
    :---:|:----:|:----:|--------
    0    | x    | x    | Ncs (Network Computing System)
    1    | 0    | x    | Dce (Distributed Computing Environment)
    1    | 1    | 0    | Microsoft (GUID)
    1    | 1    | 1    | Зарезервировано на будущее

    Поле version
    ------------

    Если variant() возвращает tim::uuid::Dce, то UUID также содержит поле
    version в четырёх старших битах tim::uuid::data3, и можно вызвать
    version(), чтобы узнать, какую версию содержит ваш tim::uuid. Если
    вы создаёте экземпляры tim::uuid через конструктор, принимающий все
    числовые значения параметрами, используйте таблицу ниже для задания
    четырёх старших бит параметра w2, который попадает в tim::uuid::data3
    и содержит поле version в своих четырёх старших битах.

    msb0 | msb1 | msb2 | msb3 | version
    :---:|:----:|:----:|:----:|--------
    0    | 0    | 0    | 1    | Time
    0    | 0    | 1    | 0    | Embedded POSIX
    0    | 0    | 1    | 1    | Name
    0    | 1    | 0    | 0    | Random

    Раскладка полей для версий Dce, перечисленных в таблице выше, описана
    в [спецификации UUID Network Working Group](http://www.ietf.org/rfc/rfc4122.txt).

    Большинство платформ предоставляет утилиту для генерации новых
    UUID — например, uuidgen и guidgen. Можно также использовать
    create(). UUID, порождённые create(), имеют тип random: их биты
    tim::uuid::version устанавливаются в tim::uuid::Random, а биты
    tim::uuid::Variant — в tim::uuid::Dce. Остальная часть UUID
    заполняется случайными числами. Теоретически это значит, что
    есть небольшой шанс получить от create() не уникальный UUID. Но это
    [очень небольшой шанс](http://en.wikipedia.org/wiki/Universally_Unique_Identifier#Random_UUID_probability_of_duplicates).

    UUID можно конструировать из числовых значений, из строк или с помощью
    статической функции create(). Их можно преобразовать в строку методом
    toString(). У UUID есть variant() и version(); nil-UUID возвращают
    true из empty().
*/

/** Конструирует nil-UUID (все нули, valid=false).

    to_string() выведет nil-UUID как
    "{00000000-0000-0000-0000-000000000000}".
*/
tim::uuid::uuid()
    : _valid(true)
    , data1(0)
    , data2(0)
    , data3(0)
    , data4{}
{
}

/**
 * Копирует другой UUID.
 *
 * \param other Источник копирования.
 */
tim::uuid::uuid(const tim::uuid &other)
    : _valid(other._valid)
    , data1(other.data1)
    , data2(other.data2)
    , data3(other.data3)
    , data4{}
{
    std::memcpy(data4, other.data4, sizeof(data4));
}

/** Конструирует UUID из 11 целочисленных частей \a l, \a w1, \a w2,
    \a b1, \a b2, \a b3, \a b4, \a b5, \a b6, \a b7, \a b8.

    \param l 32-битный data1.
    \param w1 16-битный data2.
    \param w2 16-битный data3.
    \param b1 Первый байт data4.
    \param b2 Второй байт data4.
    \param b3 Третий байт data4.
    \param b4 Четвёртый байт data4.
    \param b5 Пятый байт data4.
    \param b6 Шестой байт data4.
    \param b7 Седьмой байт data4.
    \param b8 Восьмой байт data4.
*/
tim::uuid::uuid(unsigned int l, unsigned short w1, unsigned short w2,
               unsigned char b1, unsigned char b2, unsigned char b3, unsigned char b4,
               unsigned char b5, unsigned char b6, unsigned char b7, unsigned char b8)
    : _valid(true)
    , data1(l)
    , data2(w1)
    , data3(w2)
    , data4{}
{
    data4[0] = b1;
    data4[1] = b2;
    data4[2] = b3;
    data4[3] = b4;
    data4[4] = b5;
    data4[5] = b6;
    data4[6] = b7;
    data4[7] = b8;
}

/** Парсит UUID из std::string. Строка \a text должна быть оформлена
    как пять hex-полей, разделённых '-', например
    "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}", где 'x' — hex-цифра.
    Показанные фигурные скобки необязательны, но обычно их пишут.
    Если разбор не удался, создаётся nil-UUID. Соответствие пяти
    hex-полей публичным членам tim::uuid описано в to_string().

    \param text Строковое представление в любом из поддерживаемых
                форматов (canonical / no_brackets / compact).

    \sa to_string(), tim::uuid()
*/
tim::uuid::uuid(const std::string &text)
    : _valid(false)
    , data1(0)
    , data2(0)
    , data3(0)
    , data4{}
{
    from_string(text);
}

/**
 * Парсит UUID из C-строки.
 *
 * \param text Нуль-терминированная строка.
 */
tim::uuid::uuid(const char *text)
    : _valid(false)
    , data1(0)
    , data2(0)
    , data3(0)
    , data4{}
{
    if (!text)
        return;

    if (!tim_uuid_from_hex(text, data1, data2, data3, data4))
    {
        clear();
        return;
    }

    _valid = true;
}

/** Возвращает строковое представление этого tim::uuid. Строка
    оформлена как пять hex-полей, разделённых '-' и заключённых в
    фигурные скобки, т.е. "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}",
    где 'x' — hex-цифра. Слева направо пять hex-полей получаются из
    четырёх публичных членов tim::uuid следующим образом:

    Поле № | Источник
    :-----:|---------
    1      | data1
    2      | data2
    3      | data3
    4      | data4[0] .. data4[1]
    5      | data4[2] .. data4[7]

    \param format Желаемый формат.
    \return Строковое представление.
*/
std::string tim::uuid::to_string(const format format) const
{
    char buffer[39];
    buffer[38] = 0;

    char *data = buffer;
    switch (format)
    {
        case format::canonical:
            tim_uuid_to_hex(data, data1, data2, data3, data4, true);
            break;

        case format::no_brackets:
            tim_uuid_to_hex(data, data1, data2, data3, data4, false);
            break;

        case format::compact:
            tim_uuid_to_compact_hex(data, data1, data2, data3, data4);
            break;
    }
    return buffer;
}

/**
 * Парсит UUID из строки, заменяя текущее значение.
 *
 * \param text Строковое представление.
 * \return true при успешном разборе.
 */
bool tim::uuid::from_string(const std::string &text)
{
    if (text.length() < 36)
        return false;

    const char *data = text.c_str();

    if (*data == '{' && text.length() < 37)
        return false;

    if (!tim_uuid_from_hex(data, data1, data2, data3, data4))
    {
        clear();
        return false;
    }

    _valid = true;

    return true;
}

/** Возвращает true, если это nil-UUID
    {00000000-0000-0000-0000-000000000000}; иначе возвращает false.

    \return true, если все 128 бит нулевые (nil-UUID).
*/
bool tim::uuid::is_null() const
{
    return data4[0] == 0 && data4[1] == 0 && data4[2] == 0 && data4[3] == 0 &&
           data4[4] == 0 && data4[5] == 0 && data4[6] == 0 && data4[7] == 0 &&
           data1 == 0 && data2 == 0 && data3 == 0;
}

/** Сбрасывает все поля в ноль и помечает UUID как invalid. */
void tim::uuid::clear()
{
    data1 = 0;
    data2 = 0;
    data3 = 0;
    std::memset(data4, 0, sizeof(data4));
    _valid = true;
}

/**
 * Копи-присваивание.
 *
 * \param other Источник.
 * \return *this.
 */
tim::uuid &tim::uuid::operator=(const tim::uuid &other)
{
    data1 = other.data1;
    data2 = other.data2;
    data3 = other.data3;
    _valid = other._valid;
    std::memcpy(data4, other.data4, sizeof(data4));

    return *this;
}

/** Покомпонентное сравнение UUID.

    \return true, если этот tim::uuid и \a orig идентичны; иначе false.
*/
bool tim::uuid::operator==(const tim::uuid &orig) const
{
    if (data1 != orig.data1
            || data2 != orig.data2
            || data3 != orig.data3)
        return false;

    for(unsigned i = 0; i < 8; ++i)
        if (data4[i] != orig.data4[i])
            return false;

    return true;
}

/** Возвращает значение поля variant UUID. Если возвращаемое значение —
    tim::uuid::Dce, вызовите uuid_version(), чтобы узнать, какая раскладка
    используется. nil-UUID считается имеющим неизвестный variant.

    \return Вариант (см. enum variant).
    \sa uuid_version()
*/
tim::uuid::variant tim::uuid::uuid_variant() const
{
    if (is_null())
        return variant::unknown;
    // Проверяем 3 старших бита data4[0].
    if ((data4[0] & 0x80) == 0x00)
        return variant::ncs;
    else if ((data4[0] & 0xC0) == 0x80)
        return variant::dce;
    else if ((data4[0] & 0xE0) == 0xC0)
        return variant::microsoft;
    else if ((data4[0] & 0xE0) == 0xE0)
        return variant::reserved;
    return variant::unknown;
}

/** Возвращает поле version UUID, если поле variant равно
    tim::uuid::variant::dce. Иначе возвращает
    tim::uuid::version::unknown.

    \return Версия (см. enum version).
    \sa variant()
*/
tim::uuid::version tim::uuid::uuid_version() const
{
    // Проверяем 4 старших бита data3.
    version ver = (version)(data3 >> 12);
    if (is_null()
            || (uuid_variant() != variant::dce)
            || ver < version::time
            || ver > version::random)
        return version::unknown;
    return ver;
}


#define ISLESS(f1, f2) if (f1 != f2) return (f1 < f2);

/** Лексикографическое сравнение (для упорядоченных контейнеров).

    Возвращает true, если этот tim::uuid имеет такое же поле variant,
    как \a other, и лексикографически предшествует \a other. Если у
    \a other другое поле variant, результат определяется сравнением
    двух вариантов.

    \sa variant()
*/
bool tim::uuid::operator<(const tim::uuid &other) const
{
    if (uuid_variant() != other.uuid_variant())
        return uuid_variant() < other.uuid_variant();

    ISLESS(data1, other.data1);
    ISLESS(data2, other.data2);
    ISLESS(data3, other.data3);
    for (int n = 0; n < 8; ++n)
        ISLESS(data4[n], other.data4[n]);

    return false;
}


#define ISMORE(f1, f2) if (f1 != f2) return (f1 > f2);

/** Лексикографическое сравнение (для упорядоченных контейнеров).

    Возвращает true, если этот tim::uuid имеет такое же поле variant,
    как \a other, и лексикографически следует за \a other. Если у
    \a other другое поле variant, результат определяется сравнением
    двух вариантов.

    \sa variant()
*/
bool tim::uuid::operator>(const tim::uuid &other) const
{
    if (uuid_variant() != other.uuid_variant())
        return uuid_variant() > other.uuid_variant();

    ISMORE(data1, other.data1);
    ISMORE(data2, other.data2);
    ISMORE(data3, other.data3);
    for (int n = 0; n < 8; ++n)
        ISMORE(data4[n], other.data4[n]);

    return false;
}

/** Генерирует новый случайный UUID v4 (DCE, random).

    \return Свежий UUID с вариантом tim::uuid::Dce и версией
            tim::uuid::Random.

    \sa variant(), version()
*/

tim::uuid tim::uuid::create()
{
    static std::random_device rd;
    static std::default_random_engine rng(rd());

    tim::uuid result;

    unsigned *data = &(result.data1);

    std::uniform_int_distribution<> dist(0, std::numeric_limits<unsigned>::max());
    *data       = dist(rng);
    *(data + 1) = dist(rng);
    *(data + 2) = dist(rng);
    *(data + 3) = dist(rng);

    result.data4[0] = (result.data4[0] & 0x3F) | 0x80; // UV_DCE — вариант DCE.
    result.data3 = (result.data3 & 0x0FFF) | 0x4000;   // UV_Random — версия random.

    return result;
}
