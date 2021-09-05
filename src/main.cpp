#include <iostream>
#include <vector>
#include <string> 
#include <unordered_map>
#include <functional>
#include <Windows.h>
#include <errno.h>

#ifdef WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

constexpr int32_t ANY = -1;

struct CommandDetail;

enum class CommandErrorType {
	NumArgsError, MissingArgsError, ArgTypeError, CommandFailedError, MissingCommandError, None
};

struct CommandError {
	CommandError() = default;
	CommandError(const std::string& msg, CommandErrorType type) : error_msg(msg), type(type) { }

	CommandErrorType type = CommandErrorType::None;
	std::string error_msg = "Command was successful.";
};

std::vector<std::string> divided_args(const std::string& input);
std::string mash_args_together(const std::vector<std::string>& args);
std::string get_cd();

std::string get_input();
CommandError check_for_spaces(const std::string& str);
CommandError successful_procedure();
void through_error(const CommandError& error);

bool check_for_errors(const CommandError& error);
bool check_arg_count(CommandDetail& detail);
bool is_number(const std::string& s);

void command_loop();
void init_commands();
void init_errors();
void start_prompt();

CommandError command_echo(CommandDetail& detail);
CommandError command_exit(CommandDetail& detail);
CommandError command_sys(CommandDetail& detail);
CommandError command_history(CommandDetail& detail);
CommandError command_clear(CommandDetail& detail);
CommandError command_help(CommandDetail& detail);
CommandError command_cd(CommandDetail& detail);

using CommandFn = std::function<CommandError(CommandDetail& detail)>;
using Command = std::unordered_map<std::string, CommandDetail>;

struct CommandDetail {
	int32_t arg_num = 0;
	std::string command_name = "Name";
	std::string description = "Description";
	CommandFn fn;
	bool dynamic_args = false;

	std::vector<std::string> args;

	CommandDetail() = default;
	CommandDetail(int32_t arg_num, const std::string& command_name, const CommandFn& fn, const std::string& description) : 
		arg_num(arg_num), command_name(command_name), fn(fn), description(description) { }
};

static Command commands;
static std::unordered_map<CommandErrorType, std::string> error_type_fetcher;
static std::vector<std::string> cmd_history;
std::string current_cd;

int main(int argc, char** argv) {
	current_cd = get_cd();

	init_commands();
	init_errors();
	start_prompt();

	command_loop();

	return 0;
}

void command_loop() {
	bool running = true;

	while (running) {
		//Get user line input
		std::string input = get_input();
		if (check_for_errors(check_for_spaces(input)))
			continue;

		//Fetch and tokenize the input 
		auto args = divided_args(input);

		//Find requested command in hashmap
		Command::iterator it = commands.find(args[0]);

		//Execute that command
		if (it != commands.end()) {
			CommandDetail cmd = commands[args[0]];
			args.erase(args.begin());
			cmd.args = args;

			if (!check_arg_count(cmd)) {
				through_error(CommandError("Error: Incorrect number of arguments passed to command.", CommandErrorType::NumArgsError));
				continue;
			}
				
			check_for_errors(cmd.fn(cmd)); 

			cmd_history.push_back(cmd.command_name + ' ' + mash_args_together(cmd.args));
		}
		else
			std::cout <<"'" << args[0] << "'is not a recognized internal or external command." << std::endl;
	}
}

void init_commands() {
	CommandDetail echo(ANY, "echo", command_echo, "Print out arguments to the command prompt.");
	commands[echo.command_name] = echo;

	CommandDetail exit(0, "exit", command_exit, "Will close out of the command prompt.");
	commands[exit.command_name] = exit;

	CommandDetail sys(ANY, "sys", command_sys, "Will call arguments through the operating system.");
	commands[sys.command_name] = sys;

	CommandDetail his(0, "history", command_history, "Will print out history of commands.");
	commands[his.command_name] = his;

	CommandDetail clr(2, "clr", command_clear, "Will clear the given arguments (--history | --screen).");
	clr.dynamic_args = true;
	commands[clr.command_name] = clr;

	CommandDetail help(0, "help", command_help, "Will display list of commands and their descriptions");
	commands[help.command_name] = help;

	CommandDetail cd(1, "cd", command_cd, "Will change the directory relative to the one currently at.");
	cd.dynamic_args = true;
	commands[cd.command_name] = cd;
}

void init_errors() {
	error_type_fetcher[CommandErrorType::ArgTypeError] = "ArgTypeError";
	error_type_fetcher[CommandErrorType::CommandFailedError] = "CommandFailedError";
	error_type_fetcher[CommandErrorType::MissingArgsError] = "MissingArgsError";
	error_type_fetcher[CommandErrorType::MissingCommandError] = "MissingCommandError";
	error_type_fetcher[CommandErrorType::None] = "None";
	error_type_fetcher[CommandErrorType::NumArgsError] = "NumArgsError";
}

void start_prompt() {
	printf("************************************************\n");
	printf("**Command Developer PowerShell v1.0.0\n");
	printf("**Copyright (c) Strahinja\n");
	printf("**Entering shell at %s, %s.\n", __DATE__, __TIME__);
	printf("************************************************\n");
}

std::vector<std::string> divided_args(const std::string& input) {
	if (input == "") {
		std::cout << "Error: cannot pass in empty command.\n";
		return std::vector<std::string>();
	}
	else {
		std::string changable_input = input;
		changable_input.erase(changable_input.begin(), std::find_if(changable_input.begin(), changable_input.end(), std::bind1st(std::not_equal_to<char>(), ' ')));
		std::vector<std::string> args;
		std::string current_arg;
		for (auto& c : changable_input) {
			if (c == ' ') {
				if (check_for_spaces(current_arg).type == CommandErrorType::None)
					args.push_back(current_arg);
				current_arg = std::string();
			}
			else 
				current_arg += c;
		}

		if (check_for_spaces(current_arg).type == CommandErrorType::None) args.push_back(current_arg);	//Will push whatever is left over in token buffer if it is not just spaces
		return args;
	}
}

std::string mash_args_together(const std::vector<std::string>& args) {
	std::string out;
	for (auto& arg : args) 
		out += arg + ' ';

	return out;
}

std::string get_input() {
	std::cout << current_cd + "> ";

	std::string input;
	std::getline(std::cin, input);

	return input;
}

CommandError check_for_spaces(const std::string& str) {
	for (auto& c : str) {
		if (!isspace(c)) 
			return successful_procedure();
	}

	return CommandError("Error: input cannot just be spaces!", CommandErrorType::MissingCommandError);
}

CommandError successful_procedure() {
	return CommandError();
}

bool check_for_errors(const CommandError& error) {
	if (error.type != CommandErrorType::None) {
		std::cout << error.error_msg << std::endl;
		return true;
	}

	return false;
}

bool check_arg_count(CommandDetail& detail) {
	if (detail.arg_num >= 0) {
		if (detail.arg_num == detail.args.size() || (detail.dynamic_args && detail.args.size() <= detail.arg_num))
			return true;
		else
			return false;
	}
	return true;
}

void through_error(const CommandError& error) {
	std::cout << error.error_msg << " Error Type: " << error_type_fetcher[error.type] << std::endl;
}

bool is_number(const std::string& s) {
	return !s.empty() && std::find_if(s.begin(),
		s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

CommandError command_echo(CommandDetail& detail) {
	for (auto& arg : detail.args) 
		std::cout << arg << std::endl;

	return successful_procedure();
}

CommandError command_exit(CommandDetail& detail) {
	exit(EXIT_SUCCESS);
	return successful_procedure();
}

CommandError command_sys(CommandDetail& detail) {
	std::string cmd;

	for (auto& arg : detail.args) 
		cmd += arg + ' ';

	if (!cmd.empty())
		system(cmd.c_str());
	else
		return CommandError("Cannot run nothing in System Command.", CommandErrorType::CommandFailedError);
	return successful_procedure();
}

CommandError command_history(CommandDetail& detail) {
	std::cout << "History:\n";

	for (auto& h_cmd : cmd_history) {
		std::cout << h_cmd << std::endl;
	}

	return successful_procedure();
}

CommandError command_clear(CommandDetail& detail) {
	for (auto& arg : detail.args) {
		if (arg == "-h" || arg == "--history")
			cmd_history.clear();
		if (arg == "--screen" || arg == "-s")
			system("cls");
	}

	return successful_procedure();
}

CommandError command_help(CommandDetail& detail) {
	std::cout << "Commands: \n";

	for (auto& cmd : commands) {
		std::string arg_num = std::to_string(cmd.second.arg_num);
		std::cout << "Name: " << cmd.first << ", Description: " << cmd.second.description <<  " Arguments: " << ((cmd.second.arg_num >= 0) ? arg_num : "Any") << std::endl;
	}

	return successful_procedure();
}

std::string get_cd() {
	char buff[FILENAME_MAX]; //create string buffer to hold path
	GetCurrentDir(buff, FILENAME_MAX);
	std::string current_working_dir(buff);

	return current_working_dir;
}

CommandError command_cd(CommandDetail& detail) {
	if (detail.args.size() == 0) {
		current_cd = get_cd();
	}
	else if (_chdir(detail.args[0].c_str()) != 0) {
		return CommandError("Could not cd into directory.", CommandErrorType::CommandFailedError);
	}
	current_cd = get_cd();
	return successful_procedure();
}