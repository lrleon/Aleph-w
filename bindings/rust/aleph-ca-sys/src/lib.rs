//! Raw FFI bindings to the Aleph-w cellular-automata C ABI (`ca-c-api.h`).
//!
//! These declarations mirror the `aleph_ca_*_v1` entry points exactly. They are
//! `unsafe` to call; prefer the safe wrapper in the `aleph-ca` crate.
//!
//! Hand-written rather than `bindgen`-generated to avoid a libclang build
//! dependency; the surface is tiny and stable (every symbol is `_v1`).
#![allow(non_camel_case_types)]

use std::os::raw::{c_char, c_int};

/// Status codes returned by the integer-returning entry points.
pub const ALEPH_CA_OK: c_int = 0;
pub const ALEPH_CA_ERR_NULL: c_int = 1;
pub const ALEPH_CA_ERR_RANGE: c_int = 2;
pub const ALEPH_CA_ERR_ALLOC: c_int = 3;
pub const ALEPH_CA_ERR_INTERNAL: c_int = 4;

/// Opaque engine handle.
#[repr(C)]
pub struct aleph_ca_engine {
    _private: [u8; 0],
}

extern "C" {
    pub fn aleph_ca_api_version_v1() -> u32;
    pub fn aleph_ca_last_error_v1() -> c_int;
    pub fn aleph_ca_error_message_v1() -> *const c_char;

    pub fn aleph_ca_create_gol_engine_v1(rows: usize, cols: usize) -> *mut aleph_ca_engine;
    pub fn aleph_ca_destroy_engine_v1(engine: *mut aleph_ca_engine);

    pub fn aleph_ca_extents_v1(
        engine: *const aleph_ca_engine,
        rows: *mut usize,
        cols: *mut usize,
    ) -> c_int;
    pub fn aleph_ca_set_cell_v1(
        engine: *mut aleph_ca_engine,
        row: usize,
        col: usize,
        value: i32,
    ) -> c_int;
    pub fn aleph_ca_get_cell_v1(
        engine: *const aleph_ca_engine,
        row: usize,
        col: usize,
        out: *mut i32,
    ) -> c_int;
    pub fn aleph_ca_step_v1(engine: *mut aleph_ca_engine) -> c_int;
    pub fn aleph_ca_run_v1(engine: *mut aleph_ca_engine, steps: usize) -> c_int;
    pub fn aleph_ca_copy_frame_v1(
        engine: *const aleph_ca_engine,
        out: *mut i32,
        out_len: usize,
    ) -> c_int;
    pub fn aleph_ca_steps_run_v1(engine: *const aleph_ca_engine) -> usize;
}