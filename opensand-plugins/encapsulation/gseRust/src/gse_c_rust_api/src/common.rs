use std::slice::{from_raw_parts, from_raw_parts_mut};
use dvb_gse_rust::label::{Label, LabelType};
use libc::size_t;


#[repr(C)]
#[derive(Copy, Clone)]
pub struct RustMutSlice {
    pub(crate) size: size_t,
    pub(crate) bytes: *mut u8,
}

pub fn cslice_from_rsslice(rs_slice: &mut [u8]) -> RustMutSlice {
    RustMutSlice {
        size: rs_slice.len(),
        bytes: rs_slice.as_mut_ptr(),
    }
}

impl RustMutSlice {
    #[allow(clippy::mut_from_ref)]
    pub fn to_slice(&self) -> &mut [u8] {
        let slice = unsafe { from_raw_parts_mut(self.bytes, self.size) };
        slice
    }

    pub fn to_box(&self) -> Box<[u8]> {
        let box_from_slice = unsafe { Box::from_raw(self.to_slice()) };
        box_from_slice
    }
}

#[repr(C)]
#[derive(Debug)]
pub struct RustSlice {
    pub size: size_t,
    pub bytes: *const u8,
}

impl RustSlice {
    pub fn to_slice(&self) -> &[u8] {
        let slice = unsafe { from_raw_parts(self.bytes, self.size) };
        slice
    }
}
#[repr(C)]
#[derive(Copy, Clone)]
pub struct RustLabel {
    label_type: RustLabelType,
    bytes: [u8; 6],
}
#[repr(C)]
#[derive(Copy, Clone)]
pub enum RustLabelType {
    SixBytes,
    ThreeBytes,
    Broadcast,
    ReUse,
}

impl RustLabel {
    pub fn to_rust_label(&self) -> Label {
        let label: Label = match self.label_type {
            RustLabelType::SixBytes => Label::new(&LabelType::SixBytesLabel, &self.bytes),
            RustLabelType::ThreeBytes => Label::new(&LabelType::ThreeBytesLabel, &self.bytes[0..3]),
            RustLabelType::Broadcast => Label::new(&LabelType::Broadcast, &self.bytes[0..0]),
            RustLabelType::ReUse => Label::new(&LabelType::ReUse, &self.bytes[0..0]),
        };

        label
    }
}

pub fn new_label_fromrs(rs_label: Label) -> RustLabel {
    let mut bytes: [u8; 6] = [0; 6];
    let label_type = match rs_label {
        Label::SixBytesLabel(label) => {
            bytes.copy_from_slice(&label);
            RustLabelType::SixBytes
        }
        Label::ThreeBytesLabel(label) => {
            bytes[0..3].copy_from_slice(&label[0..3]);
            RustLabelType::ThreeBytes
        }
        Label::Broadcast => RustLabelType::Broadcast,
        Label::ReUse => RustLabelType::ReUse,
    };
    RustLabel { label_type, bytes }
}