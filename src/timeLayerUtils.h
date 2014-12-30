#ifndef TIME_LAYER_UTILS_H
#define TIME_LAYER_UTILS_H
  
#include <pebble.h>

void load_time_layer(Window *window_layer, TextLayer *time_layer, const char* text);
void update_time(TextLayer *time_layer);
void destroy_time_layer(TextLayer *time_layer);

#endif
