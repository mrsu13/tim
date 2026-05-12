#include "tim_test.h"

#include "tim_signal.h"

#include <functional>
#include <memory>
#include <string>


// --- Базовая семантика signal/slot -------------------------------------------

TIM_TEST_CASE(signal_default_connection_is_disconnected)
{
    tim::signal_connection c;
    TIM_CHECK(!c.connected());
}

TIM_TEST_CASE(signal_connect_returns_connected_token)
{
    tim::signal<> sig;
    auto c = sig.connect(std::function<void()>([]{}));
    TIM_CHECK(c.connected());
}

TIM_TEST_CASE(signal_emit_calls_slot)
{
    tim::signal<> sig;
    int calls = 0;
    auto c = sig.connect(std::function<void()>([&]{ ++calls; }));
    sig();
    TIM_CHECK(calls == 1);
    sig();
    TIM_CHECK(calls == 2);
}

TIM_TEST_CASE(signal_emit_passes_args)
{
    tim::signal<int, std::string> sig;
    int got_i = -1;
    std::string got_s;
    auto c = sig.connect(std::function<void(int, std::string)>(
        [&](int i, std::string s)
        {
            got_i = i;
            got_s = s;
        }));
    sig(42, std::string("hello"));
    TIM_CHECK(got_i == 42);
    TIM_CHECK(got_s == "hello");
}

TIM_TEST_CASE(signal_multiple_slots_all_fire)
{
    tim::signal<> sig;
    int a = 0, b = 0;
    auto ca = sig.connect(std::function<void()>([&]{ ++a; }));
    auto cb = sig.connect(std::function<void()>([&]{ ++b; }));
    sig();
    TIM_CHECK(a == 1);
    TIM_CHECK(b == 1);
}


// --- Отключение --------------------------------------------------------------

TIM_TEST_CASE(signal_manual_disconnect_stops_slot)
{
    tim::signal<> sig;
    int calls = 0;
    auto c = sig.connect(std::function<void()>([&]{ ++calls; }));
    c.disconnect();
    TIM_CHECK(!c.connected());
    sig();
    TIM_CHECK(calls == 0);
}

TIM_TEST_CASE(signal_raii_disconnect_on_scope_exit)
{
    tim::signal<> sig;
    int calls = 0;
    {
        auto c = sig.connect(std::function<void()>([&]{ ++calls; }));
        sig();
        TIM_CHECK(calls == 1);
    }
    // Out of scope → connection destroyed → slot disconnected.
    sig();
    TIM_CHECK(calls == 1);
}

TIM_TEST_CASE(signal_double_disconnect_is_idempotent)
{
    tim::signal<> sig;
    auto c = sig.connect(std::function<void()>([]{}));
    c.disconnect();
    c.disconnect(); // не падает
    TIM_CHECK(!c.connected());
}


// --- Move-семантика signal_connection ---------------------------------------

TIM_TEST_CASE(signal_connection_move_construct_transfers_ownership)
{
    tim::signal<> sig;
    int calls = 0;
    auto c1 = sig.connect(std::function<void()>([&]{ ++calls; }));
    TIM_CHECK(c1.connected());

    tim::signal_connection c2(std::move(c1));
    TIM_CHECK(c2.connected());

    sig();
    TIM_CHECK(calls == 1);

    // c2 уходит из области видимости (в конце теста) — слот отключается.
}

TIM_TEST_CASE(signal_connection_move_assign_disconnects_old)
{
    tim::signal<> sig;
    int a = 0, b = 0;
    auto ca = sig.connect(std::function<void()>([&]{ ++a; }));
    auto cb = sig.connect(std::function<void()>([&]{ ++b; }));

    // ca теперь должен указывать на слот b; старый слот a — отключён.
    ca = std::move(cb);
    sig();
    TIM_CHECK(a == 0);
    TIM_CHECK(b == 1);
}

TIM_TEST_CASE(signal_connection_assign_to_default_keeps_alive)
{
    tim::signal<> sig;
    int calls = 0;
    tim::signal_connection c; // disconnected
    c = sig.connect(std::function<void()>([&]{ ++calls; }));
    TIM_CHECK(c.connected());
    sig();
    TIM_CHECK(calls == 1);
}


// --- Защита от re-entrancy --------------------------------------------------

TIM_TEST_CASE(signal_slot_can_disconnect_another_slot)
{
    // Внутри одной эмиссии слот «a» отключает слот «b». Реализация
    // снимает копию списка id перед обходом, поэтому второй вызов
    // sig() уже не должен вызывать b.
    tim::signal<> sig;
    int a_calls = 0;
    int b_calls = 0;

    tim::signal_connection b_c;
    auto a_c = sig.connect(std::function<void()>(
        [&]
        {
            ++a_calls;
            b_c.disconnect();
        }));
    b_c = sig.connect(std::function<void()>([&]{ ++b_calls; }));

    sig(); // a вызывается всегда; b — зависит от порядка обхода в unordered_map.
    sig(); // здесь b точно отключён.

    TIM_CHECK(a_calls == 2);
    TIM_CHECK(b_calls <= 1);
    TIM_CHECK(!b_c.connected());
}

TIM_TEST_CASE(signal_slot_can_disconnect_itself)
{
    // Слот, отключающий сам себя во время вызова. Слот, выполняющийся
    // в этот момент, удаляется из _slots вместе со своим std::function;
    // тело лямбды после disconnect() уже не обращается к захватам.
    tim::signal<> sig;
    int self_calls = 0;
    int other_calls = 0;

    auto other_c = sig.connect(std::function<void()>([&]{ ++other_calls; }));

    auto self_c = std::make_unique<tim::signal_connection>();
    *self_c = sig.connect(std::function<void()>(
        [&self_calls, c = self_c.get()]
        {
            ++self_calls;
            c->disconnect();
        }));

    sig();
    TIM_CHECK(self_calls == 1);
    TIM_CHECK(other_calls == 1);

    sig();
    TIM_CHECK(self_calls == 1); // больше не вызывается
    TIM_CHECK(other_calls == 2);
}
