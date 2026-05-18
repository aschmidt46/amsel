#![allow(warnings)]
fn main() {
    cxx_build::bridge("src/bridge.rs")
        // .file("src/blobstore.cc")
        .compile("cxx-demo");

    println!("cargo:rerun-if-changed=src/blobstore.cc");
    println!("cargo:rerun-if-changed=include/blobstore.h");
}
