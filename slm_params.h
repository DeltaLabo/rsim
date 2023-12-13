// Parameter file for the dB meter
// These are in a separate file to avoid updating the versioned main.ino file just to change them

#ifndef PARAMS_H
#define PARAMS_H

#define GREEN_UPPER_LIMIT  55.0 // dBA, maximum noise level for which the indicator stays green
#define YELLOW_UPPER_LIMIT 70.0 // dBA, maximum noise level for which the indicator stays yellow

#define LOGGING_PERIOD 64.0 // second(s)

#define LEQ_PERIOD        2.0      // second(s)
#define WEIGHTING         A_weighting // 'A_weighting' 'C_weighting' or 'None' (Z_weighting)
#define LEQ_UNITS         "LAeq"      // customize based on above weighting used
#define DB_UNITS          "dBA"       // customize based on above weighting used
#define MIC_OFFSET_DB     1.3         // Offset (sine-wave RMS vs. dBFS). Modify this value for linear calibration.

#endif