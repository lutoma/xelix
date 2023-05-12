#![no_std]
#![no_main]

pub mod lib;

use core::panic::PanicInfo;
pub(crate) use lib::log::_print;

extern "C" {
	fn _panic(input: *const u8, ...) -> !;
}

/*
#[no_mangle]
pub extern "C" fn rust_function() {
	println!("Hello World{}\n\0", "!");
}
*/

#[panic_handler]
fn panic(info: &PanicInfo<'_>) -> ! {
	unsafe {
		println!("{}", info);
		_panic("Rust panic\n\0".as_ptr());
	}
}
