// m90e36.c - Copyright (c) 2018-24 Andre M. Maree / KSS Technologies (Pty) Ltd.

#include "hal_platform.h"

#if (HAL_M90E36 > 0)
#include "m90e36.h"
#include "errors_events.h"
#include "systiming.h"					// timing debugging
#include "syslog.h"
#include "string_general.h"
#include "endpoints.h"
#include "rules.h"

#if (HAL_SSD1306 > 0)
	#include "ssd1306.h"
#endif

#include "driver/spi_master.h"

#define	debugFLAG					0xF000

#define	debugREAD					(debugFLAG & 0x0001)
#define	debugWRITE					(debugFLAG & 0x0002)
#define	debugTIMING					(debugFLAG & 0x0004)
#define	debugENERGY					(debugFLAG & 0x0008)

#define	debugFREQ					(debugFLAG & 0x0010)
#define	debugFACTOR					(debugFLAG & 0x0020)
#define	debugANGLE					(debugFLAG & 0x0040)
#define	debugPOWER					(debugFLAG & 0x0080)

#define	debugINIT					(debugFLAG & 0x0100)
#define	debugMODE					(debugFLAG & 0x0200)
#define	debugCRC					(debugFLAG & 0x0400)
#define	debugOFFSET					(debugFLAG & 0x0800)

//#define	debugCONTRAST				(debugFLAG & 0x1000)

#define	debugTIMING					(debugFLAG_GLOBAL & debugFLAG & 0x1000)
#define	debugTRACK					(debugFLAG_GLOBAL & debugFLAG & 0x2000)
#define	debugPARAM					(debugFLAG_GLOBAL & debugFLAG & 0x4000)
#define	debugRESULT					(debugFLAG_GLOBAL & debugFLAG & 0x8000)

// ###################################### Private variables #######################################

spi_device_interface_config_t	m90e36_config[HAL_M90E36] = {
#if		(HAL_M90E36 > 0)
	[0] = {
		.command_bits		= 0,
		.address_bits		= 0,
		.dummy_bits			= 0,
		.mode				= 3,						// only SPI mode 3 supported
		.duty_cycle_pos		= 128,						// same as 0 = 50/50% duty cycle
		.cs_ena_pretrans 	= 0,
		.cs_ena_posttrans	= 0,
		.clock_speed_hz		= 1200000,
		.input_delay_ns		= 0,
		.spics_io_num		= GPIO_NUM_5,				// D8 = CS pin
		.flags				= 0,
		.queue_size			= 16,
		.pre_cb				= 0,						// no callback handler
		.post_cb			= 0,
	},
#endif
#if		(HAL_M90E36 > 1)
	[1] = {
		.command_bits		= 0,
		.address_bits		= 0,
		.dummy_bits			= 0,
		.mode				= 3,
		.duty_cycle_pos		= 128,						// same as 0 = 50/50% duty cycle
		.cs_ena_pretrans 	= 0,
		.cs_ena_posttrans	= 0,
		.clock_speed_hz		= 1200000,
		.input_delay_ns		= 0,
		.spics_io_num		= GPIO_NUM_17,				// D3 = CS pin
		.flags				= 0,
		.queue_size			= 16,
		.pre_cb				= 0,						// no callback handler
		.post_cb			= 0,
	},
#endif
} ;

spi_device_handle_t				m90e36_handle[HAL_M90E36] ;
struct {
	uint8_t	tBlank ;									// # seconds to blank in between
	uint8_t MinContrast ;
	uint8_t	MaxContrast ;
	uint8_t	NowContrast ;
	struct {
		uint8_t	E_Scale	: 1 ;							// 0 = WHr,	1 = KwHr
		uint8_t	P_Scale	: 1 ;							// 0 = W,	1 = Kw
		uint8_t	I_Scale	: 1 ;							// 0 = A,	1 = KwHr
		uint8_t	Display	: 2 ;							// 0 = Off, 1 = Normal,	2 = Current, 3 = Button
		uint8_t	Spare	: 3 ;
		uint8_t	N_Gain	: 3 ;							// 1 -> 4
		uint8_t	L_Gain	: 5 ;							// 1 -> 24
	} Chan[HAL_M90E36] ;
} m90e36Config ;

const conf_reg_t TabConfig[] = {
#if		(M90E36_CALIB_TABLE == 0)					// AMM values
	{ SOFTRESET,	0,	RSTCOD },					// Optional ?
	{ CFGSTART,		0,	STDCOD },
	{ PLconstH,		0,	0x0861 },
	{ PLconstL, 	0,	0xC468 },
	{ MET_MODE_0,	0,	0x0087 },
	{ MET_MODE_1,	0,	0x0000 },
	{ P_SUP_TH,		0,	0x0000 },
	{ Q_SUP_TH,		0,	0x0000 },
	{ S_SUP_TH,		0,	0x0000 },
	{ P_PHASE_TH,	0,	0x0000 },
	{ Q_PHASE_TH,	0,	0x0000 },
	{ S_PHASE_TH,	0,	0x0000 },

#elif	(M90E36_CALIB_TABLE == 1)					// Tisham values
	{ SOFTRESET,	0,	RSTCOD },					// Optional ?
	{ CFGSTART,		0,	STDCOD },
	{ PLconstH,		0,	0x0000 },
	{ PLconstL, 	0,	0x0000 },
	{ MET_MODE_0,	0,	0x0000 },
	{ MET_MODE_1,	0,	0x0000 },
	{ P_SUP_TH,		0,	0x0000 },
	{ Q_SUP_TH,		0,	0x0000 },
	{ S_SUP_TH,		0,	0x0000 },
	{ P_PHASE_TH,	0,	0x0000 },
	{ Q_PHASE_TH,	0,	0x0000 },
	{ S_PHASE_TH,	0,	0x0000 },

#elif	(M90E36_CALIB_TABLE == 2)					// defaults
	{ SOFTRESET,	0,	RSTCOD },					// Optional ?
	{ CFGSTART,		0,	STDCOD },
	{ PLconstH,		0,	0x0861 },
	{ PLconstL, 	0,	0xC468 },
	{ MET_MODE_0,	0,	0x0087 },
	{ MET_MODE_1,	0,	0x0000 },
	{ P_SUP_TH,		0,	0x0000 },
	{ Q_SUP_TH,		0,	0x0000 },
	{ S_SUP_TH,		0,	0x0000 },
	{ P_PHASE_TH,	0,	0x0000 },
	{ Q_PHASE_TH,	0,	0x0000 },
	{ S_PHASE_TH,	0,	0x0000 },
									// Amod=Fwd+Rev Energy Pulse, Rmod=Fwd+Rev Energy Pulse
									// Zxcon=All(pos+neg), Pthresh=3.125%
#else
	#error "Invalid calibration table value specified !!!"
#endif
} ;

const uint8_t	m90e36RegAddr[] = {
	AP_ENER_T, AP_ENER_A, AP_ENER_B, AP_ENER_C,			// Active Forward Energy
	AN_ENER_T, AN_ENER_A, AN_ENER_B, AN_ENER_C,			// Active Reverse Energy
	RP_ENER_T, RP_ENER_A, RP_ENER_B, RP_ENER_C,			// REActive Forward Energy
	RN_ENER_T, RN_ENER_A, RN_ENER_B, RN_ENER_C,			// REActive Reverse Energy
	SA_ENER_T, SA_ENER_A, SA_ENER_B, SA_ENER_C,			// Apparent Energy (Arithmetic)
	SV_ENER_T,											// Apparent Energy (Vector)
	P_MEAN_T , P_MEAN_A , P_MEAN_B , P_MEAN_C ,			// Power Total+A+B+C Active
	Q_MEAN_T , Q_MEAN_A , Q_MEAN_B , Q_MEAN_C ,			// Power Total+A+B+C Reactive
	SA_MEAN_T, SA_MEAN_A, SA_MEAN_B, SA_MEAN_C,			// Power Arithmetic Sum Apparent
	PF_MEAN_T, PF_MEAN_A, PF_MEAN_B, PF_MEAN_C,			// Power Factor
	V_RMS_A, V_RMS_B, V_RMS_C,							// Voltage RMS
	I_RMS_A, I_RMS_B, I_RMS_C,							// Current RMS
	THD_NV_A, THD_NV_B, THD_NV_C,						// Voltage THD
	THD_NI_A, THD_NI_B, THD_NI_C,						// Current THD
	FREQ,
	P_ANGLE_A, P_ANGLE_B, P_ANGLE_C,					// Mean Power Phase Angles
	TEMP,
	V_ANGLE_A, V_ANGLE_B, V_ANGLE_C,					// Mean Voltage Phase Angles
#if		(M90E36_FUNDAMENTAL == 1)
	AP_ENER_TF, AP_ENER_AF, AP_ENER_BF, AP_ENER_CF,		// Active Forward Fundamental
	AN_ENER_TF, AN_ENER_AF, AN_ENER_BF, AN_ENER_CF,		// Active Reverse Fundamental
	P_MEAN_TF, P_MEAN_AF, P_MEAN_BF, P_MEAN_CF,			// Power Active Fundamental
#endif
#if		(M90E36_HARMONIC == 1)
	AP_ENER_TH, AP_ENER_AH, AP_ENER_BH, AP_ENER_CH,		// Active Forward Harmonic
	AN_ENER_TH, AN_ENER_AH, AN_ENER_BH, AN_ENER_CH,		// Active Reverse Harmonic
	P_MEAN_TH, P_MEAN_AH, P_MEAN_BH, P_MEAN_CH,			// Power Active Harmonic
#endif
#if		(M90E36_NEUTRAL == 1)
	I_RMS_N1, I_RMS_N0,
#endif
} ;

const char	m90e36RegNameE[] = { "      "
	"AP Tot   AP PhA   AP PhB   AP PhC   "				// Active Forward Energy
	"AN Tot   AN PhA   AN PhB   AN PhC   "				// Active Reverse Energy
	"RP Tot   RP PhA   RP PhB   RP PhC   "				// REActive Forward Energy
	"RN Tot   RN PhA   RN PhB   RN PhC   "				// REActive Reverse Energy
	"SA Tot   SA PhA   SA PhB   SA PhC   "				// Apparent Energy (Arithmetic)
	"SV Tot   "											// Apparent Energy (Vector)
} ;

const char	m90e36RegNameP[] = { "      "
	"Pa Tot   Pa PhA   Pa PhB   Pa PhC   "				// Power Total+A+B+C Active
	"Pr Tot   Pr PhA   Pr PhB   Pr PhC   "				// Power Total+A+B+C Reactive
	"Ps Tot   Ps PhA   Ps PhB   Ps PhC   "				// Power Arithmetic Sum Apparent
	"Pf Tot   Pf PhA   Pf PhB   Pf PhC   "				// Power Factor
} ;

const char	m90e36RegNameV[] = { "      "
	"Vrms Tot Vrms ChA Vrms ChB Vrms ChC "
	"Irms Tot Irms ChA Irms ChB Irms ChC "
	"Vthd Tot Vthd ChA Vthd ChB Vthd ChC "
	"Ithd Tot Ithd ChA Ithd ChB Ithd ChC "
	"F(Hz)    Ideg A   Ideg B   Ideg C   "
	"T(Cel)   Vdeg A   Vdeg B   Vdeg C   "
} ;

const char	m90e36RegNameO[] = { "      "
#if		(M90E36_FUNDAMENTAL == 1)
	"AP TotF  AP PhAF  AP PhBF  AP PhCF  "				// Active Forward Fundamental
	"AN TotF  AN PhAF  AN PhBF  AN PhCF  "				// Active Reverse Fundamental
	"PaF Tot  PaF PhA  PaF PhB  PaF PhC  "				// Power Active Fundamental
#endif
#if		(M90E36_HARMONIC == 1)
	"AP TotH  AP PhAH  AP PhBH  AP PhCH  "				// Active Forward Harmonic
	"AN TotH  AN PhAH  AN PhBH  AN PhCH  "				// Active Reverse Harmonic
	"PaH Tot  PaH PhA  PaH PhB  PaH PhC  "				// Power Active Harmonic
#endif
#if		(M90E36_NEUTRAL == 1)
	"Isam N   Icalc N  "
#endif
} ;

// ###################################### Private functions ########################################

void	m90e36Write(uint8_t eChan, uint16_t address, uint16_t val) {
	spi_transaction_t m90e36_buf ;
	memset(&m90e36_buf, 0, sizeof(m90e36_buf));
	m90e36_buf.length		= 8 * 4;
	m90e36_buf.flags 		= SPI_TRANS_USE_TXDATA ;
	m90e36_buf.tx_data[0]	= address >> 8 ;
	m90e36_buf.tx_data[1]	= address & 0xFF ;
	m90e36_buf.tx_data[2]	= val >> 8 ;
	m90e36_buf.tx_data[3]	= val & 0xFF ;
	ESP_ERROR_CHECK(spi_device_transmit(m90e36_handle[eChan], &m90e36_buf)) ;
	IF_P(debugWRITE, "TX: addr=%02x%02x d0=%02x d1=%02x\r\n", m90e36_buf.tx_data[0], m90e36_buf.tx_data[1], m90e36_buf.tx_data[2], m90e36_buf.tx_data[3]) ;
}

uint16_t m90e36Read(uint8_t eChan, uint16_t address) {
	spi_transaction_t m90e36_buf ;
	memset(&m90e36_buf, 0, sizeof(m90e36_buf)) ;
	m90e36_buf.length		= 8 * 4 ;
	m90e36_buf.flags 		= SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA ;
	m90e36_buf.tx_data[0]	= (address >> 8) | 0x80 ;
	m90e36_buf.tx_data[1]	= address & 0xFF ;
	ESP_ERROR_CHECK(spi_device_transmit(m90e36_handle[eChan], &m90e36_buf)) ;
	IF_P(debugREAD, "RX: addr=%02x%02x d0=%02x d1=%02x\r\n", m90e36_buf.tx_data[0], m90e36_buf.rx_data[1], m90e36_buf.rx_data[2], m90e36_buf.rx_data[3]) ;
	return (m90e36_buf.rx_data[1] << 8) | m90e36_buf.rx_data[2] ;
}

uint16_t m90e36ReadModifyWrite(uint8_t eChan, uint16_t Addr, uint16_t Value, uint16_t Mask) {
	uint16_t CurValue = m90e36Read(eChan, Addr) ;
	CurValue &= ~Mask ;
	CurValue |= Value ;
	m90e36Write(eChan, Addr, CurValue) ;
	return CurValue ;
}

// ################################## (re)configuration & CRCs #####################################

void	m90e36SetCurrentOffset(uint8_t eChan, uint16_t RegRMS, uint16_t RegGAIN, uint16_t RegOFST) {
	uint16_t CurAmps, CurGain ;
	uint32_t Factor1, Factor2 ;

	CurAmps = m90e36Read(eChan, RegRMS) ;
	CurGain = m90e36Read(eChan, RegGAIN) ;
	Factor1 = (CurAmps * CurGain) / 2^8 ;
	Factor2 = ~Factor1 & 0x0000FFFF ;
	m90e36Write(eChan, RegOFST, Factor2) ;
	IF_P(debugOFFSET, "Ch %d: Regs=%d/%d->%d  Il=0x%04x  Gl=0x%04x  F1=0x%08x  F2=0x%04x\r\n",
			eChan, RegRMS, RegGAIN, RegOFST, CurAmps, CurGain, Factor1, Factor2) ;
}

void	m90e36SetPowerOffset(uint8_t eChan, uint16_t RegPOWER, uint16_t RegOFST) {
	uint32_t SumOffset = 0 ;
	for (int32_t i = 0; i < M90E36_CALIB_ITER; i++) {
		SumOffset += m90e36Read(eChan, RegPOWER) ;
	}
	uint16_t NewOffset = ~(SumOffset / M90E36_CALIB_ITER) ;
	m90e36Write(eChan, RegOFST, NewOffset) ;
	IF_P(debugOFFSET, "Ch %d: Regs=%d->%d  %dx  Sum=0x%08x  Ofst=0x%04x\r\n",
			eChan, RegPOWER, RegOFST, M90E36_CALIB_ITER, SumOffset, NewOffset) ;
}

/**
 * m90e36CalcCRC() - calculate the CRC for a range of registers
 */
uint16_t m90e36CalcCRC(uint8_t eChan, uint16_t Addr0, int16_t Count) {
	uint8_t Lcrc = 0, Hcrc = 0 ;
	uint16_t RegData[Count] ;
	for (int32_t i = 0; i < Count; i++) {				// read the range of registers
		RegData[i] = m90e36Read(eChan, Addr0 + i) ;
	}
	for (int32_t i = 0; i < Count; i++) {				// HI bytes: MOD256 sum & XOR
		Lcrc += RegData[i] >> 8 ;
		Hcrc ^= RegData[i] >> 8 ;
	}
	for (int32_t i = 0; i < Count; i++) {				// LO bytes: MOD256 sum & XOR
		Lcrc += RegData[i] & 0xFF ;
		Hcrc ^= RegData[i] & 0xFF ;
	}
	IF_P(debugCRC, "CRC=%04x from %-'hY", (Hcrc << 8) | Lcrc, Count * 2, RegData) ;
	return (Hcrc << 8) | Lcrc ;
}

/**
 * m90e36HandleCRC() - Calculate the CRC from the range of registers & write the lock register/pattern
 */
void	m90e36HandleCRC(uint8_t eChan, uint16_t RegAddr1, uint16_t RegAddr2) {
	IF_myASSERT(debugPARAM, (eChan < HAL_M90E36) &&
							((RegAddr1 == CALSTART && RegAddr2 == CRC_1) ||
							 (RegAddr1 == ADJSTART && RegAddr2 == CRC_2) ||
							 (RegAddr1 == ADJSTART && RegAddr2 == CRC_3) ) ) ;
	m90e36Write(eChan, RegAddr2, m90e36CalcCRC(eChan, RegAddr1 + 1, RegAddr2 - RegAddr1 - 1)) ;
	m90e36Write(eChan, RegAddr1, CFGCOD) ;
}

// ############################## identification & initialization ##################################

int32_t	m90e36Identify(uint8_t eDev) {
	IF_myASSERT(debugPARAM, eDev < HAL_M90E36) ;
	ESP_ERROR_CHECK(spi_bus_add_device(VSPI_HOST, &m90e36_config[eDev], &m90e36_handle[eDev])) ;
	return erSUCCESS ;
}

/**
 * m90e36Init() -
 */
int32_t	m90e36Init(uint8_t eChan) {
	IF_myASSERT(debugPARAM, eChan < HAL_M90E36) ;
	// check if maybe already configured and running
	uint16_t CfgStart = m90e36Read(eChan, CFGSTART) ;
	uint16_t CalStart = m90e36Read(eChan, CALSTART) ;
	uint16_t HF_Start = m90e36Read(eChan, HF_START) ;
	uint16_t AdjStart = m90e36Read(eChan, ADJSTART) ;
	IF_P(debugINIT, "m90e36 #%d Cfg=0X%04x  Cal=0X%04x  HF=0X%04x  Adj=0X%04x\r\n", eChan, CfgStart, CalStart, HF_Start, AdjStart) ;
	if (CfgStart == CFGCOD && CalStart == CFGCOD && HF_Start == CFGCOD && AdjStart == CFGCOD) {
		IF_P(debugINIT, "m90e36 #%d ALREADY configured & running !!!!\r\n", eChan) ;
	} else {
		// write the configuration registers
		for (int32_t i = 0; i < NO_MEM(TabConfig); i++) {
			m90e36Write(eChan, TabConfig[i].addr, TabConfig[i].raw_val) ;
		}
		m90e36HandleCRC(eChan, CFGSTART, CRC_0) ;
		IF_P(debugINIT, "m90e36 #%d NEW Cfg = 0X%04x\r\n", eChan, m90e36Read(eChan, CFGSTART)) ;

	#if		(M90E36_CALIB_TABLE == 0) && (M90E36_CALIB_SOFT == 1)
		/* Preference is to fix this functionality, then hard code the values into the
		 * initialization table and do it that way. Alternative would be to run this
		 * ONLY if calibration values cannot be found in NVS, same as WIFI credentials */
		IF_P(debugINIT, "Ch %d: Offset Compensation start, DISCONNECT CT's\r\n", eChan) ;

		/* LIVE & NEUTRAL Current Offset calibration not working properly
		 * The formula as described in Atmel-46102-SE-M90E36-ApplicationNote.pdf
		 * is NOT clear on the calculation and does not yield proper values  */
		m90e36SetCurrentOffset(eChan, I_RMS_L, I_GAIN_L, I_OFST_L) ;
		m90e36SetCurrentOffset(eChan, I_RMS_N, I_GAIN_N, I_OFST_N) ;

		/* [Re]Active, LINE & NEUTRAL Power Offset calibration
		 * Not sure if the power register should be read whilst in normal or
		 * adjustment mode. Currently no real value is being read */
		m90e36Write(eChan, POWER_MODE, PWRCOD) ;		// set into low power mode for calibration
		m90e36SetPowerOffset(eChan, P_ACT_L, P_OFST_L) ;	// L Line Active Power Offset
		m90e36SetPowerOffset(eChan, P_REACT_L, Q_OFST_L) ;	// L Line ReActive Power Offset
		m90e36SetPowerOffset(eChan, P_ACT_N, P_OFST_N) ;	// N Line Active Power Offset
		m90e36SetPowerOffset(eChan, P_REACT_N, Q_OFST_N) ;	// N Line ReActive Power Offset
		m90e36Write(eChan, POWER_MODE, RSTCOD) ;		// reset to normal power mode

		IF_P(debugINIT, "Ch %d: Offset Compensation done, RECONNECT CT's\r\n", eChan) ;
	#endif

		m90e36HandleCRC(eChan, ADJSTART, CRC_2) ;		// calculate & write CRC
		IF_P(debugINIT, "m90e36 #%d NEW Adj = 0X%04x\r\n", eChan, m90e36Read(eChan, ADJSTART)) ;
	}

	// set default state
	m90e36Config.Chan[eChan].L_Gain		= 1 ;
	m90e36Config.Chan[eChan].N_Gain		= 1 ;
	m90e36Config.Chan[eChan].E_Scale	= 0 ;			// WHr not KwHr
	m90e36Config.Chan[eChan].P_Scale	= 0 ;			// W not Kw
	m90e36Config.Chan[eChan].I_Scale	= 0 ;			// A not mA
	m90e36Config.MaxContrast			= 255 ;

	return (m90e36GetSysStatus(eChan) & 0xF000) ? erFAILURE : erSUCCESS ;
}

// ############################# Common support functions ##########################################

uint8_t	m90e36CalcURI(epw_t * psEW) {
	uint8_t eUri = (psEW - table_work) / sizeof(epw_t) ;
	IF_myASSERT(debugPARAM, (eUri == table_static[eUri].epid.epuri) &&
							INRANGE(URI_M90E36_E_AP_T_0, eUri, URI_M90E36_E_AN_CH_0)) ;
	return eUri ;
}

uint8_t	m90e36CalcChan(epw_t * psEW) {
	uint8_t eChan =	INRANGE(&table_static[URI_M90E36_E_AP_T_0], psEW, &table_static[URI_M90E36_E_AP_T_0 + URI_M90E36_NUMBER_0]) ? 0 :
					INRANGE(&table_static[URI_M90E36_E_AP_T_1], psEW, &table_static[URI_M90E36_E_AP_T_1 + URI_M90E36_NUMBER_1]) ? 1 : 255 ;
	IF_myASSERT(debugPARAM, eChan < M90E36_NUM) ;
	return eChan ;
}

uint16_t m90e36ReadRegister(epw_t * psEW, uint8_t * pChan) {
	*pChan	= m90e36CalcChan(psEW) ;
	return m90e36Read(*pChan, m90e36RegAddr[m90e36CalcURI(psEW)]) ;
}

// ############################# endpoint support functions ########################################

int32_t	m90e36SenseEnergy(epw_t * psEW) {
	if (psEW->Var.def.cv_sum) {					// if just a normal update cycle
		uint8_t	eChan ;
		uint16_t RawVal	= m90e36ReadRegister(psEW, &eChan) ;
		float f32Val	= (float) RawVal / (m90e36Config.Chan[eChan].E_Scale ? 10000.0 : 10.0) ;
		xEpSetValue(psEW, (x32_t) f32Val) ;
		IF_P(debugENERGY, "Energy: Ch=%d  Raw=0x%04X  Val=%9.3f\r\n", eChan, RawVal, f32Val) ;
	} else {											// else it is a value reset call
		vCV_ResetValue(&psEW->Var) ;
		IF_P(debugENERGY, "Energy: Sum RESET\r\n") ;
	}
	return erSUCCESS ;
}

int32_t	m90e36SensePower(epw_t * psEW) {
	uint8_t	eChan ;
	uint16_t RawVal	= m90e36ReadRegister(psEW, &eChan) ;
	float f32Val	= (float) xValue2sCompToNormal(RawVal, 16) ;
#if		(M90E36_RESOLUTION == 1)
	RawVal	= m90e36Read(eChan, m90e36RegAddr[m90e36CalcURI(psEW)] + (P_MEAN_T_LSB - P_MEAN_T)) ;
	f32Val	+= (float) RawVal / 65536 ;
#endif
	f32Val	/= 1000.0 ;
	xEpSetValue(psEW, (x32_t) f32Val) ;
	return erSUCCESS ;
}

int32_t	m90e36SenseVolts(epw_t * psEW) {
	uint8_t	eChan ;
	uint16_t RawVal	= m90e36ReadRegister(psEW, &eChan) ;
	float f32Val	= (float) RawVal ;
#if		(M90E36_RESOLUTION == 1)
	RawVal	= m90e36Read(eChan, m90e36RegAddr[m90e36CalcURI(psEW)] + (V_RMS_A_LSB - V_RMS_A)) ;
	f32Val	+= (float) RawVal / 65536 ;
#endif
	f32Val	/= 100.0 ;
	xEpSetValue(psEW, (x32_t) f32Val) ;
	return erSUCCESS ;
}

int32_t	m90e36SenseCurrent(epw_t * psEW) {
	uint8_t	eChan ;
	uint16_t RawVal	= m90e36ReadRegister(psEW, &eChan) ;
	float f32Val	= (float) RawVal ;
#if		(M90E36_RESOLUTION == 1)
	RawVal	= m90e36Read(eChan, m90e36RegAddr[m90e36CalcURI(psEW)] + (I_RMS_A_LSB - I_RMS_A)) ;
	f32Val	+= (float) RawVal / 65536 ;
#endif
	f32Val	/= 1000 ;
	xEpSetValue(psEW, (x32_t) f32Val) ;
	return erSUCCESS ;
}

int32_t	m90e36SenseU100(epw_t * psEW) {			// THDx & FREQ
	uint8_t	eChan ;
	uint16_t RawVal	= m90e36ReadRegister(psEW, &eChan) ;
	float f32Val	= (float) RawVal / 100.0 ;
	xEpSetValue(psEW, (x32_t) f32Val) ;
	IF_P(debugFREQ, "Ch %d: Raw:0x%04X  Val:%9.3f\r", eChan, RawVal, f32Val) ;
	return erSUCCESS ;
}

int32_t	m90e36SenseTemperature(epw_t * psEW) {	// TEMP
	uint8_t	eChan ;
	uint16_t RawVal	= m90e36ReadRegister(psEW, &eChan) ;
	int32_t ConvVal = RawVal & 0x8000 ? -1 * (RawVal & 0x7FFF) : RawVal ;
	float f32Val	= (float) ConvVal / 100.0 ;
	xEpSetValue(psEW, (x32_t) f32Val) ;
	IF_P(debugFREQ, "Ch %d: Raw:0x%04X  Conv:0x%08X  Val:%9.3f\r", eChan, RawVal, ConvVal, f32Val) ;
	return erSUCCESS ;
}

int32_t	m90e36SensePowerFactor(epw_t * psEW) {
	uint8_t	eChan ;
	uint16_t RawVal	= m90e36ReadRegister(psEW, &eChan) ;
	int32_t ConvVal = RawVal & 0x8000 ? -1 * (RawVal & 0x7FFF) : RawVal ;
	float f32Val	= (float) ConvVal / 1000.0 ;
	xEpSetValue(psEW, (x32_t) f32Val) ;
	IF_P(debugFACTOR, "Ch %d: Raw:0x%04X  Conv:0x%08X  Val:%9.3f\r", eChan, RawVal, ConvVal, f32Val) ;
	return erSUCCESS ;
}

int32_t	m90e36SenseAngle(epw_t * psEW) {
	uint8_t	eChan ;
	uint16_t RawVal	= m90e36ReadRegister(psEW, &eChan) ;
	int32_t ConvVal = RawVal & 0x8000 ? -1 * (RawVal & 0x7FFF) : RawVal ;
	float f32Val	= (float) ConvVal / 10.0 ;
	xEpSetValue(psEW, (x32_t) f32Val) ;
	IF_P(debugANGLE, "Ch %d: Raw:0x%04X  Conv:0x%08X  Val:%9.3f\r", eChan, RawVal, ConvVal, f32Val) ;
	return erSUCCESS ;
}

int32_t	m90e36SetLiveGain(uint8_t eChan, uint8_t Gain) {
	uint16_t	NewValue ;
	switch (Gain) {
	case 1:		NewValue	= 0x8000 ;	break ;
	case 4:		NewValue	= 0x0000 ;	break ;
	case 8:		NewValue	= 0x2000 ;	break ;
	case 16:	NewValue	= 0x4000 ;	break ;
	case 24:	NewValue	= 0x6000 ;	break ;
	default:	IF_SL_ERR(debugPARAM, "Invalid Live Gain =%d", Gain) ; return erFAILURE ;
	}
	NewValue = m90e36ReadModifyWrite(eChan, MET_MODE_0, NewValue, 0xE000) ;
	m90e36HandleCRC(eChan, CFGSTART, CRC_0) ;
	return erSUCCESS ;
}

int32_t	m90e36SetNeutralGain(uint8_t eChan, uint8_t Gain) {
	uint16_t	NewValue ;
	switch (Gain) {
	case 1:		NewValue	= 0x1000 ;	break ;
	case 2:		NewValue	= 0x0000 ;	break ;
	case 4:		NewValue	= 0x0800 ;	break ;
	default:	 IF_SL_ERR(debugPARAM, "Invalid Neutral Gain =%d", Gain) ; return erFAILURE ;
	}
	NewValue = m90e36ReadModifyWrite(eChan, MET_MODE_0, NewValue, 0x1800) ;
	m90e36HandleCRC(eChan, CFGSTART, CRC_0) ;
	return erSUCCESS ;
}

uint32_t m90e36GetSysStatus(uint8_t eChan) { return ((m90e36Read(eChan, SYS_STATUS0) << 16) | m90e36Read(eChan, SYS_STATUS1)) ; }

uint32_t m90e36GetMeterStatus(uint8_t eChan) { return ((m90e36Read(eChan, MET_STAT_0) << 16) | m90e36Read(eChan, MET_STAT_1)) ; }

inline uint16_t m90e36GetLastData(uint8_t eChan)	{ return m90e36Read(eChan, LASTDATA) ; }

// ############################### device reporting functions ######################################

void	m90e36ReportSystem(uint8_t eChan) {
	m90e36system_stat_t SysStatus = (m90e36system_stat_t) m90e36GetSysStatus(eChan) ;
	printfx("Ch %d :  SystemStatus=%08X", eChan, SysStatus.x32) ;
	if (SysStatus.CS0Err)		printfx("\tCRC_0 Error!!") ;
	if (SysStatus.CS1Err)		printfx("\tCRC_1 Error!!") ;
	if (SysStatus.CS2Err)		printfx("\tCRC_2 Error!!") ;
	if (SysStatus.CS3Err)		printfx("\tCRC_3 Error!!") ;
	if (SysStatus.VrevWn)		printfx("\tVoltage Phase Seq ERROR!") ;
	if (SysStatus.IrevWn)		printfx("\tCurrent Phase Seq ERROR!") ;
	if (SysStatus.SagWarn)		printfx("\tVoltage SAG") ;
	if (SysStatus.PhLoseWn)		printfx("\tVoltage Phase LOST!") ;
	if (SysStatus.INOv1)		printfx("\tNeutral Isamp OVER Threshold") ;
	if (SysStatus.INOv0)		printfx("\tNeutral Icalc OVER Threshold") ;
	if (SysStatus.THDUOv)		printfx("\tTHD X Voltage OVER Threshold") ;
	if (SysStatus.THDIOv)		printfx("\tTHD X Current OVER Threshold") ;
	if (SysStatus.DFTdone)		printfx("\tDFT Data ready") ;
	const char cFlags[] = "TABC" ;
	char cBuffer[sizeof(cFlags)] ;
	if (SysStatus.RevQ) {
		xStringValueMap(cFlags, cBuffer, SysStatus.RevQ, sizeof(cFlags) - 1) ;
		printfx("\tRevQ: %s", cBuffer) ;
	}
	if (SysStatus.RevP) {
		xStringValueMap(cFlags, cBuffer, SysStatus.RevP, sizeof(cFlags) - 1) ;
		printfx("\tRevP: %s", cBuffer) ;
	}
	printfx(strCRLF) ;
}

void	m90e36ReportMeter(uint8_t eChan) {
	m90e36Meter_stat_t MeterStatus = (m90e36Meter_stat_t) m90e36GetMeterStatus(eChan) ;
	printfx("Ch %d :  MeterStatus %08X\r\n", eChan, MeterStatus.x32) ;
	if (MeterStatus.TQNoload)	printfx("\tReActive NO Load  ") ;
	if (MeterStatus.TPNoload)	printfx("\tActive NO Load  ") ;
	if (MeterStatus.TASNoload)	printfx("\tActive NO Load  ") ;
	if (MeterStatus.TVSNoload)	printfx("\tActive NO Load  ") ;
	if (MeterStatus.CFxRevFlg) {
		const char cFlags[] = "1234" ;
		char cBuffer[sizeof(cFlags)] ;
		xStringValueMap(cFlags, cBuffer, MeterStatus.CFxRevFlg, sizeof(cFlags) - 1) ;
		printfx("\tCFxRevFlg: %s", cBuffer) ;
	}
	if (MeterStatus.SagPhX) {
		const char cFlags[] = "ABC" ;
		char cBuffer[sizeof(cFlags)] ;
		xStringValueMap(cFlags, cBuffer, MeterStatus.CFxRevFlg, sizeof(cFlags) - 1) ;
		printfx("\tSagPhX: %s", cBuffer) ;
	}
	if (MeterStatus.LossPhX) {
		const char cFlags[] = "ABC" ;
		char cBuffer[sizeof(cFlags)] ;
		xStringValueMap(cFlags, cBuffer, MeterStatus.CFxRevFlg, sizeof(cFlags) - 1) ;
		printfx("\tLossPhX: %s", cBuffer) ;
	}
}

void	m90e36ReportConfig(uint8_t eChan) {
	printfx("Ch %d:  CFGSTRT  PLconsH  PLconsL  MMOde0   MMode1   PSupTh   QSupTh   SSupTh   PPhaTh   QPhaTh   SPhaTh   CRC_0\r\n      ", eChan) ;
	for(int32_t i = CFGSTART; i <= CRC_0; i++) {
		printfx("   0x%04X", m90e36Read(eChan, i)) ;
	}
	printfx(strCRLF) ;
}

void	m90e36ReportMeterCalib(uint8_t eChan) {
	printfx("Ch %d:  CALSTRT  PofstA   QofstA   PofstB   QofstB   PofstC   QofstC   CgainA   CphiA    CgainB   CphiB    CgainC   CphiC    CRC_1\r\n      ", eChan) ;
	for(int32_t i = CALSTART; i <= CRC_1; i++) {
		printfx("   0x%04X", m90e36Read(eChan, i)) ;
	}
	printfx(strCRLF) ;
}

void	m90e36ReportFundaCalib(uint8_t eChan) {
	printfx("Ch %d:  HFSTART  PoffAF    PoffBF    PoffCF    PgainAF  PgainBF  PgainCF  CRC_2\r\n      ", eChan) ;
	for(int32_t i = HF_START; i <= CRC_2; i++) {
		printfx("   0x%04X", m90e36Read(eChan, i)) ;
	}
	printfx(strCRLF) ;
}

void	m90e36ReportMeasureCalib(uint8_t eChan) {
	printfx("Ch %d:  ADJSTRT  VgainA  IgainA   VofstA   IofstA   VgainB  IgainB   VofstB   IofstB   VgainC  IgainC   VofstC   IofstC   IgainN   IofstN   CRC_3\r\n     ", eChan) ;
	for(int32_t i = ADJSTART; i <= CRC_3; i++) {
		printfx("   0x%04X", m90e36Read(eChan, i)) ;
	}
	printfx(strCRLF) ;
}

void	m90e36ReportData(uint8_t eChan) {
	const char * pHeader ;
	for (int32_t i = 0; i < sizeof(m90e36RegAddr); i++) {
		if (i == 0) {
			pHeader = m90e36RegNameE ;
		} else if (i == 21) {
			pHeader =  m90e36RegNameP ;
		} else if (i == 37) {
			pHeader =  m90e36RegNameV ;
		} else if (i == 62) {
			pHeader = m90e36RegNameO ;
		}
		if (pHeader) {
			printfx("\r\nCh %d: %s\r\n", eChan, pHeader) ;
			pHeader = NULL ;
		}
		printfx("   0x%04X", m90e36Read(eChan, m90e36RegAddr[i])) ;
	}
	printfx(strCRLF) ;
}

void	m90e36Report(void) {
	for (int32_t eChan = 0; eChan < HAL_M90E36; eChan++) {
		m90e36ReportSystem(Handle, eChan) ;
		m90e36ReportMeter(Handle, eChan) ;
		m90e36ReportConfig(Handle, eChan) ;
		m90e36ReportMeterCalib(Handle, eChan) ;
		m90e36ReportFundaCalib(Handle, eChan) ;
		m90e36ReportMeasureCalib(Handle, eChan) ;
		m90e36ReportData(Handle, eChan) ;
	}
}

#if		(HAL_SSD1306 > 0)

static	uint8_t Index = 0 ;
static	TickType_t PrevTick = 0 ;
void	m90e36Display(void) {
	if (m90e36_handle[0] == 0) {
		return ;
	}
	TickType_t CurTicks = xTaskGetTickCount() ;
	if ((CurTicks - PrevTick) < M90E36_STAT_INTVL) {	// enough time elapsed ?
		return ;										// nope, return
	}
	PrevTick = CurTicks ;								// yes, save current timestamp
	uint8_t eChan = Index / HAL_M90E36 ;
	ssd1306SetTextCursor(&sSSD1306, 0, 0) ;
	uint8_t Offset = eChan ? URI_M90E36_NUMBER_0 : 0 ;
	switch(Index % m90e36SCREENS) {
	case 0:
		devprintf(ssd1306PutC, "Va%8.3fVb%8.3fVc%8.3f" "Ia%8.3fIb%8.3fIc%8.3f",
			xCV_GetValue(&table_work[Offset + URI_M90E36_V_RMS_A_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_V_RMS_B_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_V_RMS_C_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_I_RMS_A_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_I_RMS_B_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_I_RMS_C_0].Var, NULL)) ;
		break ;
	case 1:
		devprintf(ssd1306PutC, "APa%7.3fAPb%7.3fAPc%7.3f" "ANa%7.3fANb%7.3fANc%7.3f",
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_AP_A_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_AP_B_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_AP_C_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_AN_A_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_AN_B_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_AN_C_0].Var, NULL)) ;
		break ;
	case 2:
		devprintf(ssd1306PutC, "APt%7.3fANt%7.3f" "RPt%7.3fRNt%7.3f" "SAt%7.3fSVt%7.3f",
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_AP_T_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_AN_T_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_RP_T_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_RN_T_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_SA_T_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_SV_T_0].Var, NULL)) ;
		break ;
	case 3:
		devprintf(ssd1306PutC, "PAa%7.3fPAb%7.3fPAc%7.3f" "PRa%7.3fPRb%7.3fPRc%7.3f",
			xCV_GetValue(&table_work[Offset + URI_M90E36_P_ACT_A_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_P_ACT_B_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_P_ACT_C_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_P_REA_A_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_P_REA_B_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_P_REA_C_0].Var, NULL)) ;
		break ;

#if		(m90e36SCREENS > 4)
	case 4:
	#if		(M90E36_FUNDAMENTAL == 1)
		devprintf(ssd1306PutC, "APaF%6.2fAPbF%6.2fAPcF%6.2f" "AnaF%6.2fANbF%6.2fAncF%6.2f",
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_AP_AF_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_AP_BF_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_AP_CF_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_AN_AF_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_AN_BF_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_AN_CF_0].Var, NULL)) ;
	#elif	(M90E36_HARMONIC == 1)
		devprintf(ssd1306PutC, "APaH%6.2fAPbH%6.2fAPcH%6.2f" "ANaH%6.2fANbH%6.2fANcH%6.2f",
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_AP_AH_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_AP_BH_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_AP_CH_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_AN_AH_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_AN_BH_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_E_AN_CH_0].Var, NULL)) ;
	#elif	(M90E36_NEUTRAL == 1)
		devprintf(ssd1306PutC, "Ins%7.3fInc%7.3f",
			xCV_GetValue(&table_work[Offset + URI_M90E36_I_RMS_NS_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_I_RMS_NC_0].Var, NULL)) ;
	#endif
		break ;
#endif

#if		(m90e36SCREENS > 5)
	case 5:
	#if		(M90E36_HARMONIC == 1)
		devprintf(ssd1306PutC, "PAa%7.3fPAb%7.3fPAc%7.3f" "PRa%7.3fPRb%7.3fPRc%7.3f",
			xCV_GetValue(&table_work[Offset + URI_M90E36_P_ACT_A_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_P_ACT_B_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_P_ACT_C_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_P_REA_A_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_P_REA_B_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_P_REA_C_0].Var, NULL)) ;
	#elif	(M90E36_NEUTRAL == 1)
		devprintf(ssd1306PutC, "Ins%7.3fInc%7.3f",
			xCV_GetValue(&table_work[Offset + URI_M90E36_I_RMS_NS_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_I_RMS_NC_0].Var, NULL)) ;
	#endif
		break ;
#endif

#if		(m90e36SCREENS > 6)
	case 6:
		devprintf(ssd1306PutC, "Ins%7.3fInc%7.3f",
			xCV_GetValue(&table_work[Offset + URI_M90E36_I_RMS_NS_0].Var, NULL),
			xCV_GetValue(&table_work[Offset + URI_M90E36_I_RMS_NC_0].Var, NULL)) ;
		break ;
#endif
	default:
		break ;
	}
	Index++ ;
	Index %= (HAL_M90E36 * m90e36SCREENS) ;
}
#endif

// ################################## Diagnostics functions ########################################

#if		(HAL_M90E36 > 0)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)		// 21x ENERGY
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
#if		(M90E36_FUNDAMENTAL == 1)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)		// 8x ENERGY Fundamental
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,  		1000)
#endif
#if		(M90E36_HARMONIC == 1)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)		// 8x ENERGY Harmonic
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
#endif
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)		// 16x POWER
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
#if		(M90E36_FUNDAMENTAL == 1)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)		// 16x POWER Fundamental
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
#endif
#if		(M90E36_HARMONIC == 1)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)		// 16x POWER Harmonic
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
#endif
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)		// 3x Vrms
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)		// 3x Irms
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)		// 3x THDv
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)		// 3x THDi
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)		// FREQ
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)		// 3x Pang
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)		// TEMP
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)		// 3x Vang
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	#if		(M90E36_NEUTRAL == 1)		// Neutral Line
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)		// 2x Irms (Neutral)
	EP_WORK(1,		0,		vfFXX, vtVALUE,		vs32B,	1,			0,			1000)
	#endif
#endif
#endif
