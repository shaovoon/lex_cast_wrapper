# Boost Lexical Cast Wrapper

## Unhelpful Exception Message

```Cpp
#include <iostream>
#include <stdexcept>
#include <boost/lexical_cast.hpp>

int main()
{
    const char* str = "aa";
    try
    {
        int result = boost::lexical_cast<int>(str);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
```

Below is exception displayed and from the message we cannot know which type conversion failed if there are more than a few `lexical_cast` calls involved.

```
bad lexical cast: source type value could not be interpreted as target
```

Let&#39;s do it again with `std::string` type.

```Cpp
#include <string>
#include <iostream>
#include <stdexcept>
#include <boost/lexical_cast.hpp>

int main()
{
    const std::string str = "aa";
    try
    {
        int result = boost::lexical_cast<int>(str);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
```

With the `std::string` type, the same exception is displayed.

```
bad lexical cast: source type value could not be interpreted as target
```

## Wrap lexical_cast to Get Function Signature

Let&#39;s wrap `lexical_cast` to throw a `std::runtime_error` with function signature information.

```Cpp
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
        oss << "bad_lexical_cast exception thrown:\n";
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
```

Output from VC++ for `const char*` source arguement.

```Cpp
int __cdecl lexical_cast_wrapper<int,const char*>(const char *const &)
```

Output from GCC and Clang for `const char*` source arguement.

```Cpp
Target lexical_cast_wrapper(const Source&) [with Target = int; Source = const char*]
```

Output from VC++ for `std::string` source arguement.

```Cpp
int __cdecl lexical_cast_wrapper<int,class std::basic_string<char,struct std::char_traits<char,class std::allocator<char> >>(const class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > &)
```

Output from GCC and Clang for `std::string` source arguement.

```Cpp
Target lexical_cast_wrapper(const Source&) [with Target = int; Source = std::basic_string<char>]
```

The next step is to hand-craft 2 parser(1 for VC++ and another for GCC/Clang) to extract the source and target argument type.

```Cpp
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
            if (func[pos] == &#39;<&#39;)
                bstack.push(true);
            else if (func[pos] == &#39;>&#39;)
            {
                bstack.pop();
                if (bstack.size() == 0)
                    break;
            }

            if (func[pos] == &#39;,&#39;&&bstack.size() == 1)
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
```

For VC++ output, I choose to display a&nbsp;typedef&#39;ed type than the full&nbsp;type information. For GCC and Clang, I display the type as it is. Output as below.

```
VC++
bad_lexical_cast exception thrown:
Source arg:aa
Source type:std::string
Target type:int
```

```
GCC and Clang
bad_lexical_cast exception thrown:
Source arg:aa
Source type:std::basic_string<char>
Target type:int
```

Note: `lexical_cast_wrapper` throws `std::runtime_error`, not `bad_lexical_cast`.

It would be nice to have if Boost `lexical_cast` developer can implement this parsing inside `lexical_cast`. For my use case, a config file with 50 rows and 10 columns needs to be parsed, with this wrapper helps narrow down the problem quickly. This wrapper only have high overhead in exceptional case. If developer needs only to use this during testing, a `LEX_CAST` macro can be defined to direct the call accordingly and exception type to be caught must be changed from `bad_lexical_cast` to `std::exception`.

```Cpp
#ifdef _DEBUG
	#define LEX_CAST lexical_cast_wrapper
#else
	#define LEX_CAST boost::lexical_cast
#endif
```
