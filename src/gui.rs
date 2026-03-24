
use std::cell::RefCell;
use std::ops::Deref;
use std::rc::Rc;
use std::sync::{Arc, Mutex};

use bytes::{Bytes, BytesMut};
use iced::Length::Fill;
use iced::futures::StreamExt;
use iced::futures::channel::mpsc::{self, Sender};
use iced::keyboard;
use iced::widget::{Column, button, checkbox, column, container, row, text};
use iced::widget::image;
use iced::advanced::image::Handle;
use iced::{Subscription, futures::{SinkExt, Stream}, stream};

use crate::gbc::CGB;
use crate::gbc::sm83::{Opcode, OperandType};
use crate::{FRAMEBUFFER};

#[derive(Clone)]
pub struct CpuState{
    current_line: String,
    pc: u16
}

pub struct Scene {
    width: u32,
    height: u32,
    show_debugger: bool,
    // img: image::Image<Handle>,
    bytes: Option<Bytes>,
    sender: Option<Sender<InternalMessage>>,

    // Debugger State
    is_break: bool,
    cpu_state: Option<CpuState>,
}

#[derive(Debug, Clone)]
pub enum Message {
    UpdateScreen,
    KeyEvent(keyboard::Event),
    Ready(Sender<InternalMessage>),
    ToggleDebugger,
    ToggleBreak,
    Step,
    UpdateCurrentOperation(String, u16)
}

pub enum InternalMessage {
    PressJoypad(usize),
    ReleaseJoypad(usize),
    PressButton(usize),
    ReleaseButton(usize),
    ToggleBreak,
    Step,
}

fn worker() -> impl Stream<Item = Message>{
       stream::channel(100, async |mut output| {
           // Create channel
           let (sender, mut receiver) = mpsc::channel::<InternalMessage>(100);

           // Send the sender back to the application
           output.send(Message::Ready(sender)).await;
        
           let mut cgb = CGB::new("resources/linksawakening.gbc");

           loop {
                cgb.clock();
                if cgb.has_frame {
                    cgb.has_frame = false;
                    let (line, pc) = cgb.bus.borrow_mut().get_instruction_offset(0);
                    let _ = output.try_send(Message::UpdateCurrentOperation(line,pc));
                    match output.send(Message::UpdateScreen).await{
                        Ok(_) => (),
                        Err(e) => panic!("SendError: {}", e),
                    }
                }
                let input = receiver.try_recv();
                match input{
                    Ok(msg) => {
                        match msg{
                            InternalMessage::PressButton(b) => cgb.bus.borrow_mut().press_button(b),
                            InternalMessage::ReleaseButton(b) => cgb.bus.borrow_mut().release_button(b),
                            InternalMessage::PressJoypad(b) => cgb.bus.borrow_mut().press_joypad(b),
                            InternalMessage::ReleaseJoypad(b) => cgb.bus.borrow_mut().release_joypad(b),
                            InternalMessage::ToggleBreak => {
                                cgb.bus.borrow_mut().break_exec()
                            },
                            InternalMessage::Step => {
                                cgb.bus.borrow_mut().step();
                                cgb.clock();
                                let (line, pc) = cgb.bus.borrow_mut().get_instruction_offset(0);
                                match output.send(Message::UpdateCurrentOperation(line,pc)).await{
                                    Ok(_) => (),
                                    Err(e) => panic!("Konnte nicht senden!"),
                                };
                            },
                        }   
                    },
                    Err(e) => (),
                }
           }
       })
   }

impl Default for Scene{
    fn default() -> Scene{
        Scene::new(160,144)
    }
}

impl Scene {
    pub fn subscription(&self) -> Subscription<Message>{
        Subscription::batch(
        [Subscription::run(worker), keyboard::listen().map(|event| Message::KeyEvent(event))]
        )
    }
    pub fn new(width: u32, height: u32) -> Self {
        Scene {
            width,
            height,
            bytes: None,
            sender: None,
            show_debugger: false,
            is_break: false,
            cpu_state: None
        }
    }
    pub fn view(&self) -> Column<'_, Message> {
        match &self.bytes{
            None => self.draw_debugger(),
            Some(m) => {
                column![
                    self.draw_debugger(),
                    image(Handle::from_rgba(self.width, self.height, m.clone()))
                    .height(Fill).width(Fill).filter_method(image::FilterMethod::Nearest)
                ]
            }
        }
    }
    pub fn draw_debugger(&self) -> Column<'_, Message> {
        if !self.show_debugger {
            column![
                button(text!("Debugger")).on_press(Message::ToggleDebugger),
            ]
        }
        else {
            let state = self.cpu_state.clone().unwrap();
            let pc_count = format!("{:#06x}:   ", state.pc);
            let opcode = state.current_line.clone();

            column![
                button(text!("Debugger")).on_press(Message::ToggleDebugger),
                container(
                    column![
                        row![checkbox(self.is_break).on_toggle(|_| Message::ToggleBreak), text!("Break")],
                        button("Step").on_press(Message::Step),
                        text([pc_count, opcode].concat())
                    ]
                )
            ]
        }
    }

    pub fn update(&mut self, message: Message) {
        match message {
            Message::UpdateCurrentOperation(line, pc) => self.cpu_state = Some(CpuState {current_line: line, pc}),
            Message::ToggleBreak => {
                self.is_break = !self.is_break;
                self.sender.as_mut().unwrap().try_send(InternalMessage::ToggleBreak);
            }
            Message::Step => {
                match self.sender.as_mut().unwrap().try_send(InternalMessage::Step){
                    Ok(_) => (),
                    Err(e) => panic!("Kann nicht steppen"),
                }
            },
            Message::ToggleDebugger => self.show_debugger = !self.show_debugger,
            Message::Ready(sender) => self.sender = Some(sender),
            Message::UpdateScreen => {
                match FRAMEBUFFER.lock(){
                    Err(e) => panic!("Konnte Mutex in GUI nicht sperren: {}", e),
                    Ok(m) => {
                        self.bytes = Some(Bytes::from_owner(m.clone()));
                    }
                }
            },
            Message::KeyEvent(event) => {
                match &mut self.sender{
                    None => (),
                    Some(sender) => {
                        let result = match event{
                            keyboard::Event::KeyPressed { key, modified_key, physical_key, location, modifiers, text, repeat } => {
                                match physical_key{
                                    keyboard::key::Physical::Code(keyboard::key::Code::Enter) => Some(sender.try_send(InternalMessage::PressButton(3))),
                                    keyboard::key::Physical::Code(keyboard::key::Code::Backspace) => Some(sender.try_send(InternalMessage::PressButton(2))),
                                    keyboard::key::Physical::Code(keyboard::key::Code::KeyA) => Some(sender.try_send(InternalMessage::PressButton(1))),
                                    keyboard::key::Physical::Code(keyboard::key::Code::KeyS) => Some(sender.try_send(InternalMessage::PressButton(0))),
                                    keyboard::key::Physical::Code(keyboard::key::Code::ArrowDown) => Some(sender.try_send(InternalMessage::PressJoypad(3))),
                                    keyboard::key::Physical::Code(keyboard::key::Code::ArrowUp) => Some(sender.try_send(InternalMessage::PressJoypad(2))),
                                    keyboard::key::Physical::Code(keyboard::key::Code::ArrowLeft) => Some(sender.try_send(InternalMessage::PressJoypad(1))),
                                    keyboard::key::Physical::Code(keyboard::key::Code::ArrowRight) => Some(sender.try_send(InternalMessage::PressJoypad(0))),
                                    _ => None,
                                }
                            },
                            keyboard::Event::KeyReleased { key, modified_key, physical_key, location, modifiers } => {
                                match physical_key{
                                    keyboard::key::Physical::Code(keyboard::key::Code::Enter) => Some(sender.try_send(InternalMessage::ReleaseButton(3))),
                                    keyboard::key::Physical::Code(keyboard::key::Code::Backspace) => Some(sender.try_send(InternalMessage::ReleaseButton(2))),
                                    keyboard::key::Physical::Code(keyboard::key::Code::KeyA) => Some(sender.try_send(InternalMessage::ReleaseButton(1))),
                                    keyboard::key::Physical::Code(keyboard::key::Code::KeyS) => Some(sender.try_send(InternalMessage::ReleaseButton(0))),
                                    keyboard::key::Physical::Code(keyboard::key::Code::ArrowDown) => Some(sender.try_send(InternalMessage::ReleaseJoypad(3))),
                                    keyboard::key::Physical::Code(keyboard::key::Code::ArrowUp) => Some(sender.try_send(InternalMessage::ReleaseJoypad(2))),
                                    keyboard::key::Physical::Code(keyboard::key::Code::ArrowLeft) => Some(sender.try_send(InternalMessage::ReleaseJoypad(1))),
                                    keyboard::key::Physical::Code(keyboard::key::Code::ArrowRight) => Some(sender.try_send(InternalMessage::ReleaseJoypad(0))),
                                    _ => None,
                                }
                            },
                            _ => None,
                        };
                        match result{
                            None => (),
                            Some(r) => match r{
                                Ok(_) => (),
                                Err(e) => panic!("Konnte nicht senden! {}", e),
                            },
                        }
                    }
                }
            }
        }
    }
}
