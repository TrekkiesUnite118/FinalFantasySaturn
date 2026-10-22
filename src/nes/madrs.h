/*******************************************************************
 *
 *                      APU Sound RAM Layout
 *
 * Where the APU emulation's tables and buffers sit in sound RAM,
 * and the DSP settings that go with them. Included by apu.c only.
 *
 * MADR_ names index madrs_init, which apu_preinit loads into the
 * DSP's MADRS. Trailing numbers are table sizes in 16-bit words.
 *
*******************************************************************/
static const uint32 tri_map_addr = 0x077800;

#define MADR_PULSE_BASE 0x00 // 2048
#define MADR_PULSE_FREQLO 0x01 // 2048
#define MADR_PULSE_FREQHI 0x02 // 2048
#define MADR_PULSE0_PHRST 0x03 //   16
#define MADR_PULSE1_PHRST 0x04 //   16
#define MADR_PULSE0_PHOFFSA 0x05 //   16
#define MADR_PULSE0_PHOFFSB 0x06 //   16
#define MADR_PULSE1_PHOFFSA 0x07 //   16
#define MADR_PULSE1_PHOFFSB 0x08 //   16
#define MADR_TRI_BASE 0x09 // 2048
#define MADR_TRI_FREQLO 0x0A // 2048
#define MADR_TRI_FREQHI 0x0B // 2048
#define MADR_NOISE_BASELO 0x0C //  256
#define MADR_NOISE_BASEHI 0x0D //  256
#define MADR_NOISE_MODEMUL 0x0E //  256
#define MADR_NOISE_FREQLO 0x0F //  256
#define MADR_NOISE_FREQHI 0x10 //  256
#define MADR_PULSE_VOLUME 0x11 //  256
#define MADR_NOISE_VOLUME 0x12 //  256
static const uint32 position_addr = 0x070EC0;

#define MADR_TRI_POSITION 0x13 //    1
#define MADR_PULSE0A_POSITION 0x14 //    1
#define MADR_PULSE0B_POSITION 0x15 //    1
#define MADR_PULSE1A_POSITION 0x16 //    1
#define MADR_PULSE1B_POSITION 0x17 //    1
#define MADR_PCM_QNLSCALE 0x18 //  128
static const int16 tri_coef_volume = 980;
static const int16 pcm_coef_volume = 2857;
static const uint32 time_scale_ntsc = 51674;
static const uint32 time_scale_pal = 55626;
#define MADR_PTNFIFOA 0x19
#define MADR_PTNFIFOB 0x1A
#define MADR_CFIFO 0x1B
#define MADR_EXCHIP 0x1C
#define MADR_CFIFO_POSITION 0x1D
static const uint32 cfifo_position_addr = 0x076FCA;



static const uint8 rbp_init = 0x35;

static const uint8 saw_bank64k = 0x05;

static const uint8 tri_bank64k = 0x07;

static const uint16 madrs_init[0x20] = 
{
 0x0000, // 0x00 ;;; Abs byte offs: 0x6A000
 0x0800, // 0x01 ;;; Abs byte offs: 0x6B000
 0x1000, // 0x02 ;;; Abs byte offs: 0x6C000
 0x1800, // 0x03 ;;; Abs byte offs: 0x6D000
 0x1810, // 0x04 ;;; Abs byte offs: 0x6D020
 0x1820, // 0x05 ;;; Abs byte offs: 0x6D040
 0x1830, // 0x06 ;;; Abs byte offs: 0x6D060
 0x1840, // 0x07 ;;; Abs byte offs: 0x6D080
 0x1850, // 0x08 ;;; Abs byte offs: 0x6D0A0
 0x1860, // 0x09 ;;; Abs byte offs: 0x6D0C0
 0x2060, // 0x0A ;;; Abs byte offs: 0x6E0C0
 0x2860, // 0x0B ;;; Abs byte offs: 0x6F0C0
 0x30E0, // 0x0C ;;; Abs byte offs: 0x701C0
 0x31E0, // 0x0D ;;; Abs byte offs: 0x703C0
 0x32E0, // 0x0E ;;; Abs byte offs: 0x705C0
 0x33E0, // 0x0F ;;; Abs byte offs: 0x707C0
 0x34E0, // 0x10 ;;; Abs byte offs: 0x709C0
 0x3560, // 0x11 ;;; Abs byte offs: 0x70AC0
 0x3660, // 0x12 ;;; Abs byte offs: 0x70CC0
 0x3760, // 0x13 ;;; Abs byte offs: 0x70EC0
 0x3761, // 0x14 ;;; Abs byte offs: 0x70EC2
 0x3762, // 0x15 ;;; Abs byte offs: 0x70EC4
 0x3763, // 0x16 ;;; Abs byte offs: 0x70EC6
 0x3764, // 0x17 ;;; Abs byte offs: 0x70EC8
 0x3765, // 0x18 ;;; Abs byte offs: 0x70ECA
 0x3FE5, // 0x19 ;;; Abs byte offs: 0x71FCA
 0x4FE5, // 0x1A ;;; Abs byte offs: 0x73FCA
 0x5BE5, // 0x1B ;;; Abs byte offs: 0x757CA
 0x63E5, // 0x1C ;;; Abs byte offs: 0x767CA
 0x67E5, // 0x1D ;;; Abs byte offs: 0x76FCA
 0x0000, // 0x1E ;;; Abs byte offs: 0x6A000
 0x0000, // 0x1F ;;; Abs byte offs: 0x6A000
};
