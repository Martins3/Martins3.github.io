#![allow(dead_code)]
//   模式           说明
//  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//   类型状态模式   编译时状态检查，DraftPost → PendingReviewPost → PublishedPost，修复了原代码编译错误
//   状态模式       运行时多态，使用 Box<dyn State>，支持 reject 退回操作
//   Trait 多态     模拟继承，Drawable trait + Circle/Rectangle 实现
//   组合优于继承   UserService 组合 Logger 和 Database
//   访问者模式     Visitor + Visitable 实现双重分发
//   单元测试       3 个测试用例验证功能
//
//  关键修复
//
//  原代码的 oop() 函数无法编译，因为 Post::new() 返回 DraftPost，但调用的是 Post 的方法。现在已分离为三个明确的类型，状态转换通过消耗
//  self 实现，编译器会阻止非法状态转换。


// =============================================================================
// 1. 类型状态模式 (Type State Pattern) - 使用类型系统编码状态
// =============================================================================
// 优点：编译时保证状态转换的正确性，无运行时开销

pub struct DraftPost {
    content: String,
}

pub struct PendingReviewPost {
    content: String,
}

pub struct PublishedPost {
    content: String,
}

impl DraftPost {
    pub fn new() -> Self {
        DraftPost {
            content: String::new(),
        }
    }

    pub fn add_text(&mut self, text: &str) {
        self.content.push_str(text);
    }

    pub fn content(&self) -> &str {
        &self.content
    }

    /// 状态转换：Draft -> PendingReview
    /// 消耗 self，防止后续继续使用 DraftPost
    pub fn request_review(self) -> PendingReviewPost {
        PendingReviewPost {
            content: self.content,
        }
    }
}

impl PendingReviewPost {
    /// 状态转换：PendingReview -> Published
    pub fn approve(self) -> PublishedPost {
        PublishedPost {
            content: self.content,
        }
    }

    /// 可以拒绝审核，退回草稿状态
    pub fn reject(self) -> DraftPost {
        DraftPost {
            content: self.content,
        }
    }
}

impl PublishedPost {
    pub fn content(&self) -> &str {
        &self.content
    }
}

// =============================================================================
// 2. 状态模式 (State Pattern) - 运行时多态
// =============================================================================
// 优点：状态转换灵活，可以在运行时动态改变行为

pub struct PostWithState {
    state: Option<Box<dyn State>>,
    content: String,
}

impl PostWithState {
    pub fn new() -> Self {
        PostWithState {
            state: Some(Box::new(DraftState {})),
            content: String::new(),
        }
    }

    pub fn add_text(&mut self, text: &str) {
        // 只有草稿状态才能添加内容
        if let Some(state) = &self.state {
            if state.can_add_text() {
                self.content.push_str(text);
            }
        }
    }

    pub fn content(&self) -> &str {
        // 根据当前状态决定是否返回内容
        self.state.as_ref().unwrap().content(self)
    }

    pub fn request_review(&mut self) {
        if let Some(s) = self.state.take() {
            self.state = Some(s.request_review())
        }
    }

    pub fn approve(&mut self) {
        if let Some(s) = self.state.take() {
            self.state = Some(s.approve())
        }
    }

    pub fn reject(&mut self) {
        if let Some(s) = self.state.take() {
            self.state = Some(s.reject())
        }
    }
}

trait State {
    fn request_review(self: Box<Self>) -> Box<dyn State>;
    fn approve(self: Box<Self>) -> Box<dyn State>;
    fn reject(self: Box<Self>) -> Box<dyn State>;

    /// 默认不能添加文本
    fn can_add_text(&self) -> bool {
        false
    }

    /// 默认返回空字符串
    fn content<'a>(&self, _post: &'a PostWithState) -> &'a str {
        ""
    }
}

struct DraftState;

impl State for DraftState {
    fn request_review(self: Box<Self>) -> Box<dyn State> {
        Box::new(PendingReviewState {})
    }

    fn approve(self: Box<Self>) -> Box<dyn State> {
        self // 草稿不能直接发布
    }

    fn reject(self: Box<Self>) -> Box<dyn State> {
        self // 已经是草稿
    }

    fn can_add_text(&self) -> bool {
        true
    }
}

struct PendingReviewState;

impl State for PendingReviewState {
    fn request_review(self: Box<Self>) -> Box<dyn State> {
        self // 已在审核中
    }

    fn approve(self: Box<Self>) -> Box<dyn State> {
        Box::new(PublishedState {})
    }

    fn reject(self: Box<Self>) -> Box<dyn State> {
        Box::new(DraftState {})
    }
}

struct PublishedState;

impl State for PublishedState {
    fn request_review(self: Box<Self>) -> Box<dyn State> {
        self // 已发布
    }

    fn approve(self: Box<Self>) -> Box<dyn State> {
        self
    }

    fn reject(self: Box<Self>) -> Box<dyn State> {
        self // 已发布不能拒绝
    }

    fn content<'a>(&self, post: &'a PostWithState) -> &'a str {
        &post.content
    }
}

// =============================================================================
// 3. 继承与多态 - 使用 trait 模拟
// =============================================================================

/// 定义一个图形接口（模拟抽象基类）
pub trait Drawable {
    fn draw(&self);
    fn area(&self) -> f64;
    fn describe(&self) {
        println!("This is a drawable object with area: {}", self.area());
    }
}

pub struct Circle {
    radius: f64,
}

impl Circle {
    pub fn new(radius: f64) -> Self {
        Circle { radius }
    }
}

impl Drawable for Circle {
    fn draw(&self) {
        println!("Drawing a circle with radius {}", self.radius);
    }

    fn area(&self) -> f64 {
        std::f64::consts::PI * self.radius * self.radius
    }
}

pub struct Rectangle {
    width: f64,
    height: f64,
}

impl Rectangle {
    pub fn new(width: f64, height: f64) -> Self {
        Rectangle { width, height }
    }
}

impl Drawable for Rectangle {
    fn draw(&self) {
        println!("Drawing a rectangle {}x{}", self.width, self.height);
    }

    fn area(&self) -> f64 {
        self.width * self.height
    }

    fn describe(&self) {
        println!(
            "This is a rectangle {}x{} with area: {}",
            self.width,
            self.height,
            self.area()
        );
    }
}

/// 多态函数：接受任何实现了 Drawable trait 的对象
fn render_shapes(shapes: &[Box<dyn Drawable>]) {
    for shape in shapes {
        shape.draw();
        shape.describe();
        println!();
    }
}

// =============================================================================
// 4. 组合优于继承 - 使用组合实现代码复用
// =============================================================================

pub struct Logger;

impl Logger {
    pub fn log(&self, message: &str) {
        println!("[LOG] {}", message);
    }
}

pub struct Database {
    connection_string: String,
}

impl Database {
    pub fn new(connection: &str) -> Self {
        Database {
            connection_string: connection.to_string(),
        }
    }

    pub fn query(&self, sql: &str) {
        println!("Querying '{}' on {}", sql, self.connection_string);
    }
}

/// 通过组合而不是继承来复用功能
pub struct UserService {
    logger: Logger,
    database: Database,
}

impl UserService {
    pub fn new() -> Self {
        UserService {
            logger: Logger,
            database: Database::new("localhost:5432"),
        }
    }

    pub fn create_user(&self, name: &str) {
        self.logger.log(&format!("Creating user: {}", name));
        self.database
            .query(&format!("INSERT INTO users VALUES ('{}')", name));
    }
}

// =============================================================================
// 5. 访问者模式 (Visitor Pattern) - 双重分发
// =============================================================================

pub trait Visitor {
    fn visit_circle(&mut self, circle: &Circle);
    fn visit_rectangle(&mut self, rect: &Rectangle);
}

pub trait Visitable {
    fn accept(&self, visitor: &mut dyn Visitor);
}

impl Visitable for Circle {
    fn accept(&self, visitor: &mut dyn Visitor) {
        visitor.visit_circle(self);
    }
}

impl Visitable for Rectangle {
    fn accept(&self, visitor: &mut dyn Visitor) {
        visitor.visit_rectangle(self);
    }
}

/// 计算周长的访问者
struct PerimeterCalculator;

impl Visitor for PerimeterCalculator {
    fn visit_circle(&mut self, circle: &Circle) {
        let perimeter = 2.0 * std::f64::consts::PI * circle.radius;
        println!("Circle perimeter: {:.2}", perimeter);
    }

    fn visit_rectangle(&mut self, rect: &Rectangle) {
        let perimeter = 2.0 * (rect.width + rect.height);
        println!("Rectangle perimeter: {:.2}", perimeter);
    }
}

// =============================================================================
// 6. 建造者模式 (Builder Pattern) - 构建复杂对象
// =============================================================================

pub struct Computer {
    cpu: String,
    memory: u32,  // GB
    storage: u32, // GB
    gpu: Option<String>,
    wifi: bool,
}

impl Computer {
    pub fn builder() -> ComputerBuilder {
        ComputerBuilder::new()
    }

    pub fn specs(&self) -> String {
        format!(
            "Computer: {} CPU, {}GB RAM, {}GB SSD, GPU: {}, WiFi: {}",
            self.cpu,
            self.memory,
            self.storage,
            self.gpu.as_deref().unwrap_or("None"),
            self.wifi
        )
    }
}

pub struct ComputerBuilder {
    cpu: String,
    memory: u32,
    storage: u32,
    gpu: Option<String>,
    wifi: bool,
}

impl ComputerBuilder {
    pub fn new() -> Self {
        ComputerBuilder {
            cpu: "Intel i5".to_string(),
            memory: 8,
            storage: 256,
            gpu: None,
            wifi: false,
        }
    }

    pub fn cpu(mut self, cpu: &str) -> Self {
        self.cpu = cpu.to_string();
        self
    }

    pub fn memory(mut self, gb: u32) -> Self {
        self.memory = gb;
        self
    }

    pub fn storage(mut self, gb: u32) -> Self {
        self.storage = gb;
        self
    }

    pub fn gpu(mut self, gpu: &str) -> Self {
        self.gpu = Some(gpu.to_string());
        self
    }

    pub fn wifi(mut self, enabled: bool) -> Self {
        self.wifi = enabled;
        self
    }

    pub fn build(self) -> Computer {
        Computer {
            cpu: self.cpu,
            memory: self.memory,
            storage: self.storage,
            gpu: self.gpu,
            wifi: self.wifi,
        }
    }
}

// =============================================================================
// 7. 策略模式 (Strategy Pattern) - 算法族
// =============================================================================

pub trait PaymentStrategy {
    fn pay(&self, amount: f64) -> Result<(), String>;
}

pub struct CreditCard {
    number: String,
    cvv: String,
}

impl CreditCard {
    pub fn new(number: &str, cvv: &str) -> Self {
        CreditCard {
            number: number.to_string(),
            cvv: cvv.to_string(),
        }
    }
}

impl PaymentStrategy for CreditCard {
    fn pay(&self, amount: f64) -> Result<(), String> {
        println!(
            "Paying ${:.2} using Credit Card ending in {}",
            amount,
            &self.number[self.number.len() - 4..]
        );
        Ok(())
    }
}

pub struct PayPal {
    email: String,
}

impl PayPal {
    pub fn new(email: &str) -> Self {
        PayPal {
            email: email.to_string(),
        }
    }
}

impl PaymentStrategy for PayPal {
    fn pay(&self, amount: f64) -> Result<(), String> {
        println!("Paying ${:.2} using PayPal account {}", amount, self.email);
        Ok(())
    }
}

pub struct CryptoWallet {
    address: String,
}

impl CryptoWallet {
    pub fn new(address: &str) -> Self {
        CryptoWallet {
            address: address.to_string(),
        }
    }
}

impl PaymentStrategy for CryptoWallet {
    fn pay(&self, amount: f64) -> Result<(), String> {
        println!("Paying ${:.2} using Crypto Wallet", amount);
        println!(
            "  (Blockchain transaction sent from {})",
            &self.address[..8]
        );
        Ok(())
    }
}

pub struct ShoppingCart {
    items: Vec<(String, f64)>,
    payment_strategy: Option<Box<dyn PaymentStrategy>>,
}

impl ShoppingCart {
    pub fn new() -> Self {
        ShoppingCart {
            items: Vec::new(),
            payment_strategy: None,
        }
    }

    pub fn add_item(&mut self, name: &str, price: f64) {
        self.items.push((name.to_string(), price));
    }

    pub fn set_payment_strategy(&mut self, strategy: Box<dyn PaymentStrategy>) {
        self.payment_strategy = Some(strategy);
    }

    pub fn total(&self) -> f64 {
        self.items.iter().map(|(_, price)| price).sum()
    }

    pub fn checkout(&self) -> Result<(), String> {
        let total = self.total();
        match &self.payment_strategy {
            Some(strategy) => strategy.pay(total),
            None => Err("No payment method selected".to_string()),
        }
    }
}

// =============================================================================
// 8. 工厂模式 (Factory Pattern) - 对象创建
// =============================================================================

pub trait Animal {
    fn speak(&self);
    fn name(&self) -> &str;
}

pub struct Dog {
    name: String,
}

impl Animal for Dog {
    fn speak(&self) {
        println!("{} says: Woof!", self.name);
    }
    fn name(&self) -> &str {
        &self.name
    }
}

pub struct Cat {
    name: String,
}

impl Animal for Cat {
    fn speak(&self) {
        println!("{} says: Meow!", self.name);
    }
    fn name(&self) -> &str {
        &self.name
    }
}

pub struct Duck {
    name: String,
}

impl Animal for Duck {
    fn speak(&self) {
        println!("{} says: Quack!", self.name);
    }
    fn name(&self) -> &str {
        &self.name
    }
}

pub enum AnimalType {
    Dog,
    Cat,
    Duck,
}

/// 简单工厂
pub struct AnimalFactory;

impl AnimalFactory {
    pub fn create(animal_type: AnimalType, name: &str) -> Box<dyn Animal> {
        match animal_type {
            AnimalType::Dog => Box::new(Dog {
                name: name.to_string(),
            }),
            AnimalType::Cat => Box::new(Cat {
                name: name.to_string(),
            }),
            AnimalType::Duck => Box::new(Duck {
                name: name.to_string(),
            }),
        }
    }
}

// =============================================================================
// 9. 观察者模式 (Observer Pattern) - 事件订阅
// =============================================================================

use std::cell::RefCell;
use std::rc::Rc;

pub trait Observer {
    fn update(&self, temperature: f32, humidity: f32);
}

pub struct WeatherStation {
    temperature: RefCell<f32>,
    humidity: RefCell<f32>,
    observers: RefCell<Vec<Rc<dyn Observer>>>,
}

impl WeatherStation {
    pub fn new() -> Self {
        WeatherStation {
            temperature: RefCell::new(0.0),
            humidity: RefCell::new(0.0),
            observers: RefCell::new(Vec::new()),
        }
    }

    pub fn register_observer(&self, observer: Rc<dyn Observer>) {
        self.observers.borrow_mut().push(observer);
    }

    pub fn remove_observer(&self, observer: &Rc<dyn Observer>) {
        // 简化的移除逻辑，实际应用可能需要更复杂的比较
        self.observers
            .borrow_mut()
            .retain(|o| !Rc::ptr_eq(o, observer));
    }

    pub fn set_measurements(&self, temp: f32, humidity: f32) {
        *self.temperature.borrow_mut() = temp;
        *self.humidity.borrow_mut() = humidity;
        self.notify_observers();
    }

    fn notify_observers(&self) {
        let temp = *self.temperature.borrow();
        let humidity = *self.humidity.borrow();
        for observer in self.observers.borrow().iter() {
            observer.update(temp, humidity);
        }
    }
}

pub struct PhoneDisplay;

impl Observer for PhoneDisplay {
    fn update(&self, temperature: f32, humidity: f32) {
        println!(
            "📱 Phone Display: Temp {:.1}°C, Humidity {:.1}%",
            temperature, humidity
        );
    }
}

pub struct DesktopDisplay;

impl Observer for DesktopDisplay {
    fn update(&self, temperature: f32, humidity: f32) {
        println!(
            "🖥️  Desktop Display: Temperature = {:.1}°C, Humidity = {:.1}%",
            temperature, humidity
        );
    }
}

pub struct LoggerDisplay;

impl Observer for LoggerDisplay {
    fn update(&self, temperature: f32, humidity: f32) {
        println!(
            "📝 Logger: Weather updated - T:{:.1} H:{:.1}",
            temperature, humidity
        );
    }
}

// =============================================================================
// 10. Newtype 模式 - 类型安全包装
// =============================================================================

/// 使用 Newtype 模式确保类型安全
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub struct UserId(u64);

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub struct ProductId(u64);

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub struct OrderId(u64);

impl UserId {
    pub fn new(id: u64) -> Self {
        UserId(id)
    }
    pub fn value(&self) -> u64 {
        self.0
    }
}

impl ProductId {
    pub fn new(id: u64) -> Self {
        ProductId(id)
    }
    pub fn value(&self) -> u64 {
        self.0
    }
}

impl OrderId {
    pub fn new(id: u64) -> Self {
        OrderId(id)
    }
    pub fn value(&self) -> u64 {
        self.0
    }
}

/// 演示 Newtype 模式的优势
pub fn create_order(user_id: UserId, product_id: ProductId) -> OrderId {
    println!(
        "Creating order for user {} with product {}",
        user_id.value(),
        product_id.value()
    );
    OrderId::new(1001)
}

// =============================================================================
// 11. Deref 模式 - 智能指针
// =============================================================================

use std::ops::Deref;

/// 自定义智能指针，带引用计数和调试信息
pub struct DebugBox<T> {
    value: T,
    name: String,
}

impl<T> DebugBox<T> {
    pub fn new(value: T, name: &str) -> Self {
        println!("📦 Creating DebugBox '{}'", name);
        DebugBox {
            value,
            name: name.to_string(),
        }
    }
}

impl<T> Deref for DebugBox<T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        println!("📦 Dereferencing DebugBox '{}'", self.name);
        &self.value
    }
}

impl<T> Drop for DebugBox<T> {
    fn drop(&mut self) {
        println!("📦 Dropping DebugBox '{}'", self.name);
    }
}

pub fn oop() {
    println!("=== 1. 类型状态模式 ===");
    let mut draft = DraftPost::new();
    draft.add_text("Hello, Rust!");
    println!("Draft content: {}", draft.content());
    let pending = draft.request_review();
    let published = pending.approve();
    println!("Published content: {}", published.content());

    println!("\n=== 2. 状态模式（运行时多态） ===");
    let mut post = PostWithState::new();
    post.add_text("Runtime state management");
    println!("Content (draft): '{}'", post.content());
    post.request_review();
    post.add_text("ignored");
    post.approve();
    println!("Content (published): '{}'", post.content());

    println!("\n=== 3. 多态与 trait ===");
    let shapes: Vec<Box<dyn Drawable>> = vec![
        Box::new(Circle::new(5.0)),
        Box::new(Rectangle::new(4.0, 6.0)),
    ];
    render_shapes(&shapes);

    println!("=== 4. 组合优于继承 ===");
    let service = UserService::new();
    service.create_user("Alice");

    println!("\n=== 5. 访问者模式 ===");
    let shapes: Vec<Box<dyn Visitable>> = vec![
        Box::new(Circle::new(3.0)),
        Box::new(Rectangle::new(4.0, 5.0)),
    ];
    let mut visitor = PerimeterCalculator;
    for shape in &shapes {
        shape.accept(&mut visitor);
    }

    println!("\n=== 6. 建造者模式 ===");
    let computer = Computer::builder()
        .cpu("AMD Ryzen 9")
        .memory(32)
        .storage(1024)
        .gpu("RTX 4090")
        .wifi(true)
        .build();
    println!("{}", computer.specs());

    // 也可以使用默认值
    let basic_computer = Computer::builder().build();
    println!("{}", basic_computer.specs());

    println!("\n=== 7. 策略模式 ===");
    let mut cart = ShoppingCart::new();
    cart.add_item("Laptop", 999.99);
    cart.add_item("Mouse", 29.99);

    println!("Cart total: ${:.2}", cart.total());

    // 使用信用卡支付
    cart.set_payment_strategy(Box::new(CreditCard::new("1234567890123456", "123")));
    cart.checkout().unwrap();

    // 或者使用 PayPal
    let mut cart2 = ShoppingCart::new();
    cart2.add_item("Keyboard", 79.99);
    cart2.set_payment_strategy(Box::new(PayPal::new("user@example.com")));
    cart2.checkout().unwrap();

    println!("\n=== 8. 工厂模式 ===");
    let animals: Vec<Box<dyn Animal>> = vec![
        AnimalFactory::create(AnimalType::Dog, "Buddy"),
        AnimalFactory::create(AnimalType::Cat, "Whiskers"),
        AnimalFactory::create(AnimalType::Duck, "Daffy"),
    ];
    for animal in animals {
        animal.speak();
    }

    println!("\n=== 9. 观察者模式 ===");
    let weather_station = WeatherStation::new();
    let phone = Rc::new(PhoneDisplay);
    let desktop = Rc::new(DesktopDisplay);
    let logger = Rc::new(LoggerDisplay);

    weather_station.register_observer(phone.clone());
    weather_station.register_observer(desktop.clone());
    weather_station.register_observer(logger.clone());

    println!("First update:");
    weather_station.set_measurements(25.0, 65.0);

    println!("\nSecond update:");
    weather_station.set_measurements(28.5, 70.0);

    println!("\n=== 10. Newtype 模式 ===");
    let user_id = UserId::new(42);
    let product_id = ProductId::new(100);
    let order_id = create_order(user_id, product_id);
    println!("Created order with ID: {}", order_id.value());
    // 下面的代码会导致编译错误，防止 ID 混淆：
    // create_order(product_id, user_id); // 错误：类型不匹配

    println!("\n=== 11. Deref 模式 ===");
    {
        let debug_int = DebugBox::new(42, "my_number");
        println!("Value: {}", *debug_int); // 使用 Deref
    } // 自动调用 Drop
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_type_state_pattern() {
        let mut draft = DraftPost::new();
        draft.add_text("Test content");
        let pending = draft.request_review();
        let published = pending.approve();
        assert_eq!(published.content(), "Test content");
    }

    #[test]
    fn test_state_pattern_with_reject() {
        let mut post = PostWithState::new();
        post.add_text("Content");
        post.request_review();
        post.reject();
        post.add_text(" more");
        post.request_review();
        post.approve();
        assert_eq!(post.content(), "Content more");
    }

    #[test]
    fn test_circle_area() {
        let circle = Circle::new(2.0);
        assert!((circle.area() - 12.5664).abs() < 0.001);
    }

    #[test]
    fn test_builder_pattern() {
        let computer = Computer::builder()
            .cpu("Intel i7")
            .memory(16)
            .storage(512)
            .wifi(true)
            .build();

        assert!(computer.specs().contains("Intel i7"));
        assert!(computer.specs().contains("16GB"));
    }

    #[test]
    fn test_strategy_pattern() {
        let mut cart = ShoppingCart::new();
        cart.add_item("Test Item", 100.0);
        cart.set_payment_strategy(Box::new(CreditCard::new("1234567890123456", "123")));
        assert!(cart.checkout().is_ok());
    }

    #[test]
    fn test_factory_pattern() {
        let dog = AnimalFactory::create(AnimalType::Dog, "Rex");
        assert_eq!(dog.name(), "Rex");

        let cat = AnimalFactory::create(AnimalType::Cat, "Mittens");
        assert_eq!(cat.name(), "Mittens");
    }

    #[test]
    fn test_newtype_pattern() {
        let user_id = UserId::new(1);
        let product_id = ProductId::new(2);

        // 值可以相同但类型不同
        assert_eq!(user_id.value(), 1);
        assert_eq!(product_id.value(), 2);

        // UserId 和 ProductId 是不同的类型，以下代码会导致编译错误：
        // let ids_equal = user_id == product_id; // 错误：类型不匹配
        // create_order(product_id, user_id); // 错误：参数类型不匹配
    }

    #[test]
    fn test_deref_pattern() {
        let debug_box = DebugBox::new(100, "test");
        assert_eq!(*debug_box, 100);
    }
}
