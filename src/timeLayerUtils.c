#include <pebble.h>
#include "timeLayerUtils.h"
  
#define TIME_FORMAT "00:00:00"

// void update_time(TextLayer *time_layer) {
//   // Get a tm structure
//   time_t temp = time(NULL);
//   struct tm *tick_time = localtime(&temp);

//   // Create a long-lived buffer
//   static char buffer[] = TIME_FORMAT;

//   // Write the current hours and minutes into the buffer
//   if(clock_is_24h_style() == true) {
//     //Use 2h hour format
//     strftime(buffer, sizeof(TIME_FORMAT), "%H:%M", tick_time);
//   } else {
//     //Use 12 hour format
//     strftime(buffer, sizeof(TIME_FORMAT), "%I:%M", tick_time);
//   }

//   // Display this time on the TextLayer
//   text_layer_set_text(time_layer, buffer);
// }

void loadTimeLayer(Window *windowLayer, TextLayer *timeLayer, const char* text) {
  text_layer_set_background_color(timeLayer, GColorClear);
  text_layer_set_text_color(timeLayer, GColorBlack);
  text_layer_set_text(timeLayer, text);

  // Improve the layout to be more like a watchface
  text_layer_set_font(timeLayer, fonts_get_system_font(FONT_KEY_DROID_SERIF_28_BOLD));
  text_layer_set_text_alignment(timeLayer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(windowLayer), text_layer_get_layer(timeLayer));
}

void destroyTimeLayer(TextLayer *timeLayer) {
    text_layer_destroy(timeLayer);
}