#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "tim_pimpl.h"


namespace tim
{

class vt;

namespace p
{

struct line_edit;

}

/**
 * Однострочный/многострочный редактор ввода с историей, автодополнением
 * и поддержкой подсказок (hints).
 *
 * Конвертирует входящие сырые байты (буквы, ESC-последовательности
 * клавиш-стрелок, Ctrl-сочетания) в обновлённую отображаемую строку
 * и возвращает статус для каждого write-вызова.
 */
class line_edit
{

public:

    explicit line_edit(tim::vt *term);

    virtual ~line_edit();

    tim::vt *terminal() const;

    std::string prompt() const;

    void set_prompt(const std::string &prompt);

    bool empty() const;

    std::string line() const;

    bool new_line();

    /**
     * Статус, возвращаемый get_line() после обработки очередной порции
     * входных байт.
     */
    enum class status
    {
        finished,    ///< Редактирование завершено; можно использовать line().
        in_progress, ///< Редактирование продолжается; вызывайте get_line() ещё.
        exit,        ///< Пользователь нажал Ctrl+D.
        interrupted, ///< Пользователь нажал Ctrl+C.
        error        ///< Ошибка чтения/записи в потоки терминала.
    };

    status get_line(const char *data, std::size_t size);

    void clear();

    void hide();

    void show();

    void set_multi_line(bool enable);

    void set_mask_mode(bool enable);

    bool history_save(const std::filesystem::path &path) const;

    bool history_load(const std::filesystem::path &path);

    /** Тип контейнера для автодополнений (vector<string>). */
    using completions = std::vector<std::string>;
    /**
     * Обработчик-автодополнялка: получает текущий префикс, возвращает
     * список совпадающих имён.
     */
    using completer_fn = std::function<completions (const std::string &prefix)>;

    void set_completer(completer_fn fn);

    /**
     * Тип обработчика подсказки: получает текущую строку, возвращает
     * текст подсказки + цвет и атрибуты (bold) для отображения.
     */
    using hinter_fn = std::function<std::string (const std::string &line, int &color, int &bold)>;

    void set_hinter(hinter_fn fn);

private:

    /** PIMPL: буфер, история, флаги режимов, обработчики. */
    tim::pimpl<tim::p::line_edit> _d;
};

}
