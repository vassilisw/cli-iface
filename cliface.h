#pragma once

#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <functional>
#include <optional>


#define KEY_TAB    (0x09)  // ASCII for Tab character '\t'
#define KEY_BK     (0x7f)  // ASCII DEL (usually Backspace)
#define KEY_ENTER  (0x0a)  // ASCII Line Feed '\n' (Enter)
#define KEY_ESC    (0x1b)  // ASCII Escape character
#define KEY_UP     (0x41)  // 'A', often used in escape seq: ESC [ A = up arrow
#define KEY_DOWN   (0x42)  // 'B', ESC [ B = down arrow
#define KEY_RIGHT  (0x43)  // 'C', ESC [ C = right arrow
#define KEY_LEFT   (0x44)  // 'D', ESC [ D = left arrow
#define CLR_LINE   "\r\e[K" // ANSI escape: carriage return + clear line from cursor


// Restore terminal settings on exit
struct TermiosSaver
{
    TermiosSaver() {
        tcgetattr(STDIN_FILENO, &oldt);
    }

    // Disable canonical mode and echo
    void disableCanonEchoMode() {
        termios newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    }

    ~TermiosSaver() {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }

    termios oldt;
};


class CliIface
{
public:
    CliIface(bool initTerminal = true, bool reprintPromptOnTab = false)
        : reprintPromptOnTab_(reprintPromptOnTab)
    {
        if (initTerminal) {
            termSaver_.emplace();
            termSaver_->disableCanonEchoMode();
        }
    }

    virtual ~CliIface() = default;

    std::string prompt()
    {
        return Prompt;
    }
    
    void runloop()
    {
        std::cout << Prompt << std::flush;

        bool dobreak = false;
        while (!dobreak) {
            if (ssize_t n = read(STDIN_FILENO, &char_, 1); n <= 0)
                break;

            switch (char_) {
                case KEY_ESC: {  // Arrow keys
                        char seq[2];
                        if (read(STDIN_FILENO, &seq[0], 1) == 0) continue;
                        if (read(STDIN_FILENO, &seq[1], 1) == 0) continue;
                        if (seq[0] == '[' && onKeyArrow)
                            onKeyArrow(buf_, seq[1]);
                    }
                    break;

                case '\4':  // Ctrl+D in raw mode
                    if (onKeyCtrlD)
                        onKeyCtrlD(buf_);
                    std::cout << std::endl;
                    dobreak = true;
                    break;

                case KEY_TAB:
                    if (onKeyTab)
                        onKeyTab(buf_);
                    if (reprintPromptOnTab_)
                        std::cout << std::endl << Prompt << buf_;
                    std::cout << std::flush;
                    break;

                case KEY_ENTER:
                case '\r':  // carriage return (Enter win)
                    if (onKeyEnter)
                        onKeyEnter(buf_);
                    buf_.clear();
                    std::cout << std::endl << Prompt << std::flush;
                    break;

                case 127:   // backspace (DEL)
                case 8:     // backspace (BS)
                    if (!buf_.empty()) {
                        buf_.pop_back();
                        std::cout << "\b \b" << std::flush;
                    }
                    break;

                default:
                    buf_ += char_;
                    std::cout << char_ << std::flush;
                    break;
            }
        }
    }

    std::function<void(std::string&)> onKeyCtrlD;
    std::function<void(std::string&)> onKeyTab;
    std::function<void(std::string&)> onKeyEnter;
    std::function<void(std::string&)> onKeyEsc;
    std::function<void(std::string&, char key)> onKeyArrow;

private:
    static constexpr auto Prompt = "> ";

    bool reprintPromptOnTab_; // reprint prompt + current input
    std::optional<TermiosSaver> termSaver_;
    std::string buf_;
    char char_;
};


class CliAutoComplete
    : public CliIface
{
public:
    CliAutoComplete()
    {
        onKeyTab = [this](std::string& input) {
            if (!onNeedSuggestions)
                return;

            const auto& suggestions = onNeedSuggestions(input);

            if (suggestions.size() == 1) {
                input = input.substr(0, input.rfind(' ') + 1);
                input += suggestions[0] + " ";
                std::cout << CLR_LINE << prompt() << input;
            }
            else if (suggestions.size() > 1) {
                std::cout << std::endl;
                for (const auto& s : suggestions)
                    std::cout << s << " ";
                std::cout << std::endl << prompt() << input;
            }
        };

        onKeyArrow = [this](std::string& input, char key) {
            size_t sz = history_.size();
            if (sz == 0)
                return;

            if (key == KEY_UP && historyPos_ > 0) --historyPos_;
            else if (key == KEY_DOWN && historyPos_ < sz) ++historyPos_;

            if (historyPos_ < sz)
                input = history_[historyPos_];
            else
                input.clear(); // past the end, empty input

            std::cout << CLR_LINE << prompt() << input << std::flush;
        };

        CliIface::onKeyEnter = [this](std::string& input) {
            if (onKeyEnter)
                onKeyEnter(input);

            if (!input.empty() && (history_.size() == 0 || input != history_.back()))
                history_.push_back(input);
            historyPos_ = history_.size();
        };
    }

    std::function<std::vector<std::string>(std::string& input)> onNeedSuggestions;
    std::function<void(std::string&)> onKeyEnter;

private:
    using CliIface::onKeyTab;   // hide it
    using CliIface::onKeyArrow; // hide it

    std::vector<std::string> history_;
    ssize_t historyPos_ = 0;

};



// ------------------------------------------------------------------
/*
int main()
{
    CliIface cli;
    cli.onKeyEnter_ = [](const std::string& input){
    };
    cli.runloop();
    return 0;
}
*/