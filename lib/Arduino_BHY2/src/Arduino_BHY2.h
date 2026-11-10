#pragma once

/**
 * @file Arduino_BHY2.h
 * @brief Cabecera unificada y podada de Arduino_BHY2.
 * 
 * Se han eliminado los módulos pesados e innecesarios (BLEHandler, DFUManager y EslovHandler)
 * para liberar memoria RAM estática (.bss) y Heap en el nRF52832.
 */

#include "Arduino.h"
#include "BoschSensortec.h"
#include "BoschParser.h"
#include "sensors/SensorManager.h"
#include "sensors/SensorID.h"
#include "sensors/SensorTypes.h"
#include "sensors/SensorXYZ.h"
