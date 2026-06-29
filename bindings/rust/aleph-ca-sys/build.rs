// Phase 25 — link configuration for the Aleph-w C ABI shared library.
//
// Locates libalephca_c. By default it looks in `<repo>/build` relative to this
// crate; override with the ALEPH_CA_LIB_DIR environment variable.
use std::env;
use std::path::PathBuf;

fn main() {
    let lib_dir = env::var("ALEPH_CA_LIB_DIR").unwrap_or_else(|_| {
        let manifest = env::var("CARGO_MANIFEST_DIR").unwrap();
        // bindings/rust/aleph-ca-sys -> repo root is three levels up.
        let mut p = PathBuf::from(manifest);
        p.push("..");
        p.push("..");
        p.push("..");
        p.push("build");
        p.to_string_lossy().into_owned()
    });

    println!("cargo:rustc-link-search=native={lib_dir}");
    println!("cargo:rustc-link-lib=dylib=alephca_c");
    // Re-run if the override changes.
    println!("cargo:rerun-if-env-changed=ALEPH_CA_LIB_DIR");
}