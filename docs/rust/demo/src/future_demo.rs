use std::future::Future;
use std::pin::Pin;
use std::sync::{Arc, Condvar, Mutex};
use std::task::{Context, Poll, Wake, Waker};

struct ParkState {
    ready: Mutex<bool>,
    changed: Condvar,
}

struct ThreadWaker {
    state: Arc<ParkState>,
}

impl Wake for ThreadWaker {
    fn wake(self: Arc<Self>) {
        self.wake_by_ref();
    }

    fn wake_by_ref(self: &Arc<Self>) {
        let mut ready = self.state.ready.lock().unwrap();
        *ready = true;
        self.state.changed.notify_one();
    }
}

fn block_on<F: Future>(future: F) -> F::Output {
    let state = Arc::new(ParkState {
        ready: Mutex::new(false),
        changed: Condvar::new(),
    });
    let waker = Waker::from(Arc::new(ThreadWaker {
        state: Arc::clone(&state),
    }));
    let mut context = Context::from_waker(&waker);
    let mut future = Box::pin(future);

    loop {
        match Future::poll(Pin::as_mut(&mut future), &mut context) {
            Poll::Ready(value) => return value,
            Poll::Pending => {
                let mut ready = state.ready.lock().unwrap();
                while !*ready {
                    ready = state.changed.wait(ready).unwrap();
                }
                *ready = false;
            }
        }
    }
}

struct Countdown {
    remaining: u8,
}

impl Countdown {
    fn new(remaining: u8) -> Self {
        Self { remaining }
    }
}

impl Future for Countdown {
    type Output = &'static str;

    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        if self.remaining == 0 {
            println!("Countdown is ready");
            return Poll::Ready("done");
        }

        println!("Countdown pending: remaining = {}", self.remaining);
        self.remaining -= 1;
        cx.waker().wake_by_ref();
        Poll::Pending
    }
}

async fn build_message(name: &str) -> String {
    let status = Countdown::new(3).await;
    format!("async result for {name}: {status}")
}

pub fn run_all() {
    println!("=== Future trait 演示 ===");
    let mut future = Countdown::new(1);
    let waker = Waker::noop();
    let mut context = Context::from_waker(waker);

    println!("第一次 poll:");
    let first = Future::poll(Pin::new(&mut future), &mut context);
    println!("poll result = {:?}", first);

    println!("第二次 poll:");
    let second = Future::poll(Pin::new(&mut future), &mut context);
    println!("poll result = {:?}", second);

    println!("\n=== async/await + block_on 演示 ===");
    let message = block_on(build_message("rust future"));
    println!("{message}");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn countdown_returns_ready_after_pending_steps() {
        let result = block_on(Countdown::new(2));
        assert_eq!(result, "done");
    }

    #[test]
    fn async_function_can_await_custom_future() {
        let message = block_on(build_message("test"));
        assert_eq!(message, "async result for test: done");
    }
}
