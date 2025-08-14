#include "cliface.h"
#include "commandtree.h"


// Splits 'input' by the given separator character and returns vector of parts
std::vector<std::string> split(const std::string& input, char separator)
{
    std::vector<std::string> result;
    std::string::size_type start = 0;
    auto pos = input.find(separator);

    while (pos != std::string::npos) {
        result.push_back(input.substr(start, pos - start));
        start = pos + 1;
        pos = input.find(separator, start);
    }

    // Add the last segment
    auto final = input.substr(start);
    if (!final.empty())
        result.push_back(std::move(final));

    return result;
}


int main()
{
    ra::CommandTree tree;
    tree.insert({ "git", "add" }, [](){ std::cout << "Executing git add"; });
    tree.insert({ "git", "commit" }, [](){ std::cout << "Executing git commit"; });
    tree.insert({ "docker", "run" }, [](){ std::cout << "Executing docker run"; });
    tree.insert({ "docker", "build" }, [](){ std::cout << "Executing docker build"; });

    tree.insert({ "playlist", "add" }, [](){ std::cout << "!playlist add"; });
    tree.insert({ "playlist", "remove" }, [](){ std::cout << "!playlist remove"; });
    tree.insert({ "playlist", "insert" }, [](){ std::cout << "!playlist input"; });

    //
    CliAutoComplete cli;
    
    cli.onKeyEnter = [&tree](std::string& input) {
        if (auto node = tree.find(split(input, ' '))) {
            std::cout << std::endl << std::flush;
            node->execute();
        }
    };

    cli.onNeedSuggestions = [&tree](std::string& input) -> std::vector<std::string> {
        return tree.complete(split(input, ' '), 0);
    };

    cli.runloop();

    return 0;
}
