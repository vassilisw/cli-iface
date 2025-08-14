/*
    This is ai generated content
*/

#pragma once

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <optional>


namespace ra
{
    class CommandTree
    {
    public:
        using Params = std::vector<std::string>;
        using Func = std::function<void(const Params& params)>;

        // Insert a command path with an associated function at the leaf
        void insert(const std::vector<std::string>& path, Func func, size_t index = 0)
        {
            if (index == path.size()) {
                action_ = std::move(func); // store function at leaf node
                //TODO update remainderPathAsParams // ref[asparamsonins]
                return;
            }
            children_[path[index]].insert(path, std::move(func), index + 1);
        }

        // Find the node corresponding to path (returns nullptr if not found)
        const CommandTree* find(const std::vector<std::string>& path, size_t index = 0)
        {
            if (index == path.size())
                return this;

            auto it = children_.find(path[index]);
            if (it == children_.end()) {
                if (remainderPathAsParams) {
                    actionParams_ = Params(path.cbegin() + index, path.cend());
                    return this;
                }
                else {
                    return nullptr;
                }
            }
            
            return it->second.find(path, index + 1);
        }

        // Execute the function stored at this node if any, return true if executed
        bool execute() const
        {
            if (action_) {
                (*action_)(actionParams_);
                return true;
            }

            return false;
        }

        // Get all keys at this node (for completion suggestion, etc.)
        std::vector<std::string> keys() const
        {
            std::vector<std::string> keys;
            for (const auto& p : children_)
                keys.push_back(p.first);

            return keys;
        }

        // Returns possible completions for the last part of the path, or all keys if path is empty or ends with empty prefix
        std::vector<std::string> complete(const std::vector<std::string>& path, size_t index = 0, const std::string& prefix = "") const
        {
            if (index == path.size()) {
                // At the node for the full input path; suggest children keys that start with prefix
                std::vector<std::string> completions;
                for (const auto& [key, _] : children_) {
                    if (key.find(prefix) == 0) // starts with prefix
                        completions.push_back(key);
                }
                return completions;
            }

            auto it = children_.find(path[index]);
            if (it == children_.end()) {
                // No exact match for this segment; if this is the last segment, treat as prefix search here
                if (index == path.size() - 1) {
                    std::vector<std::string> completions;
                    for (const auto& [key, _] : children_) {
                        if (key.find(path[index]) == 0)
                            completions.push_back(key);
                    }
                    return completions;
                }

                return {}; // no completions possible
            }

            // Recurse deeper
            return it->second.complete(path, index + 1, prefix);
        }

        // Returns true if after removal this node is empty and can be pruned by parent
        bool remove(const std::vector<std::string>& path, size_t index = 0)
        {
            if (index == path.size()) {
                // At leaf node: remove action if exists
                action_.reset();
            }
            else {
                auto it = children_.find(path[index]);
                if (it == children_.end())
                    return false; // nothing to remove

                bool emptyChild = it->second.remove(path, index + 1);
                if (emptyChild)
                    children_.erase(it);
            }

            // Return true if no action and no children, so parent can prune this node
            return !action_.has_value() && children_.empty();
        }

        //
        void test()
        {
            CommandTree tree;

            // Insert some commands with associated functions
            tree.insert({ "git", "add" }, [](const Params& params) { std::cout << "Executing git add\n"; });
            tree.insert({ "git", "commit" }, [](const Params& params) { std::cout << "Executing git commit\n"; });
            tree.insert({ "docker", "run" }, [](const Params& params) { std::cout << "Executing docker run\n"; });
            tree.insert({ "docker", "build" }, [](const Params& params) { std::cout << "Executing docker build\n"; });

            // Simulate user input
            std::vector<std::vector<std::string>> testPaths = {
                {"git", "add"},
                {"git", "commit"},
                {"docker", "push"}, // no such leaf
                {"docker", "run"},
                {"git"}
            };

            for (const auto& path : testPaths) {
                std::cout << "Input: ";
                for (const auto& p : path) std::cout << p << " ";
                std::cout << std::endl;

                auto node = tree.find(path);
                if (node && node->execute()) {
                    std::cout << "Command executed successfully\n";
                }
                else {
                    std::cout << "Command not found or no action assigned\n";

                    // If found partial node, print possible completions
                    if (node) {
                        std::cout << "Possible next commands: ";
                        for (auto& key : node->keys())
                            std::cout << key << " ";
                        std::cout << std::endl;
                    }
                }
                std::cout << "----\n";
            }


            //
            std::cout << "auto complete suggestions: \n";
            std::vector<std::string> input = { "git" };
            auto suggestions = tree.complete(input, 0, "ad");
            for (const auto& s : suggestions)
                std::cout << s << " ";
            std::cout << std::endl;

            //
            tree.remove({ "git", "add" });
        }

        bool remainderPathAsParams = true;

        private:
            std::map<std::string, CommandTree> children_;
            std::optional<Func> action_; // execute if this node is a leaf
            Params actionParams_;
    };


} // ra namespace