#include <charconv>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <system_error>

/*
 * 经典例子：错误码逐层返回与异常统一处理
 *
 * 程序通过下面三层调用来启动服务：
 *
 * main -> start_service -> load_config -> parse_port
 *
 * parse_port() 发现端口不合法时，只有 main() 知道如何向用户报告错误。
 *
 * 不使用异常时，每个函数必须同时传递正常结果和错误码。即使中间层无法
 * 处理错误，也必须检查错误码，再把它返回给上一层；漏掉任何一次检查都
 * 可能让错误被忽略。
 *
 * 使用异常时，函数签名和函数体只表达正常流程。parse_port() 抛出的异常
 * 会越过无法处理它的 load_config() 和 start_service()，最终由 main() 在
 * 程序边界统一捕获。这不是省略错误处理，而是将错误传播与正常返回路径
 * 分离开。
 *
 * 如果某个中间层有能力恢复，也可以在那里 catch；处理不了就不要捕获。
 * 对于可预期、频繁发生的失败，C++23 std::expected 往往更合适；异常适合
 * 当前函数无法处理、需要跨越多层传播的异常情况。
 */

struct Config {
	int port;
};

namespace without_exception
{

enum class Error {
	none,
	missing_port,
	invalid_port,
};

std::string_view message(Error error)
{
	switch (error) {
	case Error::none:
		return "success";
	case Error::missing_port:
		return "missing 'port='";
	case Error::invalid_port:
		return "port must be an integer from 1 to 65535";
	}
	return "unknown error";
}

Error parse_port(std::string_view text, int &port)
{
	if (text.empty())
		return Error::invalid_port;

	const char *first = text.data();
	const char *last = first + text.size();
	auto [end, error] = std::from_chars(first, last, port);
	if (error != std::errc{} || end != last || port < 1 || port > 65535)
		return Error::invalid_port;

	return Error::none;
}

Error load_config(std::string_view text, Config &config)
{
	constexpr std::string_view prefix = "port=";
	if (text.substr(0, prefix.size()) != prefix)
		return Error::missing_port;

	int port = 0;
	Error error = parse_port(text.substr(prefix.size()), port);
	// 本层处理不了，只能检查错误码并继续上传。
	if (error != Error::none)
		return error;

	config.port = port;
	return Error::none;
}

Error start_service(std::string_view text)
{
	Config config{};
	Error error = load_config(text, config);
	// 再上传一层；正常路径被错误传播逻辑打断。
	if (error != Error::none)
		return error;

	std::cout << "start service on port " << config.port << '\n';
	return Error::none;
}

} // namespace without_exception

namespace with_exception
{

class ConfigError : public std::runtime_error {
    public:
	using std::runtime_error::runtime_error;
};

int parse_port(std::string_view text)
{
	int port = 0;
	const char *first = text.data();
	const char *last = first + text.size();
	auto [end, error] = std::from_chars(first, last, port);
	if (text.empty() || error != std::errc{} || end != last || port < 1 ||
	    port > 65535) {
		throw ConfigError("port must be an integer from 1 to 65535");
	}
	return port;
}

Config load_config(std::string_view text)
{
	constexpr std::string_view prefix = "port=";
	if (text.substr(0, prefix.size()) != prefix)
		throw ConfigError("missing 'port='");

	// 不需要接收输出参数，也不需要检查并转发错误码。
	return Config{ parse_port(text.substr(prefix.size())) };
}

void start_service(std::string_view text)
{
	// load_config() 抛异常时，本函数会自动退出并继续向上传播。
	Config config = load_config(text);
	std::cout << "start service on port " << config.port << '\n';
}

} // namespace with_exception

int main()
{
	constexpr std::string_view config_text = "port=abc";

	std::cout << "without exception:\n";
	without_exception::Error error =
		without_exception::start_service(config_text);
	if (error != without_exception::Error::none)
		std::cout << "cannot start service: "
			  << without_exception::message(error) << '\n';

	std::cout << "\nwith exception:\n";
	// 在真正知道如何处理错误的程序边界统一捕获。
	try {
		with_exception::start_service(config_text);
	} catch (const with_exception::ConfigError &error) {
		std::cout << "cannot start service: " << error.what() << '\n';
	}
}
