// protected 的几种典型用法与反例。
// 编译：g++ -std=c++17 -Wall -Wextra -pedantic-errors protected.cpp -o protected.out
// 文中注释掉的语句都是预期编译失败的写法。

#include <cassert>
#include <cctype>
#include <cstddef>
#include <istream>
#include <locale>
#include <sstream>
#include <streambuf>
#include <string>
#include <utility>
#include <vector>

/* ------------------------------------------------------------------
 * 1. NVI：public 非虚入口 + protected 虚钩子
 *    protected 而不是 private 的原因：派生类需要调基类默认实现，
 *    也需要使用基类提供的复用原语。
 * ------------------------------------------------------------------ */
class ReportGenerator {
    public:
	std::string generate() const
	{
		std::string out = header();
		out += body();
		out += footer();
		return out;
	}

	virtual ~ReportGenerator() = default;

    protected:
	// 派生类可以覆盖，也可以写 ReportGenerator::header() 调用默认实现
	virtual std::string header() const
	{
		return "== report ==\n";
	}

	virtual std::string body() const = 0;

	// 给派生类用的复用原语，外部调用者不需要知道
	static std::string join(const std::vector<std::string> &lines)
	{
		std::string out;
		for (const auto &line : lines) {
			out += line;
			out += '\n';
		}
		return out;
	}

    private:
	virtual std::string footer() const
	{
		return "== end ==\n";
	}
};

class CsvReport : public ReportGenerator {
    protected:
	std::string body() const override
	{
		return join({ "a,1", "b,2" });
	}

	std::string header() const override
	{
		// 派生类能显式调用基类的 protected 虚函数实现
		return std::string("id,value\n") + ReportGenerator::header();
	}
};

static void test_nvi()
{
	CsvReport report;
	const ReportGenerator &generator = report;
	// generator.body();  // 错误：protected 成员不能被外部代码访问
	assert(generator.generate() ==
	       "id,value\n== report ==\na,1\nb,2\n== end ==\n");
}

/* ------------------------------------------------------------------
 * 2. 标准库的定制点：std::basic_streambuf
 *    protected 虚函数只给派生类重写，public 包装负责维护缓冲区不变量。
 * ------------------------------------------------------------------ */
class UpperBuf : public std::streambuf {
    public:
	explicit UpperBuf(std::string data) : data_(std::move(data)) {}

    protected:
	// 读空时由基类的 public 包装 sbumpc/sgetc 调到
	int_type underflow() override
	{
		if (gptr() < egptr()) {
			return traits_type::to_int_type(*gptr());
		}
		if (source_ >= data_.size()) {
			return traits_type::eof();
		}
		buf_ = static_cast<char>(std::toupper(static_cast<unsigned char>(
			data_[source_++])));
		// setg 是基类提供的 protected 原语，用来公布 get 区
		setg(&buf_, &buf_, &buf_ + 1);
		return traits_type::to_int_type(buf_);
	}

    private:
	std::string data_;
	std::size_t source_ = 0;
	char buf_ = 0;
};

static void test_streambuf()
{
	UpperBuf buffer("hello protected");
	std::istream in(&buffer);
	std::string word;
	in >> word;
	assert(word == "HELLO");
	in >> word;
	assert(word == "PROTECTED");
	// buffer.underflow();  // 错误：定制点是 protected 的
}

/* ------------------------------------------------------------------
 * 3. 生命周期协议：std::locale::facet 的 protected 构造/析构
 *    标准库借助 protected 析构函数把销毁工作留给持有引用计数的 locale。
 *    facet 的析构是 protected，派生类的析构仍然是 public（隐式生成）。
 * ------------------------------------------------------------------ */
class CommaPunct : public std::numpunct<char> {
    protected:
	char do_decimal_point() const override
	{
		return ',';
	}

	char do_thousands_sep() const override
	{
		return '.';
	}

	std::string do_grouping() const override
	{
		return "\3";
	}
};

static void test_facet()
{
	// refs 默认为 0，locale 取得所有权并负责在引用计数归零时 delete
	std::locale locale(std::locale::classic(), new CommaPunct);
	std::ostringstream out;
	out.imbue(locale);
	out << 1234567;
	assert(out.str() == "1.234.567");

	// std::locale::facet *facet = new CommaPunct;
	// delete facet;  // 错误：facet 的析构函数是 protected，
	//                //       只能由 std::locale 按引用计数销毁
}

/* ------------------------------------------------------------------
 * 4. 用 protected 构造/析构控制实例化与 delete
 * ------------------------------------------------------------------ */
class Handle {
    public:
	int id() const
	{
		return id_;
	}

    protected:
	explicit Handle(int id) : id_(id) {}
	// protected 且非虚：禁止 delete Handle*，同时不需要虚析构函数
	~Handle() = default;

    private:
	int id_;
};

class FileHandle : public Handle {
    public:
	explicit FileHandle(int id) : Handle(id) {}
	~FileHandle() = default; // 派生类析构仍能调用 protected 基类析构
};

static void test_protected_dtor()
{
	FileHandle file(7);
	assert(file.id() == 7);

	// Handle handle(1);        // 错误：构造函数是 protected
	// Handle *p = new FileHandle(1);
	// delete p;                // 错误：析构函数是 protected
}

// noncopyable 风格的基类：protected 构造/析构 + 删除的拷贝操作
// Boost 的做法是把构造、析构和删除的拷贝操作都放在 protected
class NonCopyable {
    protected:
	NonCopyable() = default;
	~NonCopyable() = default;
	NonCopyable(const NonCopyable &) = delete;
	NonCopyable &operator=(const NonCopyable &) = delete;
};

class Connection : private NonCopyable {
    public:
	Connection() = default;
};

static void test_noncopyable()
{
	Connection connection;
	(void)connection;
	// Connection copy = connection;  // 错误：拷贝构造函数被删除
}

/* ------------------------------------------------------------------
 * 5. CRTP / mixin 中的内部原语
 * ------------------------------------------------------------------ */
template <class Derived> struct RelationalOps {
    public:
	bool operator!=(const Derived &other) const
	{
		return !(self() == other);
	}

	bool operator>(const Derived &other) const
	{
		return other < self();
	}

    protected:
	const Derived &self() const
	{
		return static_cast<const Derived &>(*this);
	}
};

struct Money : RelationalOps<Money> {
    public:
	explicit Money(int cents) : cents_(cents) {}

	bool operator==(const Money &other) const
	{
		return cents_ == other.cents_;
	}

	bool operator<(const Money &other) const
	{
		return cents_ < other.cents_;
	}

	Money max(const Money &other) const
	{
		return self() < other ? other : self(); // 派生类使用 protected 原语
	}

    private:
	int cents_;
};

static void test_crtp()
{
	Money a(100);
	Money b(200);
	assert(a != b);
	assert(b > a);
	assert(b.max(a) == b);
}

/* ------------------------------------------------------------------
 * 6. 反例：protected 数据成员守不住不变量
 *    下面这段是能编译通过的，问题正在于它“能编译通过”。
 * ------------------------------------------------------------------ */
class WithProtectedData {
    protected:
	int size_ = 0;
};

class BreaksInvariant : public WithProtectedData {
    public:
	// 任何派生类都能把基类的状态改坏，基类无法拦截
	void corrupt()
	{
		size_ = -1;
	}
};

static void test_protected_data_is_bad()
{
	BreaksInvariant object;
	object.corrupt();
	(void)object;
}

int main()
{
	test_nvi();
	test_streambuf();
	test_facet();
	test_protected_dtor();
	test_noncopyable();
	test_crtp();
	test_protected_data_is_bad();
	return 0;
}
