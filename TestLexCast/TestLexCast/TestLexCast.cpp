#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <stack>
#include <boost/lexical_cast.hpp>

#ifdef _WIN32
#define MY_FUNC_SIG __FUNCSIG__
#else
#define MY_FUNC_SIG __PRETTY_FUNCTION__
#endif


#ifdef _WIN32
std::string replace_string_type(const std::string& long_str_type)
{
	std::string result = (long_str_type.find("const") != std::string::npos) ? "const " : "";
	if (long_str_type.find("std::basic_string<char") != std::string::npos)
		result += "std::string";
	else if (long_str_type.find("std::basic_string<wchar_t") != std::string::npos)
		result += "std::wstring";
	else if (long_str_type.find("std::basic_string<char16_t") != std::string::npos)
		result += "std::u16string";
	else if (long_str_type.find("std::basic_string<char32_t") != std::string::npos)
		result += "std::u32string";
	else if (long_str_type.find("std::pmr::basic_string<char") != std::string::npos)
		result += "std::pmr::string";
	else if (long_str_type.find("std::pmr::basic_string<wchar_t") != std::string::npos)
		result += "std::pmr::wstring";
	else if (long_str_type.find("std::pmr::basic_string<char16_t") != std::string::npos)
		result += "std::pmr::u16string";
	else if (long_str_type.find("std::pmr::basic_string<char32_t") != std::string::npos)
		result += "std::pmr::u32string";
	else
		result = long_str_type;

	return result;
}

bool getTypes(const std::string& func, std::string& src_type, std::string& target_type)
{
	src_type = ""; 
	target_type = "";
	static const std::string func_prelude = "lexical_cast_wrapper<";
	size_t pos = func.find(func_prelude);
	if (pos != std::string::npos)
	{
		pos += func_prelude.size();
		std::stack<bool> bstack; // type of the stack does not matter
		bstack.push(true);
		bool parsing_target = true;
		while (pos < func.size())
		{
			if (func[pos] == '<')
				bstack.push(true);
			else if (func[pos] == '>')
			{
				bstack.pop();
				if (bstack.size() == 0)
					break;
			}

			if (func[pos] == ','&&bstack.size() == 1)
			{
				parsing_target = false;
				++pos;
				continue;
			}

			if (parsing_target)
				target_type += func[pos];
			else
				src_type += func[pos];

			++pos;
		}

		src_type    = replace_string_type(src_type);
		target_type = replace_string_type(target_type);

		return true;
	}
	return false;
}
#else
bool getTypes(const std::string& func, std::string& src_type, std::string& target_type)
{
	src_type = "";
	target_type = "";
	static const std::string prelude = "[with Target = ";
	static const std::string prelude2 = "; Source = ";

	size_t pos = func.find(prelude);
	if (pos != std::string::npos)
	{
		pos += prelude.size();
		size_t pos2 = func.find(prelude2, pos);
		if (pos2 != std::string::npos)
		{
			target_type = func.substr(pos, pos2 - pos);
			pos2 += prelude2.size();
			size_t posEnd = func.find("]", pos2);
			if (posEnd != std::string::npos)
			{
				src_type = func.substr(pos2, posEnd - pos2);
				return true;
			}
		}
	}
	return false;
}
#endif

template <typename Target, typename Source>
inline Target lexical_cast_wrapper(const Source &arg)
{
	Target result;
	try
	{
		result = boost::lexical_cast<Target>(arg);
	}
	catch (boost::bad_lexical_cast&)
	{
		std::string src_type = ""; std::string target_type = "";
		if (getTypes(MY_FUNC_SIG, src_type, target_type))
		{
			std::ostringstream oss;
			oss << "bad_lexical_cast exception thrown:";
			oss << "\nSource arg:" << arg;
			oss << "\nSource type:" << src_type;
			oss << "\nTarget type:" << target_type;
			throw std::runtime_error(oss.str().c_str());
		}
	}
	return result;
}

int main()
{
	//const char* str = "aa";
	const std::string str = "aa";
	try
	{
		int result = lexical_cast_wrapper<int>(str);
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}

/*
#include <string>
#include <iostream>
#include <boost/lexical_cast.hpp>

int main()
{
	//const char* str = "aa";
	const std::string str = "aa";
	try
	{
		int result = boost::lexical_cast<int>(str);
	}
	catch (boost::bad_lexical_cast& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
*/
/*
#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <boost/lexical_cast.hpp>

#ifdef _WIN32
#define MY_FUNC_SIG __FUNCSIG__
#else
#define MY_FUNC_SIG __PRETTY_FUNCTION__
#endif

template <typename Target, typename Source>
inline Target lexical_cast_wrapper(const Source &arg)
{
	Target result;

	try
	{
		result = boost::lexical_cast<Target>(arg);
	}
	catch (boost::bad_lexical_cast&)
	{
		std::ostringstream oss;
		oss << "lexical_cast exception thrown:\n";
		oss << "Source arg:<" << arg << ">\n";
		oss << "Function sig:" << MY_FUNC_SIG;
		throw std::runtime_error(oss.str().c_str());
	}

	return result;
}

int main()
{
	const std::string str = "aa";
	try
	{
		int result = lexical_cast_wrapper<int>(str);
	}
	catch (std::runtime_error& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
*/

/*
#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <boost/lexical_cast.hpp>

#ifdef _WIN32
#define MY_FUNC_SIG __FUNCSIG__
#else
#define MY_FUNC_SIG __PRETTY_FUNCTION__
#endif

template <typename Target, typename Source>
inline Target lexical_cast_wrapper(const Source &arg)
{
	Target result;

	try
	{
		result = boost::lexical_cast<Target>(arg);
	}
	catch (boost::bad_lexical_cast&)
	{
		std::ostringstream oss;
		oss << "lexical_cast exception thrown:\n";
		oss << "Source arg:<" << arg << ">\n";
		oss << "Function sig:" << MY_FUNC_SIG;
		throw std::runtime_error(oss.str().c_str());
	}

	return result;
}

int main()
{
	const std::string str = "aa";
	try
	{
		int result = lexical_cast_wrapper<int>(str);
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
*/