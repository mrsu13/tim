#include "tim_line_edit.h"

#include "tim_line_edit_p.h"

#include "tim_a_protocol.h"
#include "tim_file_tools.h"
#include "tim_string_tools.h"
#include "tim_trace.h"
#include "tim_vt.h"

#include "utf8/utf8.h"

#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>


/**
 * \class tim::line_edit
 *
 * \brief Терминальный редактор строки. Поддерживается автодополнение.
 *
 * Эта реализация основана на <https://github.com/antirez/linenoise.git>.
 */

// Public

/**
 * Конструктор.
 *
 * \param term Терминал, на котором редактируется строка.
 */
tim::line_edit::line_edit(tim::vt *term)
    : _d(new tim::p::line_edit())
{
    assert(term);

    _d->_terminal = term;
    _d->_cols = _d->_terminal->cols();
}

/** Виртуальный деструктор. */
tim::line_edit::~line_edit() = default;

/** \return Терминал, к которому привязан редактор. */
tim::vt *tim::line_edit::terminal() const
{
    return _d->_terminal;
}

/** \return Текущий префикс приглашения (без ANSI-обвязки). */
std::string tim::line_edit::prompt() const
{
    return tim::from_wstring(_d->_prompt);
}

/**
 * Устанавливает приглашение.
 *
 * \param prompt Строка приглашения (может быть с ANSI-кодами).
 */
void tim::line_edit::set_prompt(const std::string &prompt)
{
    _d->_prompt = tim::to_wstring(prompt);
    _d->_plen = tim::vt::strlen(prompt);
}

/** \return true, если в буфере редактирования сейчас нет символов. */
bool tim::line_edit::empty() const
{
    return _d->_line.empty();
}

/**
 * Вызывается, когда get_line() вернул status::finished.
 *
 * \return Текущее содержимое буфера редактирования (UTF-8) —
 *         отредактированная строка.
 */
std::string tim::line_edit::line() const
{
    return tim::from_wstring(_d->_line);
}

/**
 * Печатает '\\n' и выводит новое приглашение. Используется после
 * "выталкивания" сообщения сверху.
 *
 * Начинает редактирование строки:
 *
 * 1. Выводит приглашение.
 * 2. Возвращает управление вызывающему, который должен вызывать
 *    get_line() каждый раз, когда из входного потока приходят данные.
 *
 * Порядок использования: сначала вызывается new_line(), затем
 * get_line() — до тех пор, пока тот не вернёт \c status::finished,
 * \c status::exit или \c status::error.
 *
 * Между вызовами get_line() можно вызывать hide() и show(), если
 * необходимо вывести какой-то приходящий асинхронно ввод, не смешивая
 * его с текущей редактируемой строкой.
 *
 * \return \c true при успехе и \c false, если запись в выход не удалась.
 *
 * \sa get_line()
 */
bool tim::line_edit::new_line()
{
    _d->_in_completion = false;
    _d->_old_pos = _d->_pos = 0;
    _d->_cols = _d->_terminal->cols();
    _d->_old_rows = 0;
    _d->_history_idx = 0;

    _d->_line.clear();

   /* Последняя запись истории — это всегда наш текущий буфер;
    * изначально это просто пустая строка. */
    _d->_history.emplace_back(L"");

    const std::string p = prompt();
    return (!_d->_line_count++
                        || _d->_terminal->protocol()->write("\n", 1))
                    && _d->_terminal->protocol()->write(p.c_str(), p.size());
}

/**
 * Передаёт байты ввода редактору; обновляет состояние и возвращает
 * статус. Вызывается для обработки пользовательского ввода, когда во
 * входном потоке появились данные.
 *
 * \param data Сырые байты ввода.
 * \param size Размер.
 * \return Статус редактирования — проверять, завершено ли
 *         редактирование строки.
 *
 * \sa new_line()
 */
tim::line_edit::status tim::line_edit::get_line(const char *data, std::size_t size)
{
    assert(data);

    if (!size)
        return status::in_progress;

    std::int32_t c;
    {
        const utf8_int8_t *next = utf8codepoint((const utf8_int8_t *)data, &c);
        const int d = next - (const utf8_int8_t *)data;
        data += d;
        size -= d;
    }

    if ((_d->_in_completion
                || c == (char)tim::key::tab)
            && _d->_completer)
    {
        c = _d->complete_line(c);
        if (c < 0)
            return status::error;
        if (c == 0)
            return status::in_progress;
    }

    char seq[3];

    switch (c)
    {
        case (char)tim::key::enter:
        case (char)tim::key::cr:
            if (!_d->_history.empty())
                _d->_history.pop_back();
            if (_d->_ml_mode)
                _d->edit_move_end();
            if (_d->_hinter)
            {
                /* Принудительная перерисовка без подсказок, чтобы
                 * после перевода строки оставить предыдущую строку
                 * в том виде, в котором её ввёл пользователь. */
                hinter_fn hc = _d->_hinter;
                _d->_hinter = nullptr;
                _d->refresh_line();
                _d->_hinter = hc;
            }
            _d->history_add(_d->_line);
            return status::finished;

        case (char)tim::key::ctrl_c:
            _d->_terminal->protocol()->write_str("^C");
            clear();
            return status::interrupted;

        case (char)tim::key::backspace:
        case (char)tim::key::ctrl_h:
            _d->edit_backspace();
            break;

        case (char)tim::key::ctrl_d: /* Удалить символ справа от курсора;
                                        если строка пуста — повести себя
                                        как end-of-file. */
            if (!_d->_line.empty())
                _d->edit_delete();
            else
            {
                if (!_d->_history.empty())
                    _d->_history.pop_back();
                return status::exit;
            }
            break;

        case (char)tim::key::ctrl_t: /* Меняет местами текущий и предыдущий символы. */
            if (_d->_pos > 0
                    && _d->_pos < _d->_line.size())
            {
                int aux = _d->_line[_d->_pos - 1];
                _d->_line[_d->_pos - 1] = _d->_line[_d->_pos];
                _d->_line[_d->_pos] = aux;
                if (_d->_pos != _d->_line.size() - 1)
                    ++_d->_pos;
                _d->refresh_line();
            }
            break;

        case (char)tim::key::ctrl_b:
            _d->edit_move_left();
            break;

        case (char)tim::key::ctrl_f:
            _d->edit_move_right();
            break;

        case (char)tim::key::ctrl_p:
            _d->edit_history_next(tim::p::line_edit::history_dir::prev);
            break;

        case (char)tim::key::ctrl_n:
            _d->edit_history_next(tim::p::line_edit::history_dir::next);
            break;

        case (char)tim::key::esc: /* ESC-последовательность. */
            /* Читаем следующие два байта, представляющие
             * ESC-последовательность. Используем два чтения, чтобы
             * корректно работать с медленными терминалами, выдающими
             * символы с задержкой. */
            if (size < 2)
                break;
            seq[0] = *data++;
            seq[1] = *data++;
            size -= 2;

            /* Последовательности ESC [. */
            if (seq[0] == '[')
            {
                if (seq[1] >= '0'
                        && seq[1] <= '9')
                {
                    /* Расширенная ESC-последовательность, читаем ещё байт. */
                    if (!size)
                        break;
                    seq[2] = *data++;
                    --size;
                    if (seq[2] == '~')
                    {
                        switch (seq[1])
                        {
                            case '3': /* Клавиша Delete. */
                                _d->edit_delete();
                                break;
                        }
                    }
                }
                else
                {
                    switch (seq[1])
                    {
                        case 'A': /* Вверх. */
                            _d->edit_history_next(tim::p::line_edit::history_dir::prev);
                            break;
                        case 'B': /* Вниз. */
                            _d->edit_history_next(tim::p::line_edit::history_dir::next);
                            break;
                        case 'C': /* Вправо. */
                            _d->edit_move_right();
                            break;
                        case 'D': /* Влево. */
                            _d->edit_move_left();
                            break;
                        case 'H': /* Home. */
                            _d->edit_move_home();
                            break;
                        case 'F': /* End. */
                            _d->edit_move_end();
                            break;
                    }
                }
            }

            /* Последовательности ESC O. */
            else if (seq[0] == 'O')
            {
                switch (seq[1])
                {
                    case 'H': /* Home. */
                        _d->edit_move_home();
                        break;
                    case 'F': /* End. */
                        _d->edit_move_end();
                        break;
                }
            }
            break;

        case (char)tim::key::ctrl_u: /* Удалить всю строку. */
            _d->_line.clear();
            _d->_pos = 0;
            _d->refresh_line();
            break;

        case (char)tim::key::ctrl_k: /* Удалить от текущей позиции до конца строки. */
            _d->_line.erase(_d->_line.cbegin() + _d->_pos, _d->_line.cend());
            _d->refresh_line();
            break;

        case (char)tim::key::ctrl_a: /* Перейти в начало строки. */
            _d->edit_move_home();
            break;

        case (char)tim::key::ctrl_e: /* Перейти в конец строки. */
            _d->edit_move_end();
            break;

        case (char)tim::key::ctrl_l: /* Очистить экран. */
            _d->_terminal->clear();
            _d->refresh_line();
            break;

        case (char)tim::key::ctrl_w: /* Удалить предыдущее слово. */
            _d->edit_delete_prev_word();
            break;

        default:
            _d->edit_insert(c);
            break;
    }

    return status::in_progress;
}

/** Очищает буфер редактирования (как Ctrl+U). */
void tim::line_edit::clear()
{
    _d->_line.clear();
    _d->_in_completion = true;
    _d->_mask_mode = false;
}

/**
 * Прячет приглашение и текущий ввод (стирает с экрана), чтобы вывести
 * приходящие асинхронно данные.
 *
 * \sa show() new_line()
 */

void tim::line_edit::hide()
{
    if (_d->_ml_mode)
        _d->refresh_multi_line(tim::p::line_edit::refresh_flag::clean);
    else
        _d->refresh_single_line(tim::p::line_edit::refresh_flag::clean);
}

/**
 * Возвращает приглашение и буфер на экран после hide().
 *
 * \sa hide() new_line()
 */

void tim::line_edit::show()
{
    if (_d->_in_completion)
        _d->refresh_line_with_completion(nullptr, tim::p::line_edit::refresh_flag::write);
    else
        _d->refresh_line_with_flags(tim::p::line_edit::refresh_flag::write);
}

/**
 * Включает/выключает многострочный режим редактирования (по умолчанию
 * выключен).
 *
 * \param enable true — многострочный, false — однострочный.
 */
void tim::line_edit::set_multi_line(bool enable)
{
    _d->_ml_mode = enable;
}

/**
 * Включает/выключает "маску ввода". Когда она включена, вместо
 * вводимых пользователем символов терминал отображает соответствующее
 * количество звёздочек ("****"). Используется для паролей и прочих
 * секретов, которые не следует показывать.
 *
 * \param enable true — маскировать.
 */
void tim::line_edit::set_mask_mode(bool enable)
{
    _d->_mask_mode = enable;
}

/**
 * Сохраняет историю на диск (по одной строке на запись).
 *
 * \param path Путь к файлу истории.
 * \return \c true при успехе и \c false в противном случае.
 */
bool tim::line_edit::history_save(const std::filesystem::path &path) const
{
    assert(!path.empty() && "History file path must not be empty.");

    const mode_t old_umask = umask(S_IXUSR | S_IRWXG | S_IRWXO);

    std::FILE *fp = std::fopen(path.string().c_str(), "w");
    if (!fp)
    {
        TIM_TRACE(error,
                  TIM_TR("Failed to open History file '%s' for writing: %s"_en,
                         "Ошибка при открытии файла истории '%s' на запись: %s"_ru),
                  path.string().c_str(), std::strerror(errno));
        return false;
    }
    umask(old_umask);
    chmod(path.string().c_str(), S_IRUSR | S_IWUSR);
    for (const std::wstring &s: _d->_history)
        if (!s.empty()
                && std::fprintf(fp, "%s\n", tim::from_wstring(s).c_str()) < 0)
        {
            TIM_TRACE(error,
                      TIM_TR("Failed to write to History file '%s': %s"_en,
                             "Ошибка записи в файл истории '%s': %s"_ru),
                      path.string().c_str(), std::strerror(errno));
            break;
        }
    std::fclose(fp);

    return true;
}

/**
 * Загружает историю с диска. Дубликаты и пустые строки
 * отфильтровываются.
 *
 * \param path Путь к файлу истории.
 * \return \c true при успехе и \c false в противном случае.
 */
bool tim::line_edit::history_load(const std::filesystem::path &path)
{
    assert(!path.empty() && "History file path must not be empty.");

    if (!std::filesystem::exists(path))
        return false;

    std::FILE *fp = std::fopen(path.string().c_str(), "r");
    if (!fp)
    {
        TIM_TRACE(error,
                  TIM_TR("Failed to open History file '%s' for reading: %s"_en,
                         "Ошибка при открытии файла истории '%s' на чтение: %s"_ru),
                  path.string().c_str(), std::strerror(errno));
        return false;
    }

    std::vector<char> buf(_d->MAX_LINE_SIZE);

    while (std::fgets(&buf[0], buf.size(), fp))
    {
        char *p = std::strchr(&buf[0], '\r');
        if (!p)
            p = std::strchr(&buf[0], '\n');
        if (p)
            *p = '\0';
        _d->history_add(tim::to_wstring(&buf[0]));
    }
    std::fclose(fp);

    return true;
}

/**
 * Регистрирует функцию автодополнения. Вызывается при Tab.
 *
 * \param fn Обработчик-автодополнялка.
 */
void tim::line_edit::set_completer(tim::line_edit::completer_fn fn)
{
    _d->_completer = fn;
}

/**
 * Регистрирует функцию подсказки. Если задана, её результат
 * отображается справа от приглашения перед редактируемой строкой.
 *
 * \param fn Обработчик подсказки.
 */
void tim::line_edit::set_hinter(tim::line_edit::hinter_fn fn)
{
    _d->_hinter = fn;
}

// Private

/**
 * Добавляет строку в историю.
 */
void tim::p::line_edit::history_add(const std::wstring &line)
{
    if (!line.empty())
        _history.emplace_back(line);
}

/** Звуковой сигнал; используется при автодополнении, когда нечего
  * дополнять или когда все варианты уже показаны. Этот метод ничего
  * не делает.
  */
void tim::p::line_edit::beep()
{
}

/* Вызывается из complete_line() и show() для отрисовки текущей
 * редактируемой строки с предлагаемым автодополнением. Если текущая
 * таблица автодополнений уже доступна, она передаётся вторым
 * аргументом; иначе функция получит её через обработчик.
 *
 * Флаги — те же, что и у refresh_line*(). */
void tim::p::line_edit::refresh_line_with_completion(const tim::line_edit::completions *c, refresh_flags flags)
{
    assert(_completer);

    /* Получаем автодополнения, если вызывающий их не предоставил. */
    const tim::line_edit::completions *lc =
            !c
                    || c->empty()
                ? new tim::line_edit::completions(_completer(tim::from_wstring(_line)))
                : c;

    /* Показываем редактируемую строку с автодополнением, если есть, иначе просто перерисовываем. */
    if (_completion_idx < lc->size())
    {
        const std::size_t p = _pos;
        const std::wstring l = _line;
        _pos = lc->at(_completion_idx).size();
        _line = tim::to_wstring(lc->at(_completion_idx));
        refresh_line_with_flags(flags);
        _pos = p;
        _line = l;
    }
    else
        refresh_line_with_flags(flags);

    if (c != lc)
        delete lc;
}

/* Вспомогательная функция для edit(), вызывается, когда пользователь
 * нажимает клавишу [Tab] для дополнения текущего ввода.
 *
 * Если функция возвращает ненулевое значение, вызывающий должен
 * обрабатывать возвращённое значение как байт, прочитанный со
 * стандартного ввода, и обрабатывать его как обычно: то есть функция
 * может вернуть байт, прочитанный с терминала, но не обработанный.
 * Иначе, если возвращён ноль, ввод был поглощён complete_line() для
 * навигации по возможным дополнениям, и вызывающий должен прочитать
 * следующие символы из входного потока. */
std::int32_t tim::p::line_edit::complete_line(std::int32_t key_pressed)
{
    assert(_completer);

    char c = key_pressed;

    tim::line_edit::completions lc = _completer(tim::from_wstring(_line));
    if (lc.empty())
    {
        beep();
        _in_completion = false;
        c = 0; // Никогда не разрешать табы.
    }
    else
    {
        switch (c)
        {
            case (char)tim::key::tab:
                if (!_in_completion)
                {
                    _in_completion = true;
                    _completion_idx = 0;
                }
                else
                {
                    _completion_idx = (_completion_idx + 1) % (lc.size() + 1);
                    if (_completion_idx == lc.size())
                        beep();
                }
                c = 0;
                break;

            case (char)tim::key::esc:
                /* Снова показать исходный буфер. */
                if (_completion_idx < lc.size())
                    refresh_line();
                _in_completion = false;
                c = 0;
                break;

            default:
                /* Обновить буфер и вернуться. */
                if (_completion_idx < lc.size())
                {
                    _line = tim::to_wstring(lc.at(_completion_idx));
                    _pos = _line.size();
                }
                _in_completion = false;
                break;
        }

        /* Показать автодополнение или исходный буфер. */
        if (_in_completion
                && _completion_idx < lc.size())
            refresh_line_with_completion(&lc, refresh_flag::all);
        else
            refresh_line();
    }

    return c; /* Вернуть последний прочитанный символ. */
}

/** Помощник refresh_single_line() и refresh_multi_line() для
  * отображения подсказок справа от приглашения.
  */
void tim::p::line_edit::refresh_show_hints(std::wstring &s)
{
    if (!_hinter)
        return;

    _cols = _terminal->cols();

    if (_plen + _line.size() < _cols)
    {
        int color = -1, bold = 0;
        const std::wstring hint = tim::to_wstring(_hinter(tim::from_wstring(_line), color, bold));
        if (!hint.empty())
        {
            if (bold == 1
                    && color == -1)
                color = 37;
            if (color != -1
                    || bold != 0)
                s += tim::to_wstring(tim::sprintf("\033[%d;%d;49m", bold, color));
            s += hint;
            if (color != -1
                    || bold != 0)
                s += L"\033[0m";
        }
    }
}

/** Низкоуровневая перерисовка строки в однострочном режиме.
  *
  * Перезаписывает текущую редактируемую строку в соответствии с
  * содержимым буфера, позицией курсора и количеством колонок терминала.
  *
  * Функция может только удалить старое приглашение, только записать
  * его, либо сделать и то и другое.
  */
void tim::p::line_edit::refresh_single_line(refresh_flags flags)
{
    _cols = _terminal->cols();

    const wchar_t *buf = _line.c_str();
    std::size_t len = _line.size();
    std::size_t pos = _pos;

    while (_plen + pos >= _cols)
    {
        ++buf;
        --len;
        --pos;
    }

    while (_plen + len > _cols)
        --len;

    /* Курсор к левому краю. */
    std::wstring ws(L"\r");

    if (flags.test(refresh_flag::write))
    {
        /* Записать приглашение и текущее содержимое буфера. */
        ws += _prompt;
        if (_mask_mode)
            ws += std::wstring(_line.size(), L'*');
        else
            ws += std::wstring_view(buf, len);
        /* Показать подсказки, если есть. */
        refresh_show_hints(ws);
    }

    /* Стереть до правого края. */
    ws += L"\x1b[0K";

    if (flags.test(refresh_flag::write))
    {
        /* Вернуть курсор в исходную позицию. */
        ws += tim::to_wstring(tim::sprintf("\r\x1b[%dC", (int)(pos + _plen)));
    }

    const std::string s = tim::from_wstring(ws);
    if (!_terminal->protocol()->write(s.c_str(), s.size()))
    {
        /* От ошибки записи восстановиться невозможно. */
    }
}

/* Низкоуровневая перерисовка строки в многострочном режиме.
 *
 * Перезаписывает текущую редактируемую строку в соответствии с
 * содержимым буфера, позицией курсора и количеством колонок терминала.
 *
 * Функция может только удалить старое приглашение, только записать его,
 * либо сделать и то и другое.
 */
void tim::p::line_edit::refresh_multi_line(refresh_flags flags)
{
    _cols = _terminal->cols();

    int rows = static_cast<int>((_plen + _line.size() + _cols - 1) / _cols); /* Количество строк, занятых текущим буфером. */
    int rpos = static_cast<int>((_plen + _old_pos + _cols) / _cols); /* Относительная строка курсора. */
    int rpos2; /* rpos после перерисовки. */
    int col; /* Позиция колонки, нумерация с нуля. */
    int old_rows = static_cast<int>(_old_rows);
    int j;

    _old_rows = rows;

    /* Шаг 1: очистить все ранее использованные строки. Для этого
     * сначала спускаемся на последнюю строку. */

    std::wstring ws;

    if (flags.test(refresh_flag::clean))
    {
        if (old_rows - rpos > 0)
            ws += tim::to_wstring(tim::sprintf("\x1b[%dB", old_rows - rpos));

        /* Теперь для каждой строки очищаем её и поднимаемся вверх. */
        for (j = 0; j < old_rows - 1; ++j)
            ws += L"\r\x1b[0K\x1b[1A";
    }

    if (flags.test(refresh_flag::all))
    {
        /* Очистить верхнюю строку. */
        ws += L"\r\x1b[0K";
    }

    if (flags.test(refresh_flag::write))
    {
        /* Записать приглашение и текущее содержимое буфера. */
        ws += _prompt;
        if (_mask_mode)
            ws += std::wstring(_line.size(), L'*');
        else
            ws += _line;

        /* Показать подсказки, если есть. */
        refresh_show_hints(ws);

        /* Если приглашение оказалось в самом конце экрана — нужно
         * испустить перевод строки и переместить приглашение в первую
         * колонку. */
        if (_pos
                && _pos == _line.size()
                && (_pos + _plen) % _cols == 0)
        {
            ws += L"\n\r";
            ++rows;
            if (rows > static_cast<int>(_old_rows))
                _old_rows = rows;
        }

        /* Переместить курсор в нужную позицию. */
        rpos2 = static_cast<int>((_plen + _pos + _cols) / _cols); /* Текущая относительная строка курсора. */

        /* Подниматься вверх, пока не достигнем ожидаемой позиции. */
        if (rows - rpos2 > 0)
            ws += tim::to_wstring(tim::sprintf("\x1b[%dA", rows - rpos2));

        /* Установить колонку. */
        col = (_plen + (int)_pos) % (int)_cols;
        if (col)
            ws += tim::to_wstring(tim::sprintf("\r\x1b[%dC", col));
        else
            ws += L'\r';
    }

    _old_pos = _pos;

    const std::string s = tim::from_wstring(ws);
    if (!_terminal->protocol()->write(s.c_str(), s.size()))
    {
        /* От ошибки записи восстановиться невозможно. */
    }
}

/** Вызывает одну из двух низкоуровневых функций — refresh_single_line()
  * или refresh_multi_line() — в зависимости от выбранного режима.
  */
void tim::p::line_edit::refresh_line_with_flags(refresh_flags flags)
{
    if (_ml_mode)
        refresh_multi_line(flags);
    else
        refresh_single_line(flags);
}

/** Утилитарная функция, чтобы не указывать refresh_flag::all каждый раз.
  */
void tim::p::line_edit::refresh_line()
{
    refresh_line_with_flags(refresh_flag::all);
}

/** Вставка символа \a c в текущую позицию курсора.
 *
 * При ошибке записи в терминал возвращается \c false, иначе \c true.
 */
bool tim::p::line_edit::edit_insert(std::int32_t c)
{
    _cols = _terminal->cols();

    _line.insert(_pos, 1, c);
    ++_pos;
    if (_pos == _line.size()
            && !_ml_mode
            && _plen + _line.size() < _cols
            && !_hinter)
    {
        /* Избегаем полной перерисовки строки в тривиальном случае. */
        const std::int32_t d = _mask_mode
                            ? '*'
                            : c;
        std::string s(utf8codepointsize(d), 0);
        utf8catcodepoint((utf8_int8_t *)(&s[0]), d, s.size());
        if (!_terminal->protocol()->write(s.c_str(), s.size()))
            return false;
    }
    else
        refresh_line();
    return true;
}

/** Сдвиг курсора влево.
 */
void tim::p::line_edit::edit_move_left()
{
    if (_pos > 0)
    {
        --_pos;
        refresh_line();
    }
}

/** Сдвиг курсора вправо.
 */
void tim::p::line_edit::edit_move_right()
{
    if (_pos != _line.size())
    {
        ++_pos;
        refresh_line();
    }
}

/** Перемещение курсора в начало строки.
 */
void tim::p::line_edit::edit_move_home()
{
    if (_pos != 0)
    {
        _pos = 0;
        refresh_line();
    }
}

/** Перемещение курсора в конец строки.
 */
void tim::p::line_edit::edit_move_end()
{
    if (_pos != _line.size())
    {
        _pos = _line.size();
        refresh_line();
    }
}

/** Подставляет в текущую редактируемую строку следующую или предыдущую
 * запись истории, в зависимости от \a dir.
 */
void tim::p::line_edit::edit_history_next(history_dir dir)
{
    if (_history.size() > 1)
    {
        /* Обновляем текущую запись истории, прежде чем перезаписать
         * её следующей. */
        _history[_history.size() - 1 - _history_idx] = _line;
        /* Показываем новую запись. */
        _history_idx +=
            dir == history_dir::prev
                ? 1
                : -1;
        if (_history_idx < 0)
        {
            _history_idx = 0;
            return;
        }
        if (_history_idx >= (int)_history.size())
        {
            _history_idx = (int)_history.size() - 1;
            return;
        }

        _line = _history[_history.size() - 1 - _history_idx];
        _pos = _line.size();
        refresh_line();
    }
}

/** Удаление символа справа от курсора без изменения позиции курсора.
  * Соответствует поведению клавиши [Delete] на клавиатуре.
  */
void tim::p::line_edit::edit_delete()
{
    if (!_line.empty()
            && _pos < _line.size())
    {
        _line.erase(_pos, 1);
        refresh_line();
    }
}

/** Реализация Backspace.
 */
void tim::p::line_edit::edit_backspace()
{
    if (!_line.empty()
            && _pos > 0)
    {
        _line.erase(_pos - 1, 1);
        --_pos;
        refresh_line();
    }
}

/** Удаляет предыдущее слово, оставляя курсор в начале текущего слова.
  */
void tim::p::line_edit::edit_delete_prev_word()
{
    const std::size_t old_pos = _pos;

    while (_pos > 0
                && std::isspace(_line[_pos - 1]))
        --_pos;
    while (_pos > 0
                && !std::isspace(_line[_pos - 1]))
        --_pos;

    _line.erase(_line.cbegin() + _pos, _line.cbegin() + old_pos);
    refresh_line();
}
