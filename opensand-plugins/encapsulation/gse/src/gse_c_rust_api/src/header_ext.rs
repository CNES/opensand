use dvb_gse_rust::header_extension::{Extension, ExtensionData, NewExtensionError};
use libc::size_t;
use std::ptr;
#[repr(C)]
#[derive(Copy, Clone,Debug)]
pub struct CHeaderExtensionSlice {
    pub size: size_t,
    pub bytes: *const CHeaderExtension,
}

impl CHeaderExtensionSlice {
    // pub fn new(label_type: &LabelType, label: &[u8]) -> CHeaderExtensionSlice {

    // }
    pub fn new_empty() -> CHeaderExtensionSlice {
        CHeaderExtensionSlice {
            size: 0,
            bytes: ptr::null(),
        }
    }
}

#[repr(C)]
#[derive(Debug)]
pub struct CHeaderExtension {
    pub id: u16,
    pub data: *mut u8,
}

pub fn from_c_vec_to_rust_vec(mut c_vec: Vec<CHeaderExtension>) -> Vec<Extension> {
    let mut ret: Vec<Extension> = vec![];
    for _ in 0..c_vec.len() {
        let a = c_vec.pop();
        if let Some(v) = a {
            let b = v.to_rust_extension();
            ret.push(b)
        };
    }
    std::mem::forget(c_vec);

    ret
}

pub fn from_rust_vec_to_c_vec(c_vec: &Vec<Extension>) -> Vec<CHeaderExtension> {

    let mut ret: Vec<CHeaderExtension> = vec![];
    for rust_header_ext in c_vec {
        let b = new_headerext_fromrs(rust_header_ext);
        ret.push(b);
    }

    ret
}

impl CHeaderExtensionSlice {
    pub fn to_slice(self) -> Vec<CHeaderExtension> {
        unsafe { Vec::from_raw_parts(self.bytes as *mut CHeaderExtension, self.size, self.size) }

    }

    pub fn from(mut extensions: Vec<CHeaderExtension>) -> Self {
        let size = extensions.len();
        let bytes = extensions.as_mut_ptr() as *const CHeaderExtension;
        // Preventing the Vec from being dropped so the pointer remains valid
        std::mem::forget(extensions);
        CHeaderExtensionSlice {
            size,
            bytes: bytes as *mut CHeaderExtension,
        }
    }
}

pub fn new_headerext_fromrs(rs_extension: &Extension) -> CHeaderExtension {
    let mut bytes: [u8; 8] = [0; 8];
    let id: u16 = rs_extension.id();

    // Copy data into bytes array based on ExtensionData variant
    match rs_extension.data() {
        ExtensionData::MandatoryData(_) => panic!("No usage of MandatoryHeaderExtension in OpenSAND"),
        ExtensionData::NoData => {}
        ExtensionData::Data2(d) => {
            bytes[..2].copy_from_slice(d);
        }
        ExtensionData::Data4(d) => {
            bytes[..4].copy_from_slice(d);
        }
        ExtensionData::Data6(d) => {
            bytes[..6].copy_from_slice(d);
        }
        ExtensionData::Data8(d) => {
            bytes = *d;
        }
    };

    CHeaderExtension {
        id,
        data: bytes.as_mut_ptr(),
    }
}

impl CHeaderExtension {
    pub fn to_rust_extension(&self) -> Extension {
        let extension: Result<Extension, NewExtensionError> = match (self.id >> 8) & 0b111 {
            0 => panic!("No usage of MandatoryHeaderExtension in OpenSAND"),
            1 => Extension::new(self.id, &[]),
            2 => Extension::new(self.id, unsafe {&[
                *self.data.add(0),
                *self.data.add(1),
            ]}),
            3 => Extension::new(self.id, unsafe {&[
                *self.data.add(0),
                *self.data.add(1),
                *self.data.add(2),
                *self.data.add(3),
            ]}),
            4 => Extension::new(self.id, unsafe {&[
                *self.data.add(0),
                *self.data.add(1),
                *self.data.add(2),
                *self.data.add(3),
                *self.data.add(4),
                *self.data.add(5),
            ]}),
            5 => Extension::new(self.id, unsafe {&[
                *self.data.add(0),
                *self.data.add(1),
                *self.data.add(2),
                *self.data.add(3),
                *self.data.add(4),
                *self.data.add(5),
                *self.data.add(6),
                *self.data.add(7),
            ]}),
            _ => panic!("unreachable"),
        };
        extension.unwrap()
    }
}
