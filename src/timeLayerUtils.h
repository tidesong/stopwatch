#ifndef TIME_LAYER_UTILS_H
#define TIME_LAYER_UTILS_H
  
#include <pebble.h>

void loadTimeLayer(Window *windowLayer, TextLayer *timeLayer, const char* text);
//void update_time(TextLayer *time_layer);
void destroyTimeLayer(TextLayer *timeLayer);

#endif
