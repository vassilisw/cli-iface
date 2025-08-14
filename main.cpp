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
    using Params = ra::CommandTree::Params;

    ra::CommandTree tree;
    tree.remainderPathAsParams = false; //? ref[asparamsonins]
    
    tree.insert({}, [](const Params& params){ std::cout << "Empty!!"; });

    tree.insert({ "git", "add" }, [](const Params& params){ std::cout << "Executing git add"; });
    tree.insert({ "git", "commit" }, [](const Params& params){ std::cout << "Executing git commit"; });
    tree.insert({ "docker", "run" }, [](const Params& params){ std::cout << "Executing docker run"; });
    tree.insert({ "docker", "build" }, [](const Params& params){ std::cout << "Executing docker build"; });

    tree.insert({ "playlist", "add" }, [](const Params& params){ std::cout << "!playlist add"; });
    tree.insert({ "playlist", "remove" }, [](const Params& params){ std::cout << "!playlist remove"; });

    tree.insert({ "playlist", "insert" }, [](const Params& params){
        std::cout << "!playlist insert with params: ";
        for (const auto& s : params)
            std::cout << s << " ";
    });

    //
    CliAutoComplete cli;
    
    cli.onKeyEnter = [&tree](std::string& input) {
        if (auto node = tree.find(split(input, ' '))) {
            std::cout << std::endl << std::flush;
            if (!node->execute())
                std::cout << "no action";
        }
    };

    cli.onNeedSuggestions = [&tree](std::string& input) -> std::vector<std::string> {
        return tree.complete(split(input, ' '), 0);
    };

    cli.runloop();

    return 0;
}
