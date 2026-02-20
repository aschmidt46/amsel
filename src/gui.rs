
pub struct Scene {
    value: u32,
    width: u32,
    height: u32,
    // img: image::Image<Handle>,
    bytes: Option<Bytes>,
}

#[derive(Debug, Clone, Copy)]
pub enum Message {
    Increment,
    Decrement,
}

use std::cell::RefCell;
use std::ops::Deref;
use std::rc::Rc;
use std::sync::{Arc, Mutex};

use bytes::{Bytes, BytesMut};
use iced::Length::Fill;
use iced::widget::{button, column, text, Column};
use iced::widget::image;
use iced::advanced::image::Handle;

use crate::FRAMEBUFFER;

impl Default for Scene{
    fn default() -> Scene{
        Scene::new(160,144)
    }
}

impl Scene {
    pub fn new(width: u32, height: u32) -> Self {
        Scene {
            value: 0,
            width,
            height,
            bytes: None
        }
    }
    pub fn view(&self) -> Column<'_, Message> {
        match &self.bytes{
            None => column![button("increment").on_press(Message::Increment)],
            Some(m) => {
                column![
                    image(Handle::from_rgba(self.width, self.height, m.clone()))
                    .height(Fill).width(Fill).filter_method(image::FilterMethod::Nearest),
                    button("increment").on_press(Message::Increment)
                ]
            }
        }
    }

    pub fn update(&mut self, message: Message) {
        match message {
            Message::Increment => {
                self.value += 100;
                match FRAMEBUFFER.lock(){
                    Err(e) => panic!("Konnte Mutex in GUI nicht sperren: {}", e),
                    Ok(m) => {
                        self.bytes = Some(Bytes::from_owner(m.clone()));
                    }
                }
            }
            Message::Decrement => {
                self.value -= 1;
            }
        }
    }
}
