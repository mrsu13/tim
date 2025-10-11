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

    enum class status
    {
        Finished, ///< Editing is finished. Now you can use the edited line.
        Continue, ///< Editing is in progress. You should call get_line() again.
        Exit, ///< User pressed `[Ctrl+D]`.
        Break, ///< User pressed `[Ctrl+C]`.
        Error ///< Reading from input stream or writing to output stream failed.
    };

    status get_line(const char *data, std::size_t size);

    void clear();

    void hide();
    void show();

    void set_multi_line(bool enable);
    void set_mask_mode(bool enable);

    bool history_save(const std::filesystem::path &path) const;
    bool history_load(const std::filesystem::path &path);

    using completions = std::vector<std::string>;
    using completer_fn = std::function<completions (const std::string &prefix)>;
    void set_completer(completer_fn fn);

    /**
     * If this callback is set, its result will be shown to the right
     * from the prompt before the line to be edited.
     */
    using hinter_fn = std::function<std::string (const std::string &line, int &color, int &bold)>;
    void set_hinter(hinter_fn fn);

private:

    std::unique_ptr<tim::p::line_edit> _d;
};

}
