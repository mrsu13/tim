#pragma once

#include <filesystem>


namespace tim
{

class a_script_engine;
class line_edit;

namespace p
{

/**
 * Внутреннее состояние tim::vt_shell (PIMPL).
 */
struct vt_shell
{
    /** \return Текст welcome-баннера TIM (статическая инициализация при первом вызове). */
    static const std::string &welcome_banner();
    /** \return Текст bye-баннера TIM. */
    static const std::string &bye_banner();

    /** Скрипт-движок, на который шелл отправляет команды. */
    tim::a_script_engine *_engine = nullptr;
    /** Редактор ввода (line_edit). */
    std::unique_ptr<tim::line_edit> _ledit;
    /** Путь к файлу истории (загружается в конструкторе, сохраняется в дтр-е). */
    std::filesystem::path _history_path;
};

}

}
