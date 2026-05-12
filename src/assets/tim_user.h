#pragma once

#include "tim_uuid.h"


namespace tim
{

/**
 * Профиль пользователя TIM.
 *
 * Минимальный объект-значение, переносится между БД-слоем (tim_user_db,
 * profile_cache) и UI-слоем (prompt_service::render_post).
 */
struct user
{
    /**
     * Человекочитаемое имя для отображения.
     *
     * \return Ник, если задан; иначе первые 8 hex-символов UUID
     *         (короткий "fingerprint"); пустая строка — если id невалиден.
     */
    std::string title() const;

    /** UUID пользователя — стабильный идентификатор. */
    tim::uuid id;
    /** Открытый SSH-ключ пользователя (base64), если известен. */
    std::string pub_key;
    /** Ник для отображения (может быть пустым). */
    std::string nick;
    /** Иконка-эмодзи слева от ника (1–3 кода). */
    std::string icon;
    /** Короткий девиз/статус. */
    std::string motto;
};

}
