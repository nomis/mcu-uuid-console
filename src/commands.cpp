/*
 * uuid-console - Microcontroller console shell
 * Copyright 2019  Simon Arlott
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a std::copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <uuid/console.h>

#include <Arduino.h>

#include <algorithm>
#include <list>
#include <memory>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace uuid {

namespace console {

void Commands::add_command(unsigned int context, unsigned int flags,
		const flash_string_vector &name, const flash_string_vector &arguments,
		command_function function, argument_completion_function arg_function) {
	commands_.emplace_back(std::make_shared<Command>(context, flags, name, arguments, function, arg_function));
}

Commands::Execution Commands::execute_command(Shell &shell __attribute__((unused)), unsigned int context, unsigned int flags, const std::list<std::string> &command_line) {
	auto commands = find_command(context, flags, command_line);
	auto longest = commands.exact.crbegin();
	Execution result;

	result.error = nullptr;

	if (commands.exact.empty()) {
		result.error = F("Command not found");
	} else if (commands.exact.count(longest->first) == 1) {
		auto &command = longest->second;
		std::vector<std::string> arguments{std::next(command_line.cbegin(), command->name_.size()), command_line.cend()};

		while (!arguments.empty() && arguments.back().empty()) {
			arguments.pop_back();
		}

		if (commands.partial.upper_bound(longest->first) != commands.partial.end() && !arguments.empty()) {
			result.error = F("Command not found");
		} else if (arguments.size() < command->minimum_arguments()) {
			result.error = F("Not enough arguments for command");
		} else if (arguments.size() > command->maximum_arguments()) {
			result.error = F("Too many arguments for command");
		} else {
			command->function_(shell, arguments);
		}
	} else {
		result.error = F("Fatal error (multiple commands found)");
	}

	return result;
}

Commands::Completion Commands::complete_command(Shell &shell, unsigned int context, unsigned int flags, const std::list<std::string> &command_line) {
	auto commands = find_command(context, flags, command_line);
	Completion result;

	auto shortest_match = commands.partial.begin();
	size_t shortest_count;
	bool exact;
	if (shortest_match != commands.partial.end()) {
		shortest_count = commands.partial.count(shortest_match->first);
		exact = false;
	} else {
		shortest_match = commands.exact.begin();
		if (shortest_match != commands.exact.end()) {
			shortest_count = commands.exact.count(shortest_match->first);
			exact = true;
		} else {
			return result;
		}
	}

	bool longer_matches = commands.exact.upper_bound(shortest_match->first) != commands.exact.end()
			|| commands.partial.upper_bound(shortest_match->first) != commands.partial.end();
	bool add_space = false;

	if (commands.exact.empty() && shortest_count > 1) {
		// There are no exact matches and there are multiple commands with the same shortest partial match length
		size_t longest_common = 0;
		auto &shortest_first = commands.partial.find(shortest_match->first)->second->name_;

		// Check if any of the commands have a common substring
		for (size_t length = 1; shortest_match->first; length++) {
			bool all_match = true;

			for (auto command_it = std::next(commands.partial.find(shortest_match->first)); command_it != commands.partial.end(); command_it++) {
				if (read_flash_string(*std::next(shortest_first.begin(), length - 1)) != read_flash_string(*std::next(command_it->second->name_.begin(), length - 1))) {
					all_match = false;
					break;
				}
			}

			if (all_match) {
				longest_common = length;
			} else {
				break;
			}
		}

		if (longest_common > 0 && command_line.size() <= longest_common) {
			// Is this now an exact match for the command line?
			if (command_line.size() == longest_common) {
				exact = true;

				auto name_it = shortest_first.cbegin();
				auto line_it = command_line.cbegin();

				for (size_t i = 0; i < longest_common && name_it != shortest_first.cend() && line_it != command_line.cend(); name_it++, line_it++) {
					if (*line_it != read_flash_string(*name_it)) {
						exact = false;
					}
				}
			}

			// Create a temporary command that represents the longest common substring
			auto &target = exact ? commands.exact : commands.partial;

			target.emplace(longest_common, std::make_shared<Command>(0, 0,
					std::vector<const __FlashStringHelper *>{shortest_first.begin(), std::next(shortest_first.begin(), longest_common)}, no_arguments(),
					[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {}, no_argument_completion()));
			shortest_count = 1;
			shortest_match = target.find(longest_common);
			longer_matches = true;
			add_space = true;
		}
	}

	if (shortest_count == 1) {
		// Construct a replacement string for a single matching command
		auto &matching_command = shortest_match->second;

		for (auto &name : matching_command->name_) {
			result.replacement.emplace_back(read_flash_string(name));
		}

		if (command_line.size() > result.replacement.size()) {
			// Try to auto-complete arguments
			std::vector<std::string> arguments{std::next(command_line.cbegin(), result.replacement.size()), command_line.cend()};
			result.replacement.insert(result.replacement.end(), arguments.cbegin(), arguments.cend());

			// Remove the last argument so that it can be auto-completed
			std::string last_argument = result.replacement.back();
			result.replacement.pop_back();
			if (!arguments.empty()) {
				arguments.pop_back();
			}

			auto potential_arguments = matching_command->arg_function_(shell, arguments);

			// Remove arguments that can't match
			if (!last_argument.empty()) {
				for (auto it = potential_arguments.begin(); it != potential_arguments.end(); ) {
					if (last_argument.rfind(*it, 0) == std::string::npos) {
						it = potential_arguments.erase(it);
					} else {
						it++;
					}
				}
			}

			if (potential_arguments.size() == 1 && !last_argument.empty()) {
				// Auto-complete if there's something present in the last argument
				result.replacement.emplace_back(*potential_arguments.begin());

				if (result.replacement.size() < matching_command->name_.size() + matching_command->arguments_.size()) {
					result.replacement.emplace_back("");
				}

				potential_arguments.clear();
			} else {
				// Put the last argument back
				result.replacement.emplace_back(last_argument);
			}

			for (auto potential_argument : potential_arguments) {
				std::list<std::string> help;

				help.emplace_back(potential_argument);

				auto current_arguments = arguments.size() + 1;
				if (current_arguments < matching_command->arguments_.size()) {
					for (auto it = std::next(matching_command->arguments_.cbegin(), current_arguments); it != matching_command->arguments_.cend(); it++) {
						help.emplace_back(read_flash_string(*it));
					}
				}

				result.help.emplace_back(help);
			}
		} else if (command_line.size() < matching_command->name_.size() + matching_command->arguments_.size()) {
			// Add a space because the are more arguments for this command
			add_space = true;
		} else if (exact && longer_matches) {
			// Add a space because the are sub-commands for this command that has just matched exactly
			add_space = true;
		}
	} else {
		// Provide help for all of the potential commands
		for (auto command_it = commands.partial.begin(); command_it != commands.partial.end(); command_it++) {
			std::list<std::string> help;

			auto line_it = command_line.cbegin();

			for (auto flash_name : command_it->second->name_) {
				std::string name = read_flash_string(flash_name);

				if (line_it != command_line.cend()) {
					if (name == *line_it++) {
						continue;
					}
				}

				help.emplace_back(name);
			}

			for (auto argument : command_it->second->arguments_) {
				if (line_it != command_line.cend()) {
					line_it++;
					continue;
				}

				help.emplace_back(read_flash_string(argument));
			}

			result.help.emplace_back(help);
		}

		if (!commands.exact.empty()) {
			auto longest = commands.exact.crbegin();

			if (commands.exact.count(longest->first) == 1) {
				for (auto &name : longest->second->name_) {
					result.replacement.emplace_back(read_flash_string(name));
				}

				// Add a space because there are sub-commands for a command that has matched exactly
				add_space = true;
			}
		}
	}

	if (add_space) {
		if (command_line.size() <= result.replacement.size()) {
			result.replacement.emplace_back("");
		}
	}

	// Don't try to shorten the command lines or offer an identical replacement
	if (command_line.size() > result.replacement.size() || result.replacement == command_line) {
		result.replacement.clear();
	}

	return result;
}

Commands::Match Commands::find_command(unsigned int context, unsigned int flags, const std::list<std::string> &command_line) {
	Match commands;

	for (auto& command : commands_) {
		bool match = true;
		bool exact = true;

		if ((command->flags_ & flags) != command->flags_) {
			continue;
		}

		if (context != command->context_) {
			continue;
		}

		auto name_it = command->name_.cbegin();
		auto line_it = command_line.cbegin();

		for (; name_it != command->name_.cend() && line_it != command_line.cend(); name_it++, line_it++) {
			std::string name = read_flash_string(*name_it);
			size_t found = name.rfind(*line_it, 0);

			if (found == std::string::npos) {
				match = false;
				break;
			} else if (line_it->length() != name.length()) {
				for (auto line_check_it = std::next(line_it); line_check_it != command_line.cend(); line_check_it++) {
					if (!line_check_it->empty()) {
						// If there's more in the command line then this can't match
						match = false;
					}
				}

				// Don't check the rest of the command if this is only a partial match
				break;
			}
		}

		if (name_it != command->name_.cend()) {
			exact = false;
		}

		if (match) {
			if (exact) {
				commands.exact.emplace(command->name_.size(), command);
			} else {
				commands.partial.emplace(command->name_.size(), command);
			}
		}
	}

	return commands;
}

Commands::Command::Command(unsigned int context, unsigned int flags,
		const flash_string_vector name, const flash_string_vector arguments,
		command_function function, argument_completion_function arg_function)
		: context_(context), flags_(flags),
		  name_(name), arguments_(arguments),
		  function_(function), arg_function_(arg_function) {

}

Commands::Command::~Command() {

}

size_t Commands::Command::minimum_arguments() const {
	return std::count_if(arguments_.cbegin(), arguments_.cend(), [] (const __FlashStringHelper *argument) { return pgm_read_byte(argument) == '<'; });
}
size_t Commands::Command::maximum_arguments() const {
	return arguments_.size();
};

} // namespace console

} // namespace uuid
