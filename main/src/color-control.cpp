#include "color-control.h"

// Convert a sound measurement in decibels to a color code
int leqToColor(float Leq_dB){
  if (Leq_dB < GREEN_UPPER_LIMIT) {
    return GREEN;
  }
  else if (Leq_dB < YELLOW_UPPER_LIMIT) {
    return YELLOW;
  }
  else {
    return RED;
  }
}

void resetArray(int* array, int arraySize, int value) {
  // Populate the array with copies of the same value
  for (int i=0; i<arraySize; i++) {
    array[i] = value;
  }
}

void appendToArray(int* array, int arraySize, int newValue) {
  // Append the new value to the array
  for (int i=0; i<arraySize-1; i++) {
    array[i] = array[i+1];
  }
  array[arraySize-1] = newValue;
}

int updateColorArray(int currentColor) {
  if (initColorArray) {
    resetArray(colorArray, COLOR_WINDOW_SIZE, currentColor);
    initColorArray = false;
    // Don't change the current color
    return currentColor;
  } else {
    // The array must be reset whenever a new measurement is lower
    // than the last one
    if (currentColor < colorArray[COLOR_WINDOW_SIZE-1]) {
      resetArray(colorArray, COLOR_WINDOW_SIZE, currentColor);
      // Don't change the current color
      return currentColor;
    } else {
      if (currentColor == RED && colorArray[COLOR_WINDOW_SIZE-1] == GREEN) {
        appendToArray(colorArray, COLOR_WINDOW_SIZE, currentColor);

        return currentColor;
      } else if (currentColor == RED && colorArray[COLOR_WINDOW_SIZE-1] == RED) {
        appendToArray(colorArray, COLOR_WINDOW_SIZE, currentColor);

        return currentColor;
      } else {
        appendToArray(colorArray, COLOR_WINDOW_SIZE, currentColor);

        // Calculate the average color
        // This is possible since colors are represented by integers in the range 0-2
        float averageColor = 0.0;
        for (int i=0; i<COLOR_WINDOW_SIZE; i++) {
          averageColor += colorArray[i];
        }
        averageColor /= float(COLOR_WINDOW_SIZE);

        // Convert the floating point average to one of the defined colors
        if (averageColor < 0.5) { // 0.0 <= averageColor < 0.5
          return GREEN;
        } else if (averageColor < 1.4) { // 0.5 <= averageColor < 1.4
          return YELLOW;
        } else { // averageColor >= 1.4
          return RED;
        }
      }
    }
  }
}

// Update the LED color
void setLEDColor(int color){
  analogWrite(GREEN_LED_PIN, 255);
  analogWrite(RED_LED_PIN, 255);
  analogWrite(BLUE_LED_PIN, 255);

  if(color == RED){
    analogWrite(RED_LED_PIN, 0);
  }
  else if(color == GREEN){
    analogWrite(GREEN_LED_PIN, 0);
  }
  else { // color == YELLOW
    analogWrite(GREEN_LED_PIN, 100);
    analogWrite(RED_LED_PIN, 0);
  }
}