use dvb_gse_rust::crc::CrcCalculator;

extern "C" {
  #[allow(improper_ctypes, unused)]
  pub fn calculateZeroCrc() -> u32;
}

/// Structure that implement the ZeroCrc trait.
#[derive(Debug, PartialEq, Eq, Clone)]
pub struct ZeroCrc;

impl CrcCalculator for ZeroCrc {
    fn calculate_crc32(
        &self,
        pdu: &[u8],
        protocol_type: u16,
        total_length: u16,
        label: &[u8],
    ) -> u32 {
      calculateZeroCrc()
    }
}