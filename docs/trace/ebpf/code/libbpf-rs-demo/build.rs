use std::env;
use std::path::PathBuf;

use libbpf_cargo::SkeletonBuilder;

const BPF_SOURCE: &str = "src/bpf/trace.bpf.c";

fn main() {
    let manifest_dir =
        PathBuf::from(env::var_os("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR must be set"));
    let skeleton = manifest_dir.join("src/bpf/trace.skel.rs");

    let mut builder = SkeletonBuilder::new();
    builder.source(BPF_SOURCE);
    if let Some(clang) = env::var_os("CLANG") {
        builder.clang(clang);
    }
    builder
        .build_and_generate(&skeleton)
        .expect("failed to compile BPF program and generate skeleton");

    println!("cargo:rerun-if-changed={BPF_SOURCE}");
    println!("cargo:rerun-if-changed=src/bpf/trace.h");
}
