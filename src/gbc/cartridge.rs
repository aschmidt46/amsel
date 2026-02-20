

const LOGO_SIZE: usize = 48;
const NINTENDO_LOGO: [u8; LOGO_SIZE] = [
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
    0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
    0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
]; 

struct RomInfo{
    valid_logo: bool,
    title: String,
    manufacturer_code: String,
    licensee_code: String,
    destination_code: u8,
    old_licensee_code: u8,
    version_number: u8,
    header_checksum: u8,
    calculated_header_checksum: u8,
    global_checksum: u16,
}

pub struct RomObject{
    pub (crate) raw_data: Vec<u8>,
    info: RomInfo,
    cgb_flag: u8,
    sgb_flag: u8,
    cartridge_type: u8, // Muss noch geparsed werden
    rom_size: u8, // Muss noch geparsed werden
    ram_size: u8, // Muss noch geparsed werden
}

impl RomObject{
    pub fn new(path: &str) -> Result<Self, std::io::Error> {
        let bytes = std::fs::read(path)?;
        Ok(RomObject {info: RomObject::get_rom_info(&bytes), cgb_flag: bytes[0x0143].clone()
            , sgb_flag: bytes[0x0146].clone(), cartridge_type: bytes[0x0147].clone()
            , rom_size: bytes[0x0148].clone(), ram_size: bytes[0x0149].clone(), raw_data: bytes })
    }

    fn get_rom_info(bytes: &Vec<u8>) -> RomInfo{
        let logo = RomObject::logo_is_valid(&bytes);
        let title = std::str::from_utf8(&bytes[0x0133..0x013F]).unwrap_or("Fehler").to_string();
        let manufacturer_code = std::str::from_utf8(&bytes[0x013F..0x0143]).unwrap_or("Fehler").to_string();
        let licensee_code = std::str::from_utf8(&bytes[0x0144..0x0146]).unwrap_or("Fehler").to_string();
        let destination_code = bytes[0x014A].clone();
        let old_licensee_code = bytes[0x014B].clone();
        let version_number = bytes[0x014C].clone();
        let header_checksum = bytes[0x014D].clone();
        let mut checksum: u8 = 0;
        for addr in 0x0134.. 0x014D{
            checksum = checksum.wrapping_sub(bytes[addr]).wrapping_sub(1);
        }
        let global_checksum: u16 = ((bytes[0x014E].clone() as u16) << 8) | bytes[0x014F].clone() as u16;
        RomInfo { valid_logo: logo, title, manufacturer_code, licensee_code, destination_code, old_licensee_code,
            version_number, header_checksum, calculated_header_checksum: checksum, global_checksum }
    }

    fn logo_is_valid(bytes: &Vec<u8>) -> bool{
        let offset: usize = 0x0104;
        let mut res = true;
        for i in 0..LOGO_SIZE{
            if bytes[i + offset] != NINTENDO_LOGO[i] {
                res = false;
                break;
            }
        }
        res
    }
}
