#include "tim_user.h"


// Открытые

/**
 * Человекочитаемое имя для отображения.
 *
 * \return Ник, если задан; иначе первые 8 hex-символов UUID
 *         (короткий "fingerprint"); пустая строка — если id невалиден.
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
