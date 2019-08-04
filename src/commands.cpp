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
#include <set>
#include <string>
#include <vector>

namespace uuid {

namespace console {

flash_string_vector Commands::no_arguments;

Commands::argument_completion_function Commands::no_argument_completion = [] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) -> const std::set<std::string> {
	return std::set<std::string>{};
};

void Commands::add_command(unsigned int context, unsigned int flags,
		const flash_string_vector &name, const flash_string_vector &arguments,
		command_function function, argument_completion_function arg_function) {
	commands_.emplace_back(std::make_shared<Command>(context, flags, name, arguments, function, arg_function));
}

Commands::Execution Commands::execute_command(Shell &shell __attribute__((unused)), unsigned int context, unsigned int flags, const std::list<std::string> &command_line) {
	auto commands = find_command(context, flags, command_line, false);
	Execution result;

	result.error = nullptr;

	if (commands.size() == 0) {
		result.error = F("Command not found");
	} else if (commands.size() == 1) {
		auto command = commands.front();
		std::vector<std::string> arguments{std::next(command_line.cbegin(), command->name_.size()), command_line.cend()};

		// FIXME modify find_command() to do this
		std::list<std::string> command_line2{command_line.cbegin(), std::next(command_line.cbegin(), command->name_.size())};
		auto commands2 = find_command(context, flags, command_line2, true);

		if (commands2.size() > 1 && !arguments.empty()) {
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
	auto commands = find_command(context, flags, command_line, false);
	Completion result;

	if (commands.size() == 1) {
		auto &matching_command = commands.front();

		for (auto &name : matching_command->name_) {
			result.replacement.emplace_back(read_flash_string(name));
		}

		// If there are no arguments then search again for more longer commands
		bool more_commands = false;

		if (command_line.size() <= result.replacement.size()) {
			std::list<std::string> command_line2;

			for (auto &name : result.replacement) {
				command_line2.emplace_back(name.c_str());
			}

			auto commands2 = find_command(context, flags, command_line2, true);

			if (commands2.size() == 1) {
				if (commands2.front()->name_.size() > command_line2.size()) {
					more_commands = true;
				}
			} else if (commands2.size() > 1) {
				more_commands = true;
			}
		}

		if (more_commands) {
			result.replacement.emplace_back("");
		} else if (command_line.size() > result.replacement.size()) {
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
				potential_arguments.clear();
			} else {
				// Put the last argument back
				result.replacement.emplace_back(last_argument);
			}

			for (auto potential_argument : potential_arguments) {
				std::list<std::string> help;

				help.emplace_back(potential_argument);

				// FIXME "help" is no longer a full command line
				if (help.size() < matching_command->name_.size() + matching_command->arguments_.size()) {
					for (auto it = std::next(matching_command->arguments_.cbegin(), help.size() - matching_command->name_.size()); it != matching_command->arguments_.cend(); it++) {
						help.emplace_back(read_flash_string(*it));
					}
				}

				result.help.emplace_back(help);
			}
		} else if (command_line.size() < commands.front()->name_.size() + commands.front()->arguments_.size()) {
			result.replacement.emplace_back("");
		}

	} else if (commands.size() > 1) {
		for (auto &command : commands) {
			std::list<std::string> help;

			auto line_it = command_line.cbegin();

			for (auto flash_name : command->name_) {
				std::string name = read_flash_string(flash_name);

				if (line_it != command_line.cend()) {
					if (name == *line_it++) {
						continue;
					}
				}

				help.emplace_back(name);
			}

			for (auto argument : command->arguments_) {
				if (line_it != command_line.cend()) {
					line_it++;
					continue;
				}

				help.emplace_back(read_flash_string(argument));
			}

			result.help.emplace_back(help);
		}
	}

	return result;
}

std::list<std::shared_ptr<const Commands::Command>> Commands::find_command(unsigned int context, unsigned int flags, const std::list<std::string> &command_line, bool partial) {
	std::list<std::shared_ptr<const Command>> commands;
	size_t longest = 0;

	for (auto& command : commands_) {
		bool match = true;

		if ((command->flags_ & flags) != command->flags_) {
			continue;
		}

		if (context != command->context_) {
			continue;
		}

		if (partial) {
			auto name_it = command->name_.cbegin();
			auto line_it = command_line.cbegin();

			for (; name_it != command->name_.cend() && line_it != command_line.cend(); name_it++, line_it++) {
				std::string name = read_flash_string(*name_it);
				size_t found = name.rfind(*line_it, 0);

				if (found == std::string::npos) {
					match = false;
					break;
				} else if (*line_it != name) {
					if (std::next(line_it) != command_line.cend()) {
						// If there's more in the command line then this can't match
						match = false;
					}

					// Don't check the rest of the command if this is only a partial match
					break;
				}
			}
		} else {
			if (command->name_.size() > command_line.size()) {
				continue;
			}

			auto name_it = command->name_.cbegin();
			auto line_it = command_line.cbegin();

			for (; name_it != command->name_.cend() && line_it != command_line.cend(); name_it++, line_it++) {
				if (strcmp_P(line_it->c_str(), (PGM_P)*name_it)) {
					match = false;
					break;
				}
			}
		}

		if (match && command->name_.size() >= longest) {
			commands.emplace_back(command);
			longest = std::max(longest, command->name_.size());
		}
	}

	// Only keep the longest matches
	for (auto command = commands.begin(); command != commands.end(); ) {
		if (command->get()->name_.size() < longest) {
			command = commands.erase(command);
		} else {
			command++;
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
