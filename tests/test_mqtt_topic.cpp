#include "tim_test.h"

#include "tim_mqtt_topic.h"


// --- tim::mqtt_topic ---------------------------------------------------------

TIM_TEST_CASE(mqtt_topic_default_is_empty)
{
    tim::mqtt_topic t;
    TIM_CHECK(t.empty());
    TIM_CHECK(t.size() == 0);
    TIM_CHECK(t.str() == "");
}

TIM_TEST_CASE(mqtt_topic_from_cstr)
{
    tim::mqtt_topic t("post/+/+");
    TIM_CHECK(!t.empty());
    TIM_CHECK(t.size() == 8);
    TIM_CHECK(t.str() == "post/+/+");
}

TIM_TEST_CASE(mqtt_topic_from_nullptr_is_empty)
{
    tim::mqtt_topic t(static_cast<const char *>(nullptr));
    TIM_CHECK(t.empty());
}

TIM_TEST_CASE(mqtt_topic_last_level)
{
    TIM_CHECK(tim::mqtt_topic("a/b/c").last_level() == "c");
    TIM_CHECK(tim::mqtt_topic("a").last_level() == "a");
    TIM_CHECK(tim::mqtt_topic("").last_level() == "");
    TIM_CHECK(tim::mqtt_topic("a/").last_level() == "");
}

TIM_TEST_CASE(mqtt_topic_parent)
{
    TIM_CHECK(tim::mqtt_topic("a/b/c").parent().str() == "a/b");
    TIM_CHECK(tim::mqtt_topic("a/b").parent().str() == "a");
    TIM_CHECK(tim::mqtt_topic("a").parent().empty());
    TIM_CHECK(tim::mqtt_topic("").parent().empty());
}

TIM_TEST_CASE(mqtt_topic_append_level)
{
    tim::mqtt_topic t("post");
    t /= "abc";
    TIM_CHECK(t.str() == "post/abc");

    t /= "def";
    TIM_CHECK(t.str() == "post/abc/def");
}

TIM_TEST_CASE(mqtt_topic_append_to_empty)
{
    tim::mqtt_topic t;
    t /= "user";
    TIM_CHECK(t.str() == "user");
}

TIM_TEST_CASE(mqtt_topic_operator_slash_is_pure)
{
    tim::mqtt_topic base("post");
    tim::mqtt_topic combined = base / "abc";
    TIM_CHECK(combined.str() == "post/abc");
    // Базовый топик не должен мутировать.
    TIM_CHECK(base.str() == "post");
}

TIM_TEST_CASE(mqtt_topic_equality)
{
    TIM_CHECK(tim::mqtt_topic("a/b") == tim::mqtt_topic("a/b"));
    TIM_CHECK(!(tim::mqtt_topic("a/b") == tim::mqtt_topic("a/c")));
    TIM_CHECK(tim::mqtt_topic("a/b") != tim::mqtt_topic("a/c"));
    TIM_CHECK(tim::mqtt_topic("") == tim::mqtt_topic());
}

TIM_TEST_CASE(mqtt_topic_implicit_string_view)
{
    const tim::mqtt_topic t("post/abc");
    const std::string_view sv = t;
    TIM_CHECK(sv == "post/abc");
}


// --- tim::topic_matches ------------------------------------------------------

TIM_TEST_CASE(topic_matches_literal)
{
    TIM_CHECK(tim::topic_matches("user/connect", "user/connect"));
    TIM_CHECK(!tim::topic_matches("user/connect", "user/disconnect"));
    TIM_CHECK(!tim::topic_matches("user/connect/x", "user/connect"));
    TIM_CHECK(!tim::topic_matches("user", "user/connect"));
}

TIM_TEST_CASE(topic_matches_plus_single_level)
{
    TIM_CHECK(tim::topic_matches("post/123", "post/+"));
    TIM_CHECK(tim::topic_matches("post/abc-def", "post/+"));
    // + по спецификации требует ровно одного уровня — пустого
    // (между двумя слэшами) тоже не должно быть.
    TIM_CHECK(tim::topic_matches("post/", "post/+"));
    // + не должен поглощать вложенный уровень.
    TIM_CHECK(!tim::topic_matches("post/123/sub", "post/+"));
    // + не пустота — если уровня нет, не матчится.
    TIM_CHECK(!tim::topic_matches("post", "post/+"));
}

TIM_TEST_CASE(topic_matches_plus_in_middle)
{
    TIM_CHECK(tim::topic_matches("user/setnick/abc", "user/setnick/+"));
    TIM_CHECK(tim::topic_matches("post/abc/def", "post/+/+"));
    TIM_CHECK(!tim::topic_matches("post/abc", "post/+/+"));
    TIM_CHECK(!tim::topic_matches("post/abc/def/ghi", "post/+/+"));
}

TIM_TEST_CASE(topic_matches_hash_trailing)
{
    TIM_CHECK(tim::topic_matches("sport", "sport/#"));            // 0 уровней
    TIM_CHECK(tim::topic_matches("sport/tennis", "sport/#"));     // 1
    TIM_CHECK(tim::topic_matches("sport/tennis/player1", "sport/#")); // 2+
    TIM_CHECK(!tim::topic_matches("other", "sport/#"));
}

TIM_TEST_CASE(topic_matches_hash_only)
{
    TIM_CHECK(tim::topic_matches("anything", "#"));
    TIM_CHECK(tim::topic_matches("a/b/c", "#"));
    TIM_CHECK(tim::topic_matches("", "#"));
}

TIM_TEST_CASE(topic_matches_hash_must_be_last)
{
    // По спецификации '#' допустим только как последний токен.
    TIM_CHECK(!tim::topic_matches("a/b", "#/b"));
    TIM_CHECK(!tim::topic_matches("a/b", "a/#/b"));
}

TIM_TEST_CASE(topic_matches_mixed_wildcards)
{
    TIM_CHECK(tim::topic_matches("react_event/abc/def", "react_event/+/+"));
    TIM_CHECK(tim::topic_matches("a/b/c/d/e", "a/+/+/#"));
    TIM_CHECK(tim::topic_matches("a/b/c", "a/+/c"));
    TIM_CHECK(!tim::topic_matches("a/c", "a/+/c"));
}

TIM_TEST_CASE(topic_matches_empty)
{
    TIM_CHECK(tim::topic_matches("", ""));
    TIM_CHECK(!tim::topic_matches("a", ""));
    TIM_CHECK(!tim::topic_matches("", "a"));
}
