#include <pebble.h>
#include "timeLayerUtils.h"
  
#define TIME_FORMAT "00:00:00"

void update_time(TextLayer *time_layer) {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = TIME_FORMAT;

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    //Use 2h hour format
    strftime(buffer, sizeof(TIME_FORMAT), "%H:%M", tick_time);
  } else {
    //Use 12 hour format
    strftime(buffer, sizeof(TIME_FORMAT), "%I:%M", tick_time);
  }

  // Display this time on the TextLayer
  text_layer_set_text(time_layer, buffer);
}

void load_time_layer(Window *window_layer, TextLayer *time_layer, const char* text) {
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_text_color(time_layer, GColorBlack);
  text_layer_set_text(time_layer, text);

  // Improve the layout to be more like a watchface
  text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_DROID_SERIF_28_BOLD));
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window_layer), text_layer_get_layer(time_layer));
}

void destroy_time_layer(TextLayer *time_layer) {
    text_layer_destroy(time_layer);
}