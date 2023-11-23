// Parameter file for the dB meter
// These are in a separate file to avoid updating the versioned main.ino file just to change them

#ifndef PARAMS_H
#define PARAMS_H

#define GREEN_UPPER_LIMIT 67.0
#define YELLOW_UPPER_LIMIT 77.0

#define LEQ_PERIOD        16.0      // second(s)
#define WEIGHTING         A_weighting // 'A_weighting' 'C_weighting' or 'None' (Z_weighting)
#define LEQ_UNITS         "LAeq"      // customize based on above weighting used
#define DB_UNITS          "dBA"       // customize based on above weighting used
#define MIC_OFFSET_DB     2.9         // Offset (sine-wave RMS vs. dBFS). Modify this value for linear calibration. Default is 3.0103

#endif