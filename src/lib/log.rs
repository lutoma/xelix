/* log.rs: Kernel logger integration
 * Copyright © 2010-2023 Lukas Martini
 *
 * This file is part of Xelix.
 *
 * Xelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

use lazy_static::lazy_static;
use spin::Mutex;
use core::fmt;

extern "C" {
	fn log(level: u32, input: *const u8, ...) -> i32;
	fn _panic(input: *const u8, ...) -> !;
}

pub struct Writer {
}

lazy_static! {
	pub static ref WRITER: Mutex<Writer> = Mutex::new(Writer {
	});
}

impl fmt::Write for Writer {
	fn write_str(&mut self, s: &str) -> fmt::Result {
		unsafe {
			log(1, s.as_ptr());
		}

		Ok(())
	}
}

#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => ($crate::_print(format_args!($($arg)*)));
}

#[macro_export]
macro_rules! println {
    () => ($crate::print!("\n"));
    ($($arg:tt)*) => ($crate::print!("{}\n", format_args!($($arg)*)));
}


#[doc(hidden)]
pub fn _print(args: fmt::Arguments) {
    use core::fmt::Write;
    WRITER.lock().write_fmt(args).unwrap();
}
