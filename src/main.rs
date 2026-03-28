#![allow(dead_code)]
#![allow(unused_variables)]

use std::{cell::RefCell, ops::Deref, rc::Rc, sync::{Arc, Mutex, atomic::{AtomicU8, Ordering}}, thread, time::{SystemTime, UNIX_EPOCH}};

use cpal::{self, Device, FromSample, StreamConfig, traits::{DeviceTrait, HostTrait, StreamTrait}};
use cpal::SizedSample;
use eframe::egui;



use egui::{AtomExt, Color32, Key, TextureOptions, Vec2};
use rfd::FileDialog;

use crate::{gbc::CGB};

mod gbc;

use arr_macro::arr;

const N: usize = 160 * 144 * 4;
static FRAMEBUFFER: [AtomicU8; N] = arr![AtomicU8::new(255); 92160];

pub fn index_framebuffer(x: usize, y: usize) -> usize{
    (x + 160 * y) * 4 + 0 // Kanal 0 (rot) angenommen
}

fn e_main(cgb: Arc<Mutex<CGB>>) -> eframe::Result {
    env_logger::init(); // Log to stderr (if you run with `RUST_LOG=debug`).
    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default().with_inner_size([320.0, 240.0]),
        ..Default::default()
    };
    eframe::run_native(
        "Anton's CGB Emulator",
        options,
        Box::new(|cc| {

            Ok(Box::<MyApp>::from(MyApp::new(cgb)))
        }),
    )
}

fn main(){

    let cgb = Arc::new(Mutex::new(CGB::new("./resources/pokemonkristall.gbc")));
    let cgb2 = cgb.clone();

    let host = cpal::default_host();
    let device = host.default_output_device().unwrap();

    let mut supported_configs_range = device.supported_output_configs()
        .expect("error while querying configs");
    supported_configs_range.clone().for_each(|c| println!("{:?}", c));
    let mut supported_config = supported_configs_range.next()
        .expect("no supported config?!")
        .with_max_sample_rate();
    while supported_config.sample_rate() < 20000 {
        supported_config = supported_configs_range.next()
        .expect("no supported config?!")
        .with_max_sample_rate();
    }

    let mut config: StreamConfig = supported_config.into();
    config.buffer_size = cpal::BufferSize::Fixed(512);

    run_audio::<f32>(device, config, cgb);

    let app = e_main(cgb2);
}

fn run_audio<T: SizedSample + FromSample<f64>>(
    device: Device,
    config: StreamConfig,
    cgb: Arc<Mutex<CGB>>
) {
    std::thread::spawn(move || {
        let sample_rate = config.sample_rate as f64;
        println!("{sample_rate}");
        let mut next_value = move ||  {
            match cgb.lock(){
                Ok(mut m) => {
                    m.set_sample_rate(sample_rate);
                    while !m.audio_sample_ready(){
                        m.clock();
                    }
                    m.get_stereo()
                },
                Err(e) => panic!("Was"),
            }
        };

        let channels = config.channels as usize;
        let err_fn = |err| eprintln!("Error auf Stream {err}");
        let stream = device
            .build_output_stream(
                &config,
                move |data: &mut [T], _: &cpal::OutputCallbackInfo| {
                    write_data(data, channels, &mut next_value)
                },
                err_fn,
                None
            )
            .unwrap();

        stream.play().unwrap();
        loop {
            std::thread::sleep(std::time::Duration::from_millis(1));
        }
    });
}

fn write_data<T: SizedSample + FromSample<f64>>(
    output: &mut [T],
    channels: usize,
    next_sample: &mut dyn FnMut() -> (f64, f64),
) {
    for frame in output.chunks_mut(channels) {
        let sample = next_sample();
        let left = T::from_sample(sample.0);
        let right = T::from_sample(sample.1);

        for (channel, sample) in frame.iter_mut().enumerate() {
            *sample = if channel & 1 == 0 { left } else { right };
        }
    }
}

struct MyApp {
    texture: Option<egui::TextureHandle>,
    cgb: Arc<Mutex<CGB>>,
    br: bool,
}

impl MyApp {
    fn new(cgb: Arc<Mutex<CGB>>) -> Self {
        Self {
            texture: None,
            cgb,
            br: false,
        }
    }
    fn add_key(&mut self, key: egui::Key){
        match self.cgb.lock(){
            Ok(m) => {
                match key{
                    Key::Enter => m.bus.borrow_mut().press_button(3),
                    Key::Backspace => m.bus.borrow_mut().press_button(2),
                    Key::A => m.bus.borrow_mut().press_button(1),
                    Key::S => m.bus.borrow_mut().press_button(0),
                    Key::ArrowDown => m.bus.borrow_mut().press_joypad(3),
                    Key::ArrowUp => m.bus.borrow_mut().press_joypad(2),
                    Key::ArrowLeft => m.bus.borrow_mut().press_joypad(1),
                    Key::ArrowRight => m.bus.borrow_mut().press_joypad(0),
                    _ => ()
                }
            },
            Err(e) => panic!("Lock fehler: {}", e),
        }
    }
    fn remove_key(&mut self, key: egui::Key){
        match self.cgb.lock(){
            Ok(m) => {
                match key{
                    Key::Enter => m.bus.borrow_mut().release_button(3),
                    Key::Backspace => m.bus.borrow_mut().release_button(2),
                    Key::A => m.bus.borrow_mut().release_button(1),
                    Key::S => m.bus.borrow_mut().release_button(0),
                    Key::ArrowDown => m.bus.borrow_mut().release_joypad(3),
                    Key::ArrowUp => m.bus.borrow_mut().release_joypad(2),
                    Key::ArrowLeft => m.bus.borrow_mut().release_joypad(1),
                    Key::ArrowRight => m.bus.borrow_mut().release_joypad(0),
                    _ => ()
                }
            },
            Err(e) => panic!("Lock fehler: {}", e),
        }
    }
}

impl eframe::App for MyApp {
    fn raw_input_hook(&mut self, _ctx: &egui::Context, _raw_input: &mut egui::RawInput) {
        for event in _raw_input.events.clone(){
            match event{
                egui::Event::Key { key, physical_key, pressed, repeat, modifiers } => {
                    if repeat {continue;}
                    match physical_key{
                        Some(key) => {
                            if pressed {self.add_key(key)} else {self.remove_key(key)};
                        },  
                        None => (),
                    }
                },
                _ => (),
            }
        }
    }
    fn update(&mut self, ui: &egui::Context, _frame: &mut eframe::Frame) {
        // m.has_frame = false;
        egui::CentralPanel::default().show(ui, |ui| {

            // if ui.checkbox(&mut self.br,  "Break").clicked(){
            //     m.bus.borrow_mut().break_exec();
            // }

            // if ui.button("Step").clicked(){
            //     m.bus.borrow_mut().step();
            // }
            // let (line, pc) = m.bus.borrow_mut().get_instruction_offset(0);
            // let pc_count = format!("{:#06x}:   ", pc);
            // ui.heading([pc_count,line].concat());

            if ui.button("Laden").clicked(){
                let file = FileDialog::new().set_directory(".").pick_file();
                match file{
                    Some(f) => {
                        let str = f.to_str();
                        match str{
                            Some(s) => {
                                match self.cgb.lock(){
                                    Ok(mut m) => {
                                        *m = CGB::new(s);
                                    },
                                    Err(e) => panic!("Lock fehler: {}", e),
                                }
                            },
                            None => panic!("Wat 2"),
                        }
                    },
                    None => panic!("Keine Selektion"),
                }
            }

            // match FRAMEBUFFER.lock().as_mut(){
                // Ok(m) => {
                unsafe{
                    // let colimg = egui::ColorImage::from_rgba_unmultiplied([160,144], FRAMEBUFFER.clone().map(|v| v.as_ptr().read()).as_ref());
                    let mut colimg = egui::ColorImage::filled([160,144], Color32::WHITE);
                    let iterable = colimg.as_raw_mut();
                    for i in 0..160*144*4 {
                        iterable[i] = FRAMEBUFFER[i].as_ptr().read();
                    }
                    let texture: &mut egui::TextureHandle = self.texture.get_or_insert_with(|| {
                        // Load the texture only once.
                        ui.ctx().load_texture(
                            "fb",
                            colimg.clone(),
                            TextureOptions::NEAREST
                        )
                    });
                    texture.set(colimg, TextureOptions::NEAREST);
                    ui.centered_and_justified(|ui|
                        ui.add(
                            egui::Image::from_texture(egui::load::SizedTexture::new(texture.id(), texture.size_vec2())).fit_to_fraction(Vec2::new(1.0, 1.0))
                        )
                    );
                }
                // },
                // Err(e) => panic!("Konnte nicht sperren"),
            // };
            ui.ctx().request_repaint();
        });
    }
}
