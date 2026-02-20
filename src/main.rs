#![allow(dead_code)]
#![allow(unused_variables)]

use std::{sync::Mutex, thread};

use crate::gui::Scene;

mod gbc;
mod gui;

const N: usize = 160 * 144 * 4;
static FRAMEBUFFER: Mutex<[u8; N]> = Mutex::new([255; N]);

fn main() -> iced::Result {
    // Test um Farbe zu ändern
    thread::spawn(move || {
        loop {
            for y in 0..144 {
                for x in 0..160 {
                    for c in 0..3 {
                        match FRAMEBUFFER.lock().as_mut() {
                            Err(e) => panic!("Konnte nicht sperren!"),
                            Ok(m) => {
                                m[(y + 144 * x) * 4 + c] = m[(y + 144 * x) * 4 + c].wrapping_add(1);
                            }
                        };
                    }
                }
            }
        }
    });
    iced::run(Scene::update, Scene::view)
}
