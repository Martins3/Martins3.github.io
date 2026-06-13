/// This module demonstrates how to write unit tests in Rust
///
/// Unit tests in Rust are functions with the #[test] attribute
/// They should be placed in the same file as the code they test
/// or in a separate module marked with #[cfg(test)]

// Example functions to test
pub fn add(a: i32, b: i32) -> i32 {
    a + b
}

pub fn divide(a: f64, b: f64) -> Result<f64, String> {
    if b == 0.0 {
        Err("Cannot divide by zero".to_string())
    } else {
        Ok(a / b)
    }
}

pub fn is_even(n: i32) -> bool {
    n % 2 == 0
}

pub struct Calculator {
    pub result: i32,
}

impl Calculator {
    pub fn new() -> Self {
        Calculator { result: 0 }
    }

    pub fn add(&mut self, value: i32) -> &mut Self {
        self.result += value;
        self
    }

    pub fn subtract(&mut self, value: i32) -> &mut Self {
        self.result -= value;
        self
    }

    pub fn multiply(&mut self, value: i32) -> &mut Self {
        self.result *= value;
        self
    }
}

// Unit tests are typically placed in the same file, inside a #[cfg(test)] module
#[cfg(test)]
mod tests {
    // Import everything from the parent module to test
    use super::*;

    #[test]
    fn test_add() {
        assert_eq!(add(2, 3), 5);
        assert_eq!(add(-1, 1), 0);
        assert_eq!(add(0, 0), 0);
    }

    #[test]
    fn test_divide_success() {
        let result = divide(10.0, 2.0).unwrap();
        assert_eq!(result, 5.0);
    }

    #[test]
    fn test_divide_by_zero() {
        let result = divide(10.0, 0.0);
        assert!(result.is_err());
        assert_eq!(result.unwrap_err(), "Cannot divide by zero");
    }

    #[test]
    fn test_is_even() {
        assert!(is_even(2));
        assert!(is_even(0));
        assert!(is_even(-2));
        assert!(!is_even(1));
        assert!(!is_even(-1));
        assert!(!is_even(3));
    }

    #[test]
    fn test_calculator() {
        let mut calc = Calculator::new();
        calc.add(5).subtract(2).multiply(3);
        assert_eq!(calc.result, 9);
    }

    // Using assert_eq! and assert_ne! macros
    #[test]
    fn test_assert_eq_ne() {
        let a = 5;
        let b = 5;
        assert_eq!(a, b); // Passes if a == b

        let c = 10;
        assert_ne!(a, c); // Passes if a != c
    }

    // Testing panics with should_panic
    #[test]
    #[should_panic]
    fn test_should_panic() {
        panic!("This test should panic!");
    }

    // Testing panics with expected message
    #[test]
    #[should_panic(expected = "Custom panic message")]
    fn test_should_panic_with_message() {
        panic!("Custom panic message");
    }

    // Using custom test names with underscores for readability
    #[test]
    fn test_calculator_chaining_operations() {
        let mut calc = Calculator::new();
        calc.add(10).subtract(3).multiply(2);
        assert_eq!(calc.result, 14);
    }

    // Testing with custom assertions
    #[test]
    fn test_with_custom_assertion_message() {
        let result = add(2, 2);
        assert_eq!(result, 4, "Expected 2 + 2 = 4, but got {}", result);
    }

    // Testing floating point numbers (with tolerance)
    #[test]
    fn test_floating_point_comparison() {
        let result: f64 = 0.1 + 0.2;
        let expected: f64 = 0.3;
        // For floating point comparisons, use assert! with a tolerance
        assert!(
            (result - expected).abs() < 1e-10,
            "Expected {} but got {}",
            expected,
            result
        );
    }

    // Example of a test that ignores (skips) execution
    #[test]
    #[ignore]
    fn test_ignored_example() {
        // This test will be skipped unless explicitly run with `cargo test -- --ignored`
        assert_eq!(add(100, 200), 300);
    }
}

// Integration tests are typically placed in the tests/ directory at the project root
// But for demonstration purposes, here's how you might structure them:

#[cfg(test)]
mod calculator_tests {
    use super::*;

    #[test]
    fn test_calculator_operations() {
        let mut calc = Calculator::new();

        // Test addition
        calc.add(5);
        assert_eq!(calc.result, 5);

        // Test subtraction
        calc.subtract(3);
        assert_eq!(calc.result, 2);

        // Test multiplication
        calc.multiply(4);
        assert_eq!(calc.result, 8);
    }

    #[test]
    fn test_calculator_method_chaining() {
        let mut calc = Calculator::new();
        calc.add(10).subtract(5).multiply(2).add(1);
        assert_eq!(calc.result, 11);
    }
}

// Example of parameterized tests (manual approach since Rust doesn't have built-in parameterized tests)
#[cfg(test)]
mod parameterized_tests {
    use super::*;

    #[test]
    fn test_add_with_multiple_inputs() {
        let test_cases = vec![(2, 3, 5), (0, 0, 0), (-1, 1, 0), (10, -5, 5), (-3, -7, -10)];

        for (a, b, expected) in test_cases {
            assert_eq!(add(a, b), expected, "Failed for add({}, {})", a, b);
        }
    }
}

// Demonstration of setup/teardown patterns (though Rust doesn't have built-in setup/teardown)
// You can create helper functions for setup
#[cfg(test)]
mod setup_teardown_demo {
    use super::*;

    fn setup_calculator() -> Calculator {
        println!("Setting up calculator for test...");
        Calculator::new()
    }

    #[test]
    fn test_with_setup() {
        let mut calc = setup_calculator();
        calc.add(10);
        assert_eq!(calc.result, 10);
    }
}

// To run these tests, use:
// cargo test                    # Run all tests
// cargo test test_add          # Run tests matching "test_add"
// cargo test -- --ignored      # Run ignored tests
// cargo test -- --nocapture    # Show output for tests

