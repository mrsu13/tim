#include "tim_user.h"


// Открытые

/**
 * Возвращает человекочитаемое имя для отображения: ник+иконка,
 * если оба есть; одного из двух, если есть только он; иначе
 * первые 8 hex-символов UUID как короткий "fingerprint".
 */
std::string tim::user::title() const
{
    if (!icon.empty() && !nick.empty())
        return icon + ' ' + nick;
    if (!nick.empty())
        return nick;
    if (!icon.empty())
        return icon;
    return id.valid()
                ? id.to_string(tim::uuid::format::compact).substr(0, 8)
                : std::string{};
}
