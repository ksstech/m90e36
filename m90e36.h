/*
 * m90e36.h - Copyright (c) 2018-24 Andre M. Maree/KSS Technologies (Pty) Ltd.
 */

#pragma once

#include <stdint.h>

/* Notes & references:
 * mode option to select WHr or KWHr, set default, variable used as divisor
 *
 * Full duplex, synchronous
 * MSB first
 *
 * https://www.hackster.io/whatnick/atm90e36-and-esp8266-energy-monitoring-cdac92
 * https://imgur.com/a/BsEUM
 * https://www.reddit.com/r/diyelectronics/
 */

#ifdef __cplusplus
extern "C" {
#endif

// ##################################### BUILD definitions #########################################

#define M90E36_CALIB_TABLE			0					// 0=AMM, 1=Tisham, 2=DEFAULT
// Only effective if (M90E36_CALIB_TABLE == 0) above
#define	M90E36_CALIB_SOFT			0					// enable software based calibration
#define	M90E36_CALIB_ITER			100					// number of READ iterations to determine mean value

#define	M90E36_STAT_INTVL			pdMS_TO_TICKS(2 * MILLIS_IN_SECOND)
#define	M90E36_RESOLUTION			1					// enable additional LSB values to be included
#define	M90E36_LAST_DATA			1					// enable support for LASTDATA verification

#define	M90E36_FUNDAMENTAL			1					// Enable Fundamental ENERGY endpoints
#define	M90E36_HARMONIC				1					// Enable Harmonic ENERGY endpoints
#define	M90E36_NEUTRAL				1

#if		(M90E36_FUNDAMENTAL == 0) && (M90E36_HARMONIC == 0) && (M90E36_NEUTRAL == 0)
	#define	m90e36SCREENS		4
#elif	(M90E36_FUNDAMENTAL == 1) && (M90E36_HARMONIC == 0) && (M90E36_NEUTRAL == 0)
	#define	m90e36SCREENS		5
#elif	(M90E36_FUNDAMENTAL == 0) && (M90E36_HARMONIC == 1) && (M90E36_NEUTRAL == 0)
	#define	m90e36SCREENS		5
#elif	(M90E36_FUNDAMENTAL == 0) && (M90E36_HARMONIC == 0) && (M90E36_NEUTRAL == 1)
	#define	m90e36SCREENS		5
#elif	(M90E36_FUNDAMENTAL == 1) && (M90E36_HARMONIC == 1) && (M90E36_NEUTRAL == 0)
	#define	m90e36SCREENS		6
#elif	(M90E36_FUNDAMENTAL == 1) && (M90E36_HARMONIC == 0) && (M90E36_NEUTRAL == 1)
	#define	m90e36SCREENS		6
#elif	(M90E36_FUNDAMENTAL == 0) && (M90E36_HARMONIC == 1) && (M90E36_NEUTRAL == 1)
	#define	m90e36SCREENS		6
#elif	(M90E36_FUNDAMENTAL == 1) && (M90E36_HARMONIC == 1) && (M90E36_NEUTRAL == 1)
	#define	m90e36SCREENS		7
#endif

// ############################################# Macros ############################################

//#define POWER_MODE			0x04 	// Small-Power Mode
//#define LSB					0x08 	// RMS/Power 16-bit LSB

// Status and Special Registers..
#define SOFTRESET			0x00	// Software Reset
#define SYS_STATUS0			0x01	// System Status 0
#define SYS_STATUS1			0x02	// System Status 1
#define FUNC_ENAB0			0x03	// Function Enable 0
#define FUNC_ENAB1			0x04	// Function Enable 1
#define	ZX_CONFIG			0x07
#define V_SAG_THR			0x08 	// Voltage Sag Threshold
#define	PH_LOSS_TH			0x09	// Phase Loss Threshold
#define	IN_WARN_TH0			0x0A	// Threshold for calculated (Ia+Ib+Ic) NEUTRAL line RMS current
#define	IN_WARN_TH1			0x0B	// Threshold for sampled (from ADC) NEUTRAL line RMS current
#define	THD_NU_TH			0x0C	// Voltage THD warning threshold
#define	THD_NI_TH			0x0D	// Current THD warning threshold
#define	DMA_CNTL			0x0E	// DMA mode interface Control
#define LASTDATA			0x0F	// Last Read/Write SPI Value
// Low Power (Partial Measurement) Mode Registers
#define	DETECT_CTRL			0x10
#define	DETECT_TH1			0x11
#define	DETECT_TH2			0x12
#define	DETECT_TH3			0x13
#define	PM_OFST_A			0x14
#define	PM_OFST_B			0x15
#define	PM_OFST_C			0x16
#define	PM_PMGA_GAIN		0x17
#define	PM_I_RMS_A			0x18
#define	PM_I_RMS_B			0x19
#define	PM_I_RMS_C			0x1A
#define	PM_CONFIG			0x1B
#define	PM_AVG_SAMP			0x1C
#define	PM_I_RMS_LSB		0x1D
// Configuration Registers
#define CFGSTART			0x30 	// Configuration Start/End
#define PLconstH			0x31 	// High Word of PL_Constant
#define PLconstL			0x32 	// Low Word of PL_Constant
#define MET_MODE_0			0x33 	// Metering Method Configuration
#define MET_MODE_1			0x34 	// PGA Gain Configuration
#define P_SUP_TH			0x35 	// Active Startup Power Threshold
#define Q_SUP_TH			0x36 	// Reactive Startup Power Threshold
#define S_SUP_TH			0x37 	// Apparent Startup Power Threshold
#define	P_PHASE_TH			0x38	// Active Power Threshold (Energy Accumulation)
#define	Q_PHASE_TH			0x39	// Reactive Power Threshold (Energy Accumulation)
#define	S_PHASE_TH			0x3A	// Apparent Power Threshold (Energy Accumulation)
#define CRC_0				0x3B 	// Checksum 0
// Basic Metering Calibration
#define CALSTART			0x40 	// Calibration Start/End
#define P_OFST_A			0x41 	// Phase A Active Power Offset
#define Q_OFST_A 			0x42 	// Phase A Reactive Power Offset
#define P_OFST_B			0x43 	// Phase B Active Power Offset
#define Q_OFST_B 			0x44 	// Phase B Reactive Power Offset
#define P_OFST_C			0x45 	// Phase C Active Power Offset
#define Q_OFST_C 			0x46 	// Phase C Reactive Power Offset
#define GAIN_A				0x47 	// Phase A Calibration Gain
#define PHI_A				0x48 	// Phase A Calibration Angle
#define GAIN_B				0x49 	// Phase B Calibration Gain
#define PHI_B				0x4A 	// Phase B Calibration Angle
#define GAIN_C				0x4B 	// Phase C Calibration Gain
#define PHI_C				0x4C 	// Phase C Calibration Angle
#define CRC_1 				0x4D 	// Checksum 1
// Fundamental Energy Calibration
#define	HF_START			0x50	// Fundamental Start/End
#define	P_OFST_AF			0x51	// Phase A Fundamental Active Power Offset
#define	P_OFST_BF			0x52	// Phase B Fundamental Active Power Offset
#define	P_OFST_CF			0x53	// Phase C Fundamental Active Power Offset
#define	P_GAIN_AF			0x54	// Phase A Fundamental Active Power Gain
#define	P_GAIN_BF			0x55	// Phase B Fundamental Active Power Gain
#define	P_GAIN_CF			0x56	// Phase C Fundamental Active Power Gain
#define CRC_2 				0x57 	// Checksum 2
// Measurement Calibration
#define	ADJSTART			0x60
#define	V_GAIN_A			0x61	// Phase A Voltage RMS Gain
#define	I_GAIN_A			0x62	// Phase A Current RMS Gain
#define	V_OFST_A			0x63	// Phase A Voltage RMS Offset
#define	I_OFST_A			0x64	// Phase A Current RMS Offset
#define	V_GAIN_B			0x65	// Phase B Voltage RMS Gain
#define	I_GAIN_B			0x66	// Phase B Current RMS Gain
#define	V_OFST_B			0x67	// Phase B Voltage RMS Offset
#define	I_OFST_B			0x68	// Phase B Current RMS Offset
#define	V_GAIN_C			0x69	// Phase C Voltage RMS Gain
#define	I_GAIN_C			0x6A	// Phase C Current RMS Gain
#define	V_OFST_C			0x6B	// Phase C Voltage RMS Offset
#define	I_OFST_C			0x6C	// Phase C Current RMS Offset
#define	I_GAIN_N			0x6D	// Sampled NEUTRAL Current RMS Gain
#define	I_OFST_N			0x6E	// Sampled NEUTRAL Current RMS Offset
#define CRC_3 				0x6F 	// Checksum 3
// Energy registers
#define AP_ENER_T 			0x80 	// Total Forward Active Energy
#define AP_ENER_A 			0x81 	// Phase A Forward Active Energy
#define AP_ENER_B 			0x82 	// Phase B Forward Active Energy
#define AP_ENER_C 			0x83 	// Phase C Forward Active Energy
#define AN_ENER_T 			0x84 	// Total Reverse Active Energy
#define AN_ENER_A 			0x85 	// Phase A Reverse Active Energy
#define AN_ENER_B 			0x86 	// Phase B Reverse Active Energy
#define AN_ENER_C 			0x87 	// Phase C Reverse Active Energy

#define RP_ENER_T 			0x88 	// Total Forward Reactive Energy
#define RP_ENER_A 			0x89 	// Phase A Forward Reactive Energy
#define RP_ENER_B 			0x8A 	// Phase B Forward Reactive Energy
#define RP_ENER_C 			0x8B 	// Phase C Forward Reactive Energy
#define RN_ENER_T 			0x8C 	// Total Reverse Reactive Energy
#define RN_ENER_A 			0x8D 	// Phase A Reverse Reactive Energy
#define RN_ENER_B 			0x8E 	// Phase B Reverse Reactive Energy
#define RN_ENER_C 			0x8F 	// Phase C Reverse Reactive Energy

#define SA_ENER_T 			0x90 	// Total Apparent (SUM) Energy
#define SA_ENER_A 			0x91 	// Phase A Apparent Energy
#define SA_ENER_B 			0x92 	// Phase B Apparent Energy
#define SA_ENER_C 			0x93 	// Phase C Apparent Energy
#define SV_ENER_T 			0x94 	// Total Apparent (VECTOR) Energy

#define MET_STAT_0 			0x95 	// Metering Status 0
#define MET_STAT_1 			0x96 	// Metering Status 1
#define SV_MEAN_T 			0x98 	// Total Apparent (VECTOR) Power
#define SV_MEAN_T_LSB 		0x99 	// Total Apparent (VECTOR) Power LSB

// Energy registers (Fundamental)
#define AP_ENER_TF 			0xA0 	// Total Forward Active Fundamental Energy
#define AP_ENER_AF 			0xA1 	// Phase A Forward Active Fundamental Energy
#define AP_ENER_BF 			0xA2 	// Phase B Forward Active Fundamental Energy
#define AP_ENER_CF			0xA3 	// Phase C Forward Active Fundamental Energy
#define AN_ENER_TF			0xA4 	// Total Reverse Active Fundamental Energy
#define AN_ENER_AF			0xA5 	// Phase A Reverse Active Fundamental Energy
#define AN_ENER_BF			0xA6 	// Phase B Reverse Active Fundamental Energy
#define AN_ENER_CF			0xA7 	// Phase C Reverse Active Fundamental Energy

// Energy registers (Harmonic)
#define AP_ENER_TH			0xA8 	// Total Forward Active Harmonic Energy
#define AP_ENER_AH			0xA9 	// Phase A Forward Active Harmonic Energy
#define AP_ENER_BH			0xAA 	// Phase B Forward Active Harmonic Energy
#define AP_ENER_CH			0xAB 	// Phase C Forward Active Harmonic Energy
#define AN_ENER_TH			0xAC 	// Total Reverse Active Harmonic Energy
#define AN_ENER_AH			0xAD 	// Phase A Reverse Active Harmonic Energy
#define AN_ENER_BH			0xAE 	// Phase B Reverse Active Harmonic Energy
#define AN_ENER_CH			0xAF 	// Phase C Reverse Active Harmonic Energy

// Power & Power Factor registers
#define	P_MEAN_T			0xB0	// Total (All Phase Sum) Active Power
#define	P_MEAN_A			0xB1	// Phase A
#define	P_MEAN_B			0xB2	// Phase B
#define	P_MEAN_C			0xB3	// Phase C
#define	Q_MEAN_T			0xB4	// Total (All Phase Sum) Reactive Power
#define	Q_MEAN_A			0xB5	// Phase A
#define	Q_MEAN_B			0xB6	// Phase B
#define	Q_MEAN_C			0xB7	// Phase C
#define	SA_MEAN_T			0xB8	// Total (Arithmetic Sum) Apparent Power
#define	SA_MEAN_A			0xB9	// Phase A
#define	SA_MEAN_B			0xBA	// Phase B
#define	SA_MEAN_C			0xBB	// Phase C
#define	PF_MEAN_T			0xBC	// Total Power Factor
#define	PF_MEAN_A			0xBD	// Phase A
#define	PF_MEAN_B			0xBE	// Phase B
#define	PF_MEAN_C			0xBF	// Phase C

// Power LSB registers
#define	P_MEAN_T_LSB		0xC0	// Total (All Phase Sum) Active Power LSB
#define	P_MEAN_A_LSB		0xC1	// Phase A
#define	P_MEAN_B_LSB		0xC2	// Phase B
#define	P_MEAN_C_LSB		0xC3	// Phase C
#define	Q_MEAN_T_LSB		0xC4	// Total (All Phase Sum) Reactive Power LSB
#define	Q_MEAN_A_LSB		0xC5	// Phase A
#define	Q_MEAN_B_LSB		0xC6	// Phase B
#define	Q_MEAN_C_LSB		0xC7	// Phase C
#define	SA_MEAN_T_LSB		0xC8	// Total (Arithmetic Sum) Apparent Power LSB
#define	SA_MEAN_A_LSB		0xC9	// Phase A
#define	SA_MEAN_B_LSB		0xCA	// Phase B
#define	SA_MEAN_C_LSB		0xCB	// Phase C

#define	P_MEAN_TF			0xD0	// Total Active Fundamental Power
#define	P_MEAN_AF			0xD1	// Phase A
#define	P_MEAN_BF			0xD2	// Phase B
#define	P_MEAN_CF			0xD3	// Phase C
#define	P_MEAN_TH			0xD4	// Total Active Harmonic Power
#define	P_MEAN_AH			0xD5	// Phase A
#define	P_MEAN_BH			0xD6	// Phase B
#define	P_MEAN_CH			0xD7	// Phase C
#define	I_RMS_N1			0xD8	// NEUTRAL Sampled RMS Current
#define	V_RMS_A				0xD9	// Phase A RMS Voltage
#define	V_RMS_B				0xDA	// Phase B RMS Voltage
#define	V_RMS_C				0xDB	// Phase C RMS Voltage
#define	I_RMS_N0			0xDC	// NEUTRAL Calculated RMS Current
#define	I_RMS_A				0xDD	// Phase A RMS Current
#define	I_RMS_B				0xDE	// Phase B RMS Current
#define	I_RMS_C				0xDF	// Phase C RMS Current

#define	P_MEAN_TF_LSB		0xE0	// Total Active Fundamental Power LSB
#define	P_MEAN_AF_LSB		0xE1	// Phase A
#define	P_MEAN_BF_LSB		0xE2	// Phase B
#define	P_MEAN_CF_LSB		0xE3	// Phase C
#define	P_MEAN_TH_LSB		0xE4	// Total Active Harmonic Power LSB
#define	P_MEAN_AH_LSB		0xE5	// Phase A
#define	P_MEAN_BH_LSB		0xE6	// Phase B
#define	P_MEAN_CH_LSB		0xE7	// Phase C
//							0xE8	// UNDEFINED
#define	V_RMS_A_LSB			0xE9	// Phase A RMS Voltage LSB
#define	V_RMS_B_LSB			0xEA	// Phase B RMS Voltage
#define	V_RMS_C_LSB			0xEB	// Phase C RMS Voltage
//							0xEC	// UNDEFINED
#define	I_RMS_A_LSB			0xED	// Phase A RMS Current LSB
#define	I_RMS_B_LSB			0xEE	// Phase B RMS Current
#define	I_RMS_C_LSB			0xEF	// Phase C RMS Current

// THD+N, Frequency, Angle & Temperature  registers
#define	THD_NV_A			0xF1	// Phase A Voltage THD+N
#define	THD_NV_B			0xF2	// Phase B Voltage THD+N
#define	THD_NV_C			0xF3	// Phase C Voltage THD+N
#define	THD_NI_A			0xF5	// Phase A Current THD+N
#define	THD_NI_B			0xF6	// Phase B Current THD+N
#define	THD_NI_C			0xF7	// Phase C Current THD+N
#define FREQ				0xF8 	// Voltage Frequency
#define P_ANGLE_A			0xF9 	// Phase A Mean Power Angle
#define P_ANGLE_B			0xFA 	// Phase B Mean Power Angle
#define P_ANGLE_C			0xFB 	// Phase C Mean Power Angle
#define	TEMP				0xFC	// Chip Temperature
#define	V_ANGLE_A			0xFD	// Phase A Voltage phase angle
#define	V_ANGLE_B			0xFE	// Phase B Voltage phase angle
#define	V_ANGLE_C			0xFF	// Phase C Voltage phase angle

#define	CODE_DFALT				0x6886	// indicates default Power On status, not measuring
#define	CODE_START				0x5678
#define	CODE_CHECK				0x8765
#define	CODE_RESET				0x789A
#define	CODE_POWER				0xA987

// ######################################## Enumerations ###########################################

enum {
#if		(HAL_M90E36 > 0)
	M90E36_0,
#endif
#if		(HAL_M90E36 > 1)
	M90E36_1,
#endif
	M90E36_NUM,
};

enum {
	eCALSTART,
	ePLconstH,
	ePLconstL,
	eLgain,
	eLphi,
	eNgain,
	eNphi,
	ePStartTh,
	ePNolTh,
	eQStartTh,
	eQNolTh,
	eMMode,
	eCRC_1,
};


// ######################################### Structures ############################################

typedef union {
	struct {
		u8_t		Spare0	: 1;						// SYSSTATUS0
		u8_t		CS0Err	: 1;
		u8_t		Spare1	: 1;
		u8_t		CS1Err	: 1;
		u8_t		Spare2	: 1;
		u8_t		CS2Err	: 1;
		u8_t		Spare3	: 1;
		u8_t		CS3Err	: 1;
		u8_t		VrevWn	: 1;
		u8_t		IrevWn	: 1;
		u8_t		Spare4	: 2;
		u8_t		SagWarn	: 1;
		u8_t		PhLoseWn: 1;
		u8_t		Spare5	: 2;
		u8_t		INOv1	: 1;						// SYSSTATUS1
		u8_t		INOv0	: 1;
		u8_t		Spare6	: 2;
		u8_t		THDUOv	: 1;
		u8_t		THDIOv	: 1;
		u8_t		DFTdone	: 1;
		u8_t		Spare7	: 1;
		union {
			struct {
				u8_t		RevQchgT: 1;
				u8_t		RevQchgA: 1;
				u8_t		RevQchgB: 1;
				u8_t		RevQchgC: 1;
				u8_t		RevPchgT: 1;
				u8_t		RevPchgA: 1;
				u8_t		RevPchgB: 1;
				u8_t		RevPchgC: 1;
			};
			struct {
				u8_t		RevQ	: 4;
				u8_t		RevP	: 4;
			};
			u8_t		RevXchgY;
		};
	};
	struct {
		u16_t	x16a;
		u16_t	x16b;
	};
	ui32_t	x32;
} m90e36system_stat_t;

typedef union {
	struct {
		u8_t		TQNoload : 1;						// MET_STATUS0
		u8_t		TPNoload : 1;
		u8_t		TASNoload: 1;
		u8_t		TVSNoload: 1;
		u8_t		Spare0	 : 8;
		union {
			struct {
				u8_t		CF4RevFlg: 1;
				u8_t		CF3RevFlg: 1;
				u8_t		CF2RevFlg: 1;
				u8_t		CF1RevFlg: 1;
			};
			u8_t		CFxRevFlg: 4;
		};
		u8_t		Spare1	 : 8;						// MET_STATUS1
		union {
			struct {
				u8_t		Spare2	 : 1;
				u8_t		SagPhA	 : 1;
				u8_t		SagPhB	 : 1;
				u8_t		SagPhC	 : 1;
				u8_t		Spare3	 : 1;
				u8_t		LossPhA	 : 1;
				u8_t		LossPhB	 : 1;
				u8_t		LossPhC	 : 1;
			};
			struct {
				u8_t		Spare4	 : 1;
				u8_t		SagPhX	 : 3;
				u8_t		Spare5	 : 1;
				u8_t		LossPhX	 : 3;
			};
		};
	};
	struct {
		u16_t	x16a;
		u16_t	x16b;
	};
	u32_t	x32;
} m90e36Meter_stat_t;

typedef struct {
 	u8_t		addr;
 	u8_t		flag;
 	u16_t	raw_val;
 } conf_reg_t;

// See http://www.catb.org/esr/structure-packing/
// Also http://c0x.coding-guidelines.com/6.7.2.1.html

// ####################################### Global variables ########################################


// ####################################### Global functions ########################################

struct	epw_t;
i32_t	m90e36Identify(u8_t eChan);
i32_t	m90e36Init(u8_t eChan);
void	m90e36DataReadAll(u8_t eChan);
void	m90e36DataConvertAll(u8_t eChan);

i32_t	m90e36SetLiveGain(u8_t eChan, u8_t Gain);
i32_t	m90e36SetNeutralGain(u8_t eChan, u8_t Gain);

void	m90e36SetOffsetCompensation(u8_t eChan);

i32_t	m90e36SenseEnergy(struct epw_t *);
i32_t	m90e36SensePower(struct epw_t *);
i32_t	m90e36SenseVoltage(struct epw_t *);
i32_t	m90e36SenseCurrent(struct epw_t *);
i32_t	m90e36SenseTemperature(struct epw_t *);
i32_t	m90e36SensePowerFactor(struct epw_t *);
i32_t	m90e36SenseAngle(struct epw_t *);

ui32_t m90e36GetSysStatus(u8_t eChan);
ui32_t m90e36GetMeterStatus(u8_t eChan);

void	m90e36ReportStatus(u8_t eChan);
void	m90e36ReportData(u8_t eChan);
void	m90e36ReportCalib(u8_t eChan);
void	m90e36ReportAdjust(u8_t eChan);
void	m90e36Report(void);
void	m90e36Display(void);

#ifdef __cplusplus
}
#endif
