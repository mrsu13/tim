#include "tim_test.h"

#include "tim_uuid.h"

#include <string>
#include <unordered_map>
#include <unordered_set>


static const char *const CANONICAL = "{12345678-9abc-def0-1234-567890abcdef}";
static const char *const NO_BRACKETS = "12345678-9abc-def0-1234-567890abcdef";
static const char *const COMPACT = "123456789abcdef01234567890abcdef";
static const char *const NIL_CANONICAL = "{00000000-0000-0000-0000-000000000000}";


// --- Default state ----------------------------------------------------------

TIM_TEST_CASE(uuid_default_is_nil)
{
    tim::uuid u;
    TIM_CHECK(u.valid());       // Default-constructed uuid is "valid" в смысле _valid.
    TIM_CHECK(u.is_null());     // Но при этом — нулевой.
    TIM_CHECK(!u);              // operator bool → false (valid && !is_null).
}

TIM_TEST_CASE(uuid_default_stringifies_to_nil_canonical)
{
    tim::uuid u;
    TIM_CHECK(u.to_string() == NIL_CANONICAL);
    TIM_CHECK(u.to_string(tim::uuid::format::no_brackets)
              == "00000000-0000-0000-0000-000000000000");
    TIM_CHECK(u.to_string(tim::uuid::format::compact)
              == "00000000000000000000000000000000");
}


// --- Parsing ----------------------------------------------------------------

TIM_TEST_CASE(uuid_parse_canonical)
{
    tim::uuid u(std::string{ CANONICAL });
    TIM_CHECK(u.valid());
    TIM_CHECK(!u.is_null());
    TIM_CHECK(u.to_string() == CANONICAL);
}

TIM_TEST_CASE(uuid_parse_no_brackets)
{
    tim::uuid u(std::string{ NO_BRACKETS });
    TIM_CHECK(u.valid());
    TIM_CHECK(!u.is_null());
    TIM_CHECK(u.to_string() == CANONICAL);
    TIM_CHECK(u.to_string(tim::uuid::format::no_brackets) == NO_BRACKETS);
}

TIM_TEST_CASE(uuid_parse_canonical_and_nobrackets_equal)
{
    // Один и тот же UUID, записанный в двух форматах, должен
    // парситься в равные значения.
    tim::uuid a(std::string{ CANONICAL });
    tim::uuid b(std::string{ NO_BRACKETS });
    TIM_CHECK(a == b);
}

TIM_TEST_CASE(uuid_parse_compact_is_not_supported)
{
    // Compact — формат вывода без разделителей; from_string его не
    // принимает (нужны '-' между группами).
    tim::uuid u(std::string{ COMPACT });
    TIM_CHECK(!u.valid());
}

TIM_TEST_CASE(uuid_parse_invalid_too_short)
{
    tim::uuid u(std::string{ "12345" });
    TIM_CHECK(!u.valid());
}

TIM_TEST_CASE(uuid_parse_invalid_non_hex_yields_nil)
{
    // Особенность реализации: если from_string ловит ошибку внутри
    // парсинга (а не на проверке длины), он вызывает clear(), который
    // выставляет _valid = true и обнуляет поля. Результат — "valid",
    // но is_null. Здесь пинаем это поведение, чтобы случайные правки
    // не сломали его незаметно.
    tim::uuid u(std::string{ "ZZZZZZZZ-9abc-def0-1234-567890abcdef" });
    TIM_CHECK(u.is_null());
    TIM_CHECK(!u); // operator bool — false, потому что is_null.
}

TIM_TEST_CASE(uuid_parse_nullptr_cstr)
{
    tim::uuid u(static_cast<const char *>(nullptr));
    TIM_CHECK(!u.valid());
}

TIM_TEST_CASE(uuid_parse_round_trip)
{
    // Парсим, выводим, парсим — должно совпадать.
    tim::uuid a(std::string{ CANONICAL });
    tim::uuid b(a.to_string());
    tim::uuid c(a.to_string(tim::uuid::format::no_brackets));
    TIM_CHECK(a == b);
    TIM_CHECK(a == c);
}


// --- Assignment -------------------------------------------------------------

TIM_TEST_CASE(uuid_assign_from_string)
{
    tim::uuid u;
    TIM_CHECK(u.is_null());
    u = std::string{ CANONICAL };
    TIM_CHECK(!u.is_null());
    TIM_CHECK(u.to_string() == CANONICAL);
}

TIM_TEST_CASE(uuid_assign_from_uuid_copies)
{
    tim::uuid a(std::string{ CANONICAL });
    tim::uuid b;
    b = a;
    TIM_CHECK(a == b);
}

TIM_TEST_CASE(uuid_clear_yields_nil)
{
    tim::uuid u(std::string{ CANONICAL });
    TIM_CHECK(!u.is_null());
    u.clear();
    TIM_CHECK(u.is_null());
    TIM_CHECK(u.valid()); // clear() оставляет _valid = true.
}


// --- Equality and ordering --------------------------------------------------

TIM_TEST_CASE(uuid_equality_same_value)
{
    tim::uuid a(std::string{ CANONICAL });
    tim::uuid b(std::string{ CANONICAL });
    TIM_CHECK(a == b);
    TIM_CHECK(!(a != b));
}

TIM_TEST_CASE(uuid_inequality_different_value)
{
    tim::uuid a(std::string{ "{12345678-9abc-def0-1234-567890abcdef}" });
    tim::uuid b(std::string{ "{12345678-9abc-def0-1234-567890abcdee}" }); // последний бит другой
    TIM_CHECK(a != b);
}


// --- Format outputs ---------------------------------------------------------

TIM_TEST_CASE(uuid_to_string_canonical_format)
{
    tim::uuid u(std::string{ CANONICAL });
    const std::string s = u.to_string(tim::uuid::format::canonical);
    TIM_CHECK(s.size() == 38);
    TIM_CHECK(s.front() == '{');
    TIM_CHECK(s.back() == '}');
}

TIM_TEST_CASE(uuid_to_string_no_brackets_format)
{
    tim::uuid u(std::string{ CANONICAL });
    const std::string s = u.to_string(tim::uuid::format::no_brackets);
    TIM_CHECK(s.size() == 36);
    TIM_CHECK(s.front() != '{');
    TIM_CHECK(s.back() != '}');
}

TIM_TEST_CASE(uuid_to_string_compact_format)
{
    tim::uuid u(std::string{ CANONICAL });
    const std::string s = u.to_string(tim::uuid::format::compact);
    TIM_CHECK(s.size() == 32);
    TIM_CHECK(s.find('-') == std::string::npos);
    TIM_CHECK(s.find('{') == std::string::npos);
}


// --- create() ---------------------------------------------------------------

TIM_TEST_CASE(uuid_create_is_valid_and_non_null)
{
    tim::uuid u = tim::uuid::create();
    TIM_CHECK(u.valid());
    TIM_CHECK(!u.is_null());
    TIM_CHECK(u); // operator bool == true
}

TIM_TEST_CASE(uuid_create_has_dce_variant_random_version)
{
    tim::uuid u = tim::uuid::create();
    TIM_CHECK(u.uuid_variant() == tim::uuid::variant::dce);
    TIM_CHECK(u.uuid_version() == tim::uuid::version::random);
}

TIM_TEST_CASE(uuid_create_produces_unique_values)
{
    // Не строгая проверка криптографической уникальности — просто
    // вырожденный случай "две подряд сгенерированные uuid не равны".
    tim::uuid a = tim::uuid::create();
    tim::uuid b = tim::uuid::create();
    TIM_CHECK(a != b);
}


// --- Hashing (for unordered containers) -------------------------------------

TIM_TEST_CASE(uuid_works_in_unordered_set)
{
    std::unordered_set<tim::uuid> set;
    tim::uuid a = tim::uuid::create();
    tim::uuid b = tim::uuid::create();
    set.insert(a);
    set.insert(b);
    set.insert(a); // повторная вставка — должна игнорироваться
    TIM_CHECK(set.size() == 2);
    TIM_CHECK(set.count(a) == 1);
    TIM_CHECK(set.count(b) == 1);
}

TIM_TEST_CASE(uuid_works_as_unordered_map_key)
{
    std::unordered_map<tim::uuid, int> map;
    tim::uuid a = tim::uuid::create();
    map[a] = 42;
    TIM_CHECK(map[a] == 42);
    map[a] = 43;
    TIM_CHECK(map[a] == 43);
    TIM_CHECK(map.size() == 1);
}
