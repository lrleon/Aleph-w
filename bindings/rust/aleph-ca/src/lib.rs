//! Safe, idiomatic Rust wrapper over the Aleph-w cellular-automata C ABI.
//!
//! ```no_run
//! use aleph_ca::GameOfLife;
//!
//! let mut gol = GameOfLife::new(5, 5).unwrap();
//! gol.set(2, 1, true).unwrap();
//! gol.set(2, 2, true).unwrap();
//! gol.set(2, 3, true).unwrap();
//! gol.step().unwrap();
//! assert!(gol.get(1, 2).unwrap()); // blinker turned vertical
//! ```
//!
//! Experimental: the underlying ABI is stable within its `_v1` major but the
//! Rust surface may still change before 1.0.

use std::ffi::CStr;
use std::fmt;

use aleph_ca_sys as sys;

/// An error returned by the C ABI, carrying its status code and message.
#[derive(Debug, Clone)]
pub struct Error {
    pub code: i32,
    pub message: String,
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "aleph-ca error {}: {}", self.code, self.message)
    }
}

impl std::error::Error for Error {}

/// Result alias for fallible C-ABI calls.
pub type Result<T> = std::result::Result<T, Error>;

fn last_error(code: i32) -> Error {
    // SAFETY: the pointer is a valid, NUL-terminated, thread-local C string.
    let message = unsafe {
        let p = sys::aleph_ca_error_message_v1();
        if p.is_null() {
            String::new()
        } else {
            CStr::from_ptr(p).to_string_lossy().into_owned()
        }
    };
    Error { code, message }
}

fn check(code: i32) -> Result<()> {
    if code == sys::ALEPH_CA_OK {
        Ok(())
    } else {
        Err(last_error(code))
    }
}

/// Packed ABI version exposed by the linked library: `(major << 16) | minor`.
pub fn abi_version() -> u32 {
    // SAFETY: no arguments, no state.
    unsafe { sys::aleph_ca_api_version_v1() }
}

/// A Conway Game-of-Life engine on a toroidal grid.
pub struct GameOfLife {
    raw: *mut sys::aleph_ca_engine,
}

impl GameOfLife {
    /// Create a `rows x cols` engine, initially all-dead.
    pub fn new(rows: usize, cols: usize) -> Result<Self> {
        // SAFETY: FFI call; result is checked for null.
        let raw = unsafe { sys::aleph_ca_create_gol_engine_v1(rows, cols) };
        if raw.is_null() {
            // SAFETY: no arguments, reads thread-local error state.
            Err(last_error(unsafe { sys::aleph_ca_last_error_v1() }))
        } else {
            Ok(GameOfLife { raw })
        }
    }

    /// Grid extents `(rows, cols)`.
    pub fn extents(&self) -> Result<(usize, usize)> {
        let mut rows = 0usize;
        let mut cols = 0usize;
        // SAFETY: valid handle and out-pointers.
        let rc = unsafe { sys::aleph_ca_extents_v1(self.raw, &mut rows, &mut cols) };
        check(rc).map(|_| (rows, cols))
    }

    /// Set a cell alive/dead.
    pub fn set(&mut self, row: usize, col: usize, alive: bool) -> Result<()> {
        // SAFETY: valid handle.
        let rc = unsafe {
            sys::aleph_ca_set_cell_v1(self.raw, row, col, if alive { 1 } else { 0 })
        };
        check(rc)
    }

    /// Read a cell.
    pub fn get(&self, row: usize, col: usize) -> Result<bool> {
        let mut v: i32 = 0;
        // SAFETY: valid handle and out-pointer.
        let rc = unsafe { sys::aleph_ca_get_cell_v1(self.raw, row, col, &mut v) };
        check(rc).map(|_| v != 0)
    }

    /// Advance one generation.
    pub fn step(&mut self) -> Result<()> {
        // SAFETY: valid handle.
        check(unsafe { sys::aleph_ca_step_v1(self.raw) })
    }

    /// Advance `steps` generations.
    pub fn run(&mut self, steps: usize) -> Result<()> {
        // SAFETY: valid handle.
        check(unsafe { sys::aleph_ca_run_v1(self.raw, steps) })
    }

    /// Number of completed generations.
    pub fn steps_run(&self) -> usize {
        // SAFETY: valid handle.
        unsafe { sys::aleph_ca_steps_run_v1(self.raw) }
    }

    /// Copy the current frame, row-major, as a `Vec<i32>` of length rows*cols.
    pub fn frame(&self) -> Result<Vec<i32>> {
        let (rows, cols) = self.extents()?;
        let mut out = vec![0i32; rows * cols];
        // SAFETY: out has exactly rows*cols elements.
        let rc =
            unsafe { sys::aleph_ca_copy_frame_v1(self.raw, out.as_mut_ptr(), out.len()) };
        check(rc).map(|_| out)
    }
}

impl Drop for GameOfLife {
    fn drop(&mut self) {
        // SAFETY: raw was produced by create and is destroyed exactly once.
        unsafe { sys::aleph_ca_destroy_engine_v1(self.raw) };
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn abi_major_is_one() {
        assert_eq!(abi_version() >> 16, 1);
    }

    #[test]
    fn blinker_oscillates() {
        let mut gol = GameOfLife::new(5, 5).unwrap();
        assert_eq!(gol.extents().unwrap(), (5, 5));

        for c in 1..=3 {
            gol.set(2, c, true).unwrap();
        }

        gol.step().unwrap();
        assert_eq!(gol.steps_run(), 1);
        // Vertical now.
        assert!(gol.get(1, 2).unwrap());
        assert!(gol.get(2, 2).unwrap());
        assert!(gol.get(3, 2).unwrap());
        assert!(!gol.get(2, 1).unwrap());

        gol.step().unwrap();
        // Horizontal again (period 2).
        assert!(gol.get(2, 1).unwrap());
        assert!(gol.get(2, 3).unwrap());

        let live = gol.frame().unwrap().iter().filter(|&&v| v != 0).count();
        assert_eq!(live, 3);
    }

    #[test]
    fn out_of_range_is_error() {
        let mut gol = GameOfLife::new(4, 4).unwrap();
        assert!(gol.set(99, 0, true).is_err());
    }
}