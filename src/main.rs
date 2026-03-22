#![allow(dead_code)]
#![allow(unused_variables)]

use std::{sync::Mutex, thread};



use iced::Subscription;

use crate::gui::{Message, Scene};

mod gbc;
mod gui;

const N: usize = 160 * 144 * 4;
static FRAMEBUFFER: Mutex<[u8; N]> = Mutex::new([255; N]);

pub fn index_framebuffer(x: usize, y: usize) -> usize{
    (x + 160 * y) * 4 + 0 // Kanal 0 (rot) angenommen
}

fn main() -> iced::Result {
    iced::application(Scene::default, Scene::update, Scene::view).subscription(Scene::subscription).run()
}
