#![allow(dead_code)]
#![allow(unused_variables)]

use std::{sync::Mutex, thread};

use eframe::egui;



use egui::{AtomExt, TextureOptions, Vec2};
use iced::Subscription;

use crate::{gbc::CGB, gui::{Message, Scene}};

mod gbc;
mod gui;

const N: usize = 160 * 144 * 4;
static FRAMEBUFFER: Mutex<[u8; N]> = Mutex::new([255; N]);

pub fn index_framebuffer(x: usize, y: usize) -> usize{
    (x + 160 * y) * 4 + 0 // Kanal 0 (rot) angenommen
}

// fn main() -> eframe::Result {
//     env_logger::init(); // Log to stderr (if you run with `RUST_LOG=debug`).
//     let options = eframe::NativeOptions {
//         viewport: egui::ViewportBuilder::default().with_inner_size([320.0, 240.0]),
//         ..Default::default()
//     };
//     eframe::run_native(
//         "My egui App",
//         options,
//         Box::new(|cc| {
//             // This gives us image support:
//             egui_extras::install_image_loaders(&cc.egui_ctx);

//             Ok(Box::<MyApp>::default())
//         }),
//     )
// }
fn main() -> iced::Result {
    iced::application(Scene::default, Scene::update, Scene::view).subscription(Scene::subscription).run()
}

struct MyApp {
    texture: Option<egui::TextureHandle>,
    cgb: CGB,
    br: bool,
}

impl Default for MyApp {
    fn default() -> Self {
        Self {
            texture: None,
            cgb: CGB::new("resources/cgb-acid2.gbc"),
            br: false,
        }
    }
}

impl eframe::App for MyApp {
    fn update(&mut self, ui: &egui::Context, _frame: &mut eframe::Frame) {
        for i in 0..70224{
            self.cgb.clock();
        }
        self.cgb.has_frame = false;
        egui::CentralPanel::default().show(ui, |ui| {

            if ui.checkbox(&mut self.br,  "Break").clicked(){
                self.cgb.bus.borrow_mut().break_exec();
            }

            if ui.button("Step").clicked(){
                self.cgb.bus.borrow_mut().step();
            }
            let (line, pc) = self.cgb.bus.borrow_mut().get_instruction_offset(0);
            let pc_count = format!("{:#06x}:   ", pc);
            ui.heading([pc_count,line].concat());

            match FRAMEBUFFER.lock().as_mut(){
                Ok(m) => {
                    let colimg = egui::ColorImage::from_rgba_unmultiplied([160,144], m.as_mut());
                    let texture: &mut egui::TextureHandle = self.texture.get_or_insert_with(|| {
                        // Load the texture only once.
                        ui.ctx().load_texture(
                            "fb",
                            colimg.clone(),
                            TextureOptions::NEAREST
                        )
                    });
                    texture.set(colimg, TextureOptions::NEAREST);
                    ui.add(
                        egui::Image::from_texture(egui::load::SizedTexture::new(texture.id(), texture.size_vec2())).fit_to_fraction(Vec2::new(1.0, 1.0))
                    )
                },
                Err(e) => panic!("Konnte nicht sperren"),
            };
            ui.ctx().request_repaint();
        });
    }
}
