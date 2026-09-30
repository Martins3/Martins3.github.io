use std::mem::MaybeUninit;
use std::time::Duration;

use anyhow::Result;
use clap::Parser;
use libbpf_rs::skel::{OpenSkel, Skel, SkelBuilder};
use libbpf_rs::RingBufferBuilder;
use plain::Plain;

mod trace {
    include!(concat!(
        env!("CARGO_MANIFEST_DIR"),
        "/src/bpf/trace.skel.rs"
    ));
}

use trace::*;

#[derive(Debug, Parser)]
#[command(about = "Trace write(2) calls with libbpf-rs")]
struct Arguments {
    /// Only report writes from this process ID; zero reports all processes.
    #[arg(long, default_value_t = 0)]
    pid: u32,

    /// Enable libbpf's loader diagnostics.
    #[arg(short, long)]
    verbose: bool,
}

unsafe impl Plain for trace::types::event {}

fn print_event(data: &[u8]) -> i32 {
    let event = match plain::from_bytes::<types::event>(data) {
        Ok(event) => event,
        Err(_) => return 0,
    };
    let comm_bytes: Vec<u8> = event.comm.iter().map(|byte| *byte as u8).collect();
    let comm = String::from_utf8_lossy(&comm_bytes);
    println!(
        "pid={:<8} comm={:<16} fd={:<3} bytes={}",
        event.pid,
        comm.trim_end_matches('\0'),
        event.fd,
        event.count
    );
    0
}

fn main() -> Result<()> {
    let args = Arguments::parse();
    let mut builder = TraceSkelBuilder::default();
    if args.verbose {
        builder.obj_builder.debug(true);
    }

    let mut open_object = MaybeUninit::uninit();
    let mut open_skel = builder.open(&mut open_object)?;
    open_skel
        .maps
        .rodata_data
        .as_deref_mut()
        .expect("rodata map is not memory mapped")
        .target_pid = args.pid;

    let mut skel = open_skel.load()?;
    skel.attach()?;

    let mut ringbuf_builder = RingBufferBuilder::new();
    ringbuf_builder.add(&skel.maps.events, print_event)?;
    let ringbuf = ringbuf_builder.build()?;

    println!("Tracing write(2) calls; press Ctrl-C to stop.");
    loop {
        ringbuf.poll(Duration::from_millis(100))?;
    }
}
