// Тесты базового класса tim::service.
//
// Заметка про охват: конкретные сервисы tim::post_service / user_service /
// reaction_service здесь не тестируются — их конструкторы принимают
// tim::mqtt_client&, что вносит в тестовый исполняемый файл mongoose,
// mbedTLS и весь MQTT-стек. Тесты обработчиков требуют действующего
// MQTT-брокера или специальной точки ввода сообщений, которых сейчас
// нет. Поэтому здесь
// проверяем только инварианты базы — уникальность id и сохранение name.

#include "tim_test.h"

#include "tim_service.h"

#include <string>
#include <unordered_set>


namespace
{

// Тестовый наследник: даёт возможность создать экземпляр (конструктор
// service защищён).
class probe_service : public tim::service
{

public:

    explicit probe_service(const std::string &n)
        : tim::service(n)
    {}
};

}


TIM_TEST_CASE(service_id_is_unique)
{
    probe_service a("a");
    probe_service b("b");
    probe_service c("c");

    TIM_CHECK(a.id() != b.id());
    TIM_CHECK(b.id() != c.id());
    TIM_CHECK(a.id() != c.id());
}


TIM_TEST_CASE(service_id_is_monotonic)
{
    probe_service a("a");
    probe_service b("b");

    // Идентификаторы выдаются монотонно возрастающим счётчиком —
    // у второго созданного должен быть больше, чем у первого.
    TIM_CHECK(b.id() > a.id());
}


TIM_TEST_CASE(service_name_is_preserved)
{
    probe_service post("post");
    probe_service user("user");

    TIM_CHECK(post.name() == "post");
    TIM_CHECK(user.name() == "user");
}


TIM_TEST_CASE(service_id_unique_across_many)
{
    // Стресс-проверка: 1024 сервиса — все id различны.
    std::unordered_set<std::uint64_t> seen;
    for (int i = 0; i < 1024; ++i)
    {
        probe_service s("probe");
        TIM_CHECK(seen.insert(s.id()).second);
    }
}
