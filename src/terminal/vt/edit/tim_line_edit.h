#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>


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

    /**
     * Конструктор.
     *
     * \param term Терминал, на котором редактируется строка.
     */
    explicit line_edit(tim::vt *term);

    /** Виртуальный деструктор. */
    virtual ~line_edit();

    /** \return Терминал, к которому привязан редактор. */
    tim::vt *terminal() const;

    /** \return Текущий префикс приглашения (без ANSI-обвязки). */
    std::string prompt() const;

    /**
     * Устанавливает приглашение.
     *
     * \param prompt Строка приглашения (может быть с ANSI-кодами).
     */
    void set_prompt(const std::string &prompt);

    /** \return true, если в буфере редактирования сейчас нет символов. */
    bool empty() const;

    /** \return Текущее содержимое буфера редактирования (UTF-8). */
    std::string line() const;

    /**
     * Печатает '\\n' и выводит новое приглашение. Используется после
     * "выталкивания" сообщения сверху.
     *
     * \return true при успехе.
     */
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

    /**
     * Передаёт байты ввода редактору; обновляет состояние и возвращает
     * статус.
     *
     * \param data Сырые байты ввода.
     * \param size Размер.
     * \return Статус редактирования.
     */
    status get_line(const char *data, std::size_t size);

    /** Очищает буфер редактирования (как Ctrl+U). */
    void clear();

    /** Прячет приглашение и текущий ввод (стирает с экрана). */
    void hide();
    /** Возвращает приглашение и буфер на экран после hide(). */
    void show();

    /**
     * Включает/выключает многострочный режим.
     *
     * \param enable true — многострочный, false — однострочный.
     */
    void set_multi_line(bool enable);

    /**
     * Включает/выключает маску ввода (для паролей: отображается "***").
     *
     * \param enable true — маскировать.
     */
    void set_mask_mode(bool enable);

    /**
     * Сохраняет историю на диск (по одной строке на запись).
     *
     * \param path Путь к файлу истории.
     * \return true при успехе.
     */
    bool history_save(const std::filesystem::path &path) const;

    /**
     * Загружает историю с диска. Дубликаты и пустые строки отфильтровываются.
     *
     * \param path Путь к файлу истории.
     * \return true при успехе.
     */
    bool history_load(const std::filesystem::path &path);

    /** Тип контейнера для автодополнений (vector<string>). */
    using completions = std::vector<std::string>;
    /**
     * Колбэк-автодополнялка: получает текущий префикс, возвращает
     * список совпадающих имён.
     */
    using completer_fn = std::function<completions (const std::string &prefix)>;

    /**
     * Регистрирует функцию автодополнения. Вызывается при Tab.
     *
     * \param fn Колбэк-автодополнялка.
     */
    void set_completer(completer_fn fn);

    /**
     * Тип колбэка подсказки: получает текущую строку, возвращает текст
     * подсказки + цвет и атрибуты (bold) для отображения.
     */
    using hinter_fn = std::function<std::string (const std::string &line, int &color, int &bold)>;

    /**
     * Регистрирует функцию подсказки. Если задана, её результат
     * отображается справа от приглашения перед редактируемой строкой.
     *
     * \param fn Колбэк подсказки.
     */
    void set_hinter(hinter_fn fn);

private:

    /** PIMPL: буфер, история, флаги режимов, колбэки. */
    std::unique_ptr<tim::p::line_edit> _d;
};

}
