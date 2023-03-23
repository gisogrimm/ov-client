/*=========================================================================
    POINTER REGISTER
    -----------------------------------------------------------------------*/
#define ADS1115_REG_POINTER_MASK        (0x03)
#define ADS1115_REG_POINTER_CONVERT     (0x00)
#define ADS1115_REG_POINTER_CONFIG      (0x01)
#define ADS1115_REG_POINTER_LOWTHRESH   (0x02)
#define ADS1115_REG_POINTER_HITHRESH    (0x03)
/*=========================================================================*/

/**************************************************************************/
/*!
  @brief  Abstract away platform differences in Arduino wire library
*/
/**************************************************************************/
static uint8_t i2cread(void) {
  return Wire.read();
}

/**************************************************************************/
/*!
  @brief  Abstract away platform differences in Arduino wire library
*/
/**************************************************************************/
static void i2cwrite(uint8_t x) {
  Wire.write((uint8_t)x);
}

/**************************************************************************/
/*!
  @brief  Writes 16-bits to the specified destination register
*/
/**************************************************************************/
static void writeRegister(uint8_t i2cAddress, uint8_t reg, uint16_t value) {
  Wire.beginTransmission(i2cAddress);
  i2cwrite((uint8_t)reg);
  i2cwrite((uint8_t)(value >> 8));
  i2cwrite((uint8_t)(value & 0xFF));
  Wire.endTransmission();
}

/**************************************************************************/
/*!
  @brief  Writes 16-bits to the specified destination register
*/
/**************************************************************************/
static uint16_t readRegister(uint8_t i2cAddress, uint8_t reg) {
  Wire.beginTransmission(i2cAddress);
  i2cwrite(ADS1115_REG_POINTER_CONVERT);
  Wire.endTransmission();
  Wire.requestFrom(i2cAddress, (uint8_t)2);
  return ((i2cread() << 8) | i2cread());
}

/*=========================================================================
  POINTER REGISTER
  -----------------------------------------------------------------------*/
#define ADS1115_REG_POINTER_MASK        (0x03)
#define ADS1115_REG_POINTER_CONVERT     (0x00)
#define ADS1115_REG_POINTER_CONFIG      (0x01)
#define ADS1115_REG_POINTER_LOWTHRESH   (0x02)
#define ADS1115_REG_POINTER_HITHRESH    (0x03)
/*=========================================================================*/

/*=========================================================================
  CONFIG REGISTER
  -----------------------------------------------------------------------*/
#define ADS1115_REG_CONFIG_OS_MASK      (0x8000)
#define ADS1115_REG_CONFIG_OS_SINGLE    (0x8000)  // Write: Set to start a single-conversion
#define ADS1115_REG_CONFIG_OS_BUSY      (0x0000)  // Read: Bit = 0 when conversion is in progress
#define ADS1115_REG_CONFIG_OS_NOTBUSY   (0x8000)  // Read: Bit = 1 when device is not performing a conversion

#define ADS1115_REG_CONFIG_MUX_MASK     (0x7000)
#define ADS1115_REG_CONFIG_MUX_DIFF_0_1 (0x0000)  // Differential P = AIN0, N = AIN1 (default)
#define ADS1115_REG_CONFIG_MUX_DIFF_0_3 (0x1000)  // Differential P = AIN0, N = AIN3
#define ADS1115_REG_CONFIG_MUX_DIFF_1_3 (0x2000)  // Differential P = AIN1, N = AIN3
#define ADS1115_REG_CONFIG_MUX_DIFF_2_3 (0x3000)  // Differential P = AIN2, N = AIN3
#define ADS1115_REG_CONFIG_MUX_SINGLE_0 (0x4000)  // Single-ended AIN0
#define ADS1115_REG_CONFIG_MUX_SINGLE_1 (0x5000)  // Single-ended AIN1
#define ADS1115_REG_CONFIG_MUX_SINGLE_2 (0x6000)  // Single-ended AIN2
#define ADS1115_REG_CONFIG_MUX_SINGLE_3 (0x7000)  // Single-ended AIN3

#define ADS1115_REG_CONFIG_PGA_MASK     (0x0E00)
#define ADS1115_REG_CONFIG_PGA_6_144V   (0x0000)  // +/-6.144V range = Gain 2/3
#define ADS1115_REG_CONFIG_PGA_4_096V   (0x0200)  // +/-4.096V range = Gain 1
#define ADS1115_REG_CONFIG_PGA_2_048V   (0x0400)  // +/-2.048V range = Gain 2 (default)
#define ADS1115_REG_CONFIG_PGA_1_024V   (0x0600)  // +/-1.024V range = Gain 4
#define ADS1115_REG_CONFIG_PGA_0_512V   (0x0800)  // +/-0.512V range = Gain 8
#define ADS1115_REG_CONFIG_PGA_0_256V   (0x0A00)  // +/-0.256V range = Gain 16
#define ADS1115_REG_CONFIG_PGA_0_256VB   (0x0E00)  // +/-0.256V range = Gain 16

#define ADS1115_REG_CONFIG_MODE_MASK    (0x0100)
#define ADS1115_REG_CONFIG_MODE_CONTIN  (0x0000)  // Continuous conversion mode
#define ADS1115_REG_CONFIG_MODE_SINGLE  (0x0100)  // Power-down single-shot mode (default)

#define ADS1115_REG_CONFIG_DR_MASK      (0x00E0)
#define ADS1115_REG_CONFIG_DR_8SPS     (0x0000)  // 8 samples per second
#define ADS1115_REG_CONFIG_DR_16SPS    (0x0020)  // 16 samples per second
#define ADS1115_REG_CONFIG_DR_32SPS    (0x0040)  // 32 samples per second
#define ADS1115_REG_CONFIG_DR_64SPS    (0x0060)  // 64 samples per second
#define ADS1115_REG_CONFIG_DR_128SPS   (0x0080)  // 128 samples per second (default)
#define ADS1115_REG_CONFIG_DR_250SPS   (0x00A0)  // 250 samples per second
#define ADS1115_REG_CONFIG_DR_475SPS   (0x00C0)  // 475 samples per second
#define ADS1115_REG_CONFIG_DR_860SPS   (0x00E0)  // 860 samples per second

#define ADS1115_REG_CONFIG_CMODE_MASK   (0x0010)
#define ADS1115_REG_CONFIG_CMODE_TRAD   (0x0000)  // Traditional comparator with hysteresis (default)
#define ADS1115_REG_CONFIG_CMODE_WINDOW (0x0010)  // Window comparator

#define ADS1115_REG_CONFIG_CPOL_MASK    (0x0008)
#define ADS1115_REG_CONFIG_CPOL_ACTVLOW (0x0000)  // ALERT/RDY pin is low when active (default)
#define ADS1115_REG_CONFIG_CPOL_ACTVHI  (0x0008)  // ALERT/RDY pin is high when active

#define ADS1115_REG_CONFIG_CLAT_MASK    (0x0004)  // Determines if ALERT/RDY pin latches once asserted
#define ADS1115_REG_CONFIG_CLAT_NONLAT  (0x0000)  // Non-latching comparator (default)
#define ADS1115_REG_CONFIG_CLAT_LATCH   (0x0004)  // Latching comparator

#define ADS1115_REG_CONFIG_CQUE_MASK    (0x0003)
#define ADS1115_REG_CONFIG_CQUE_1CONV   (0x0000)  // Assert ALERT/RDY after one conversions
#define ADS1115_REG_CONFIG_CQUE_2CONV   (0x0001)  // Assert ALERT/RDY after two conversions
#define ADS1115_REG_CONFIG_CQUE_4CONV   (0x0002)  // Assert ALERT/RDY after four conversions
#define ADS1115_REG_CONFIG_CQUE_NONE    (0x0003)  // Disable the comparator and put ALERT/RDY in high state (default)
/*=========================================================================
    I2C ADDRESS/BITS
    -----------------------------------------------------------------------*/
#define ADS1115_ADDRESS                 (0x48)    // 1001 000 (ADDR = GND)
/*=========================================================================*/

/*=========================================================================*/

void ads1115_configure() {
  // Start with default values
  uint16_t config =
    ADS1115_REG_CONFIG_CQUE_NONE    | // Disable the comparator (default val)
    ADS1115_REG_CONFIG_CLAT_NONLAT  | // Non-latching (default val)
    ADS1115_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
    ADS1115_REG_CONFIG_CMODE_TRAD   | // Traditional comparator (default val)
    ADS1115_REG_CONFIG_DR_64SPS   | // 250 samples per second (default)
    ADS1115_REG_CONFIG_MUX_DIFF_2_3 |
    ADS1115_REG_CONFIG_MODE_CONTIN |
    ADS1115_REG_CONFIG_PGA_0_256V ;

  // Write config register to the ADC
  writeRegister(ADS1115_ADDRESS, ADS1115_REG_POINTER_CONFIG, config);
}

int16_t ads1115_read() {
  // Read the conversion results
  uint16_t res = readRegister(ADS1115_ADDRESS, ADS1115_REG_POINTER_CONVERT);
  return (int16_t)res;
}
