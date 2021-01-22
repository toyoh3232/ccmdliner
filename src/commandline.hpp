#ifndef UTILS_H
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <variant>
#include <type_traits>
#include <stdexcept>
#include <charconv>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#if defined(_WIN32)
#include <windows.h>
#endif
#if defined(_GNU_SOURCE)
#include <errno.h>
#endif
namespace utils
{
	namespace helper
	{
		inline std::string to_lower(const std::string& str)
		{
			std::string lowerstr = str;
			for (int i = 0; i < str.length(); i++)
				lowerstr[i] = std::tolower(str[i]);
			return lowerstr;
		}
		inline std::string to_upper(const std::string& str)
		{
			std::string upperstr = str;
			for (int i = 0; i < str.length(); i++)
				upperstr[i] = std::toupper(str[i]);
			return upperstr;
		}
		inline std::string normalize(const std::string& str)
		{
		#if defined(PLATFORM_POSIX) || defined(__linux__) // check defines for your setup
			return str;
		#elif defined(_WIN32)
			return to_lower(str);
		#else
			static_assert(false, "unrecognized platform");
		#endif
		}
		inline std::string denormalize(const std::string& str)
		{
		#if defined(PLATFORM_POSIX) || defined(__linux__) // check defines for your setup
			return str;
		#elif defined(_WIN32)
			return to_upper(str);
		#else
			static_assert(false, "unrecognized platform");
		#endif
		}
		inline std::string delimiter()
		{
		#if defined(PLATFORM_POSIX) || defined(__linux__) // check defines for your setup
			return  "-";
		#elif defined(_WIN32)
			return "/";
		#else
			static_assert(false, "unrecognized platform");
		#endif
		}
		inline std::string delimiter(const std::string& op)
		{
			return delimiter() + op;
		}
		inline std::string exec_name()
		{
		# if defined(_GNU_SOURCE) // if GNU extension could be used
			std::string sp(program_invocation_short_name);
			return sp;
		#elif defined(PLATFORM_POSIX) || defined(__linux__) // check defines for your setup
			std::string sp;
			std::ifstream("/proc/self/comm") >> sp;
			return sp;
		#elif defined(_WIN32)
			char buf[MAX_PATH];
			GetModuleFileNameA(nullptr, buf, MAX_PATH);
			std::string name = buf;
			auto p = name.rfind('\\');
			if (p != std::string::npos)
				return name.substr(p + 1);
			return name;
		#else
			static_assert(false, "unrecognized platform");
		#endif
		}
		namespace hidden
		{
			template<typename T>
			inline void replace(std::string& ori_str, T&& value, int total)
			{
				// repalce with {0}
				std::ostringstream oss;
				oss << value;
				auto pos = ori_str.find("{0}");
				if (pos == std::string::npos)
				{
					throw std::runtime_error("placeholder index error");
				}
				ori_str.erase(pos, 3).insert(pos, oss.str());
				// decrement of placeholer
				while (total > 0)
				{
					std::string old_placeholder = "{" + std::to_string(total) + "}";
					std::string new_placeholder = "{" + std::to_string(total - 1) + "}";
					auto pos = ori_str.find(old_placeholder);
					if (pos == std::string::npos)
					{
						throw std::runtime_error("placeholder index error");
					}
					ori_str.erase(pos, old_placeholder.size()).insert(pos, new_placeholder);
					total--;
				}
			}
		}
		inline std::string format(const std::string& fmt)
		{
			return fmt;
		}
		template<typename T, typename ...Args>
		std::string format(std::string fmt, T&& val, Args&&... args)
		{
			hidden::replace(fmt, val, sizeof... (args));
			return format(fmt, args...);
		}
	}
	using argment_variant = std::variant<bool, int, long long, std::string>;
	using customized_cast_func = std::function<argment_variant(const std::string&)>;
	class commandline
	{
	public:
		commandline() = default;
		commandline(const commandline&) = delete;       // copy constructor disabled
		commandline(const commandline&&) = delete;      // move constructor disabled
		commandline& operator=(commandline&) = delete;  // copy assignment operator disabled
		commandline& operator=(commandline&&) = delete; // move assignment operator disabled
	private:
		struct action
		{
			std::string help_msg;
			int number_of_oprands_expected;
		};
		struct option
		{
			std::string help_msg;
			customized_cast_func castor;
		};
	public:
		// default number of expected operands is -1, means any   
		void add_action(const std::string& name, const std::string& help_msg, int number_of_oprands_expected = -1)
		{
			// occurrence check
			_actions[helper::normalize(name)] = { help_msg, number_of_oprands_expected };
		}
		void add_option(const std::string& name, const std::string& help_msg, customized_cast_func customized_castor = customized_cast_func())
		{
			// occurrence check
			_options[helper::normalize(name)] = { help_msg, customized_castor };
		}
		std::string usage() const
		{
			std::ostringstream oss;
			oss << std::setiosflags(std::ios::left);
			oss << std::endl;
			// display syntax
			oss << "Usage: " << helper::exec_name();
			if (_actions.empty() == false)
				oss << " action";
			oss << " [options]" << " [oprands]" << std::endl;
			// display action
			if (_actions.empty() == false)
			{
				oss << std::endl;
				oss << "Supported actions:" << std::endl;
				for (auto& p : _actions) {
					auto& action = p.first;
					oss << "  " << std::setw(40) << helper::denormalize(action) << p.second.help_msg << std::endl;
				}
			}
			// display options
			if (_options.empty() == false)
			{
				oss << std::endl;
				oss << "Supported options:" << std::endl;
				for (auto& p : _options) {
					auto option = helper::delimiter(p.first);
					oss << "  " << std::setw(40) << helper::denormalize(option) << p.second.help_msg << std::endl;
				}
			}
			// display example
			if (example.empty() == false)
			{
				oss << "Example:" << std::endl;
				oss << "  " << this->example << std::endl;
			}
			//display footer
			if (footer.empty() == false)
			{
				oss << std::endl;
				oss << this->footer << std::endl;
			}
			return oss.str();
		}
	public:
		std::string overview;
		std::string example;
		std::string footer;
	private:
		std::map<std::string, action> _actions;
		std::map<std::string, option> _options;
	public:
		class parsed_error : public std::logic_error
		{
		public:
			explicit parsed_error(const std::string& msg) : std::logic_error(msg)
			{
			#if defined(PLATFORM_POSIX) || defined(__linux__) // check defines for your setup
				std::string op = "help";
			#elif defined(_WIN32)
				std::string op = "?";
			#else
				static_assert(false, "unrecognized platform");
			#endif
				auto em = helper::format("{0}: {1}", helper::exec_name(), std::logic_error::what());
				auto hm = helper::format("Try '{0} {1}' for more information.", helper::exec_name(), helper::delimiter(op));
				_msg = helper::format("{0}\n{1}", em, hm);
			}
			parsed_error(const std::string& err_msg, const std::string& help_msg) : std::logic_error(err_msg)
			{
				auto em = helper::format("{0}: {1}", helper::exec_name(), std::logic_error::what());
				_msg = helper::format("{0}\n{1}", em, help_msg);
			}
			~parsed_error() = default;
		public:
			const char* what() const noexcept
			{
				return _msg.c_str();
			}
		private:
			std::string _msg;
		};
		class parsed_result;
		struct oprands_entity
		{
			friend class parsed_result;
			std::string operator[](int index) const
			{
				// if index is no int [size,size), just return empty string
				if (index < 0)
					index = index + _parsed_result->_arguments.size();
				if (index < 0 || index  >= _parsed_result->_arguments.size())
					return std::string();
				return _parsed_result->_arguments[index];
			}
			size_t size()
			{
				return _parsed_result->_arguments.size();
			}
		private:
			explicit oprands_entity(parsed_result* base)
			{
				_parsed_result = base;
			}
		private:
			parsed_result* _parsed_result;
		};
		template <typename T>
		struct option_result
		{
			bool ok;
			T    result;
		};
		class parsed_result
		{
		public:
			std::string action() const
			{
				return _action;
			}
			template< typename T = bool>
			inline option_result<T> option(const std::string& op) const
			{
				static_assert(std::is_same<bool, T>() || std::is_same<int, T>() || std::is_same<long long, T>() || std::is_same<std::string, T>(), "unsupported type");
				auto it = _options.find(op);
				if (it == _options.end())
					return {false, T()};
				auto [v, f] = (*it).second;
				if (!f)
					f = commandline::basic_castor<T>();
				return { true, std::get<T>(f(v))};
			}
			oprands_entity oprands = oprands_entity(this);
		private:
			struct option_val
			{
				std::string value;
				customized_cast_func castor;
			};
		private:
			parsed_result() = default;
			friend class oprands_entity;
			friend class commandline;
		private:
			std::string _action;
			std::map<std::string, option_val> _options;
			std::vector<std::string> _arguments;
		};
	public:
			template<typename T>
			static customized_cast_func basic_castor();
	public:
		parsed_result parse(int argc, char* argv[])
		{
			parsed_result res;
			std::vector<std::string> arguments;
			for (int i = 1; i < argc; i++)
				arguments.push_back(std::string(argv[i]));
			// if help option is set in the arguments, show usage
			if (!arguments.empty())
			{
			#if  defined(_WIN32)
				if (arguments[0] == "/?")
				{
					std::cout << usage() << std::endl;
					exit(0);
				}
			#endif
				if (helper::normalize(arguments[0]) == helper::normalize(helper::delimiter("help")))
				{
					std::cout << usage() << std::endl;
					exit(0);
				}
			}
			int nae = -1;
			// parse action if necessary
			if (!_actions.empty())
			{
				if (arguments.empty())
					throw parsed_error("no action");
				if (arguments[0].find("/") == 0 && !_options.empty())
					throw parsed_error("no action", "Action shoud appear before options.");
				if (_actions.find(helper::normalize(arguments[0])) == _actions.end())
				{
					if (auto it=std::find_if(_actions.begin(),_actions.end(),[&](auto& p) 
					   {
						   return helper::normalize(arguments[0]).find(p.first) != std::string::npos; 
					   }); it != std::end(_actions))
						   throw parsed_error(helper::format("unknown action '{0}'", arguments[0]), helper::format("Do you mean action '{0}'?",it->first));
					throw parsed_error(helper::format("unknown action '{0}'", arguments[0]));
				}
				res._action = arguments[0];
				nae = _actions[res._action].number_of_oprands_expected;
				// remove the action string in current argument sequence
				arguments.erase(arguments.begin());
			}
			// parse options
			while (!_options.empty())
			{
				if (arguments.empty())
					break;
				// if argument[0] is not started with delimiter, then it is an operand
				if (arguments[0].find(helper::delimiter()) != 0)
					break;
				// initialize the default
				std::string arg = arguments[0].substr(arguments[0].find(helper::delimiter()) + 1);
				std::string val = "true";
				// if argument has explicit value
				#if defined(PLATFORM_POSIX) || defined(__linux__) // check defines for your setup
					char splitter = '=';
				#elif defined(_WIN32)
					char splitter = ':';
				#else
					static_assert(false, "unrecognized platform");
				#endif
				if (auto i = arg.find(splitter); i != std::string::npos)
				{
					val = arg.substr(i + 1);
					arg = arg.substr(0, i);
				}
				// if argument is not found in configuration table for options
				if (_options.find(helper::normalize(arg)) == _options.end())
				{
					if (auto it=std::find_if(std::begin(_options), std::end(_options), [&](auto& p)
					{
						return helper::normalize(arg).find(p.first) != std::string::npos; 
					}); it != std::end(_options))
						throw parsed_error(helper::format("unknown option '{0}{1}'",helper::delimiter(), arg), helper::format("Do you mean option '{0}{1}'?",helper::delimiter(), it->first));
					throw parsed_error(helper::format("unknown option '{0}{1}'", helper::delimiter(), arg));
				}
				// redundant option is allowed
				res._options[helper::normalize(arg)] = { val, _options[helper::normalize(arg)].castor};
				// delete this argument which has been parsed
				arguments.erase(arguments.begin());
			}
			// parse operands
			// expected number of operands not match
			if (nae >= 0 && arguments.size() > nae)
				throw parsed_error("too many operands", helper::format("The action '{0}' needs {1} operand(s).", helper::denormalize(res._action), nae));
			if (nae >= 0 && arguments.size() < nae)
				throw parsed_error("not enough operands", helper::format("The action '{0}' needs {1} operand(s).", helper::denormalize(res._action), nae));
			// if no constraint or number_of_oprands_expected constraint
			for (auto& s : arguments)
			{
				// options appearance failure
				if (s.find(helper::delimiter()) == 0)
					throw parsed_error(helper::format("invalid operand '{0}'", s), helper::format("Options should appear before operands, is it an option?"));
				res._arguments.push_back(s);
			}
			return res;
		}
	};
	// template cast function speciallization
	template<>
	inline customized_cast_func commandline::basic_castor<bool>()
	{
		return [](const std::string& str)
		{
			if (helper::normalize(str) == helper::normalize("true"))
				return true;
			return helper::normalize(str) == helper::normalize("false") ? false : throw parsed_error(helper::format("invalid option value '{0}'", str));
		};
	}
	template<>
	inline customized_cast_func commandline::basic_castor<long long>()
	{
		return [](const std::string& str)
		{
			// use std::from_chars
			long long result;
			auto [p, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
			if (std::strlen(p) != 0)
				throw parsed_error("invalid pattern", helper::format("The option value '{0}' does not seem to be a decimal.", str));
			switch (ec)
			{
			case std::errc::invalid_argument:
				throw parsed_error("invalid pattern", helper::format("The option value '{0}' does not seem to be a decimal.", str));
				break;
			case std::errc::result_out_of_range:
				throw parsed_error("out of range", helper::format("The option value '{0}' is beyond the range of type 'long long'.", str));
				break;
			}
			return result;
		};
	}
	template<>
	inline customized_cast_func commandline::basic_castor<int>()
	{
		return [](const std::string& str)
		{
			// use std::from_chars
			int result;
			auto [p, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
			if (std::strlen(p) != 0)
				throw parsed_error("invalid pattern", helper::format("The option value '{0}' does not seem to be a decimal.", str));
			switch (ec)
			{
			case std::errc::invalid_argument:
				throw parsed_error("invalid pattern", helper::format("The option value '{0}' does not seem to be a decimal.", str));
				break;
			case std::errc::result_out_of_range:
				throw parsed_error("out of range", helper::format("The option value '{0}' is beyond the range of type 'int'.", str));
				break;
			}
			return result;
		};
	}
	template<>
	inline customized_cast_func commandline::basic_castor<std::string>()
	{
		return [](const std::string& str)
		{
			return str;
		};
	}
}
#define UTILS_H
#endif // !UTILS_H
