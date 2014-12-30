#include <pebble.h>
#include <timeLayerUtils.h>  
  
//Globals  
#define MAXIMUM_NUMBER_OF_LAPS 200
#define TICK_INTERVAL 10
#define LAP_DISPLAY_DURATION 2000
#define LAP_LAYER_TEXT "Lap 001"
#define TIME_SEC_LAYER_TEXT "00:00:00"
#define TIME_MILLI_SEC_LAYER_TEXT "000"
  
static Window *sMainWindow;
static TextLayer *sLapLayer;
static TextLayer *sTimeSecLayer;
static TextLayer *sTimeMilliSecLayer;
static bool s_isRecording = false;
static bool s_isNewSession = true;
static time_t s_oldTimeInSec = 0;
static time_t s_recordTimingInSec = 0;
static uint16_t s_recordTimingInMilliSec = 0;
static time_t s_startTimeInSec = 0;
static uint16_t s_startTimeInMilliSec = 0;
static const int s_maxLaps = MAXIMUM_NUMBER_OF_LAPS;
static time_t *s_lapTimesInSec;
static uint16_t *s_lapTimesInMilliSec;
static int s_recordLapIndex = 0;
static int s_reviewLapIndex = 0;
static AppTimer *s_stopWatchTimer;
static AppTimer *s_lapDisplayTimer;
static uint32_t s_tickInterval = TICK_INTERVAL;
static uint32_t s_lapDisplayDuration = LAP_DISPLAY_DURATION;
static bool s_lapIndexChanged = false;
static bool s_secTimeChanged = false;
static bool s_milliSecTimeChanged = false;
static bool s_freezeDisplay = false;

//Main Window Functions
static void main_window_load(Window *window) {
  // Create time TextLayer
  sLapLayer = text_layer_create(GRect(0, 10, 144, 40));
  sTimeSecLayer = text_layer_create(GRect(0, 45, 144, 40));
  sTimeMilliSecLayer = text_layer_create(GRect(0, 80, 144, 40));

  // Load the time layer
  load_time_layer(window, sLapLayer, LAP_LAYER_TEXT);
  load_time_layer(window, sTimeSecLayer, TIME_SEC_LAYER_TEXT);
  load_time_layer(window, sTimeMilliSecLayer, TIME_MILLI_SEC_LAYER_TEXT);
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  destroy_time_layer(sLapLayer);
  destroy_time_layer(sTimeSecLayer);
  destroy_time_layer(sTimeMilliSecLayer);
}

void displayTime(time_t displayTimeInSec, uint16_t displayTimeInMilliSec, int lapIndex) {
  if (s_freezeDisplay) {

  }
  else {
    if (s_lapIndexChanged) {
      static char layerBuffer[] = LAP_LAYER_TEXT;
      snprintf(layerBuffer, sizeof("Lap 001"), "Lap %03d", (lapIndex + 1));
      text_layer_set_text(sLapLayer, layerBuffer);
      s_lapIndexChanged = false;
    }
    
    if (s_secTimeChanged) {
      static char secBuffer[] = TIME_SEC_LAYER_TEXT;
      struct tm *tick_time = localtime(&displayTimeInSec);
      strftime(secBuffer, sizeof(TIME_SEC_LAYER_TEXT), "%H:%M:%S", tick_time);
      text_layer_set_text(sTimeSecLayer, secBuffer);
  	s_secTimeChanged = false;
    }
    
    if (s_milliSecTimeChanged) {
      static char milliSecBuffer[] = TIME_MILLI_SEC_LAYER_TEXT;
      snprintf(milliSecBuffer, sizeof(TIME_MILLI_SEC_LAYER_TEXT), "%03d", displayTimeInMilliSec);
      text_layer_set_text(sTimeMilliSecLayer, milliSecBuffer);
  	s_milliSecTimeChanged = false;
    }
  }
}

void incrementRecordLapIndex() {
  if (++s_recordLapIndex == s_maxLaps) {
    --s_recordLapIndex;
  }
  
  s_lapIndexChanged = true;
}

void refreshDisplay() {
  s_lapIndexChanged = true;
  s_secTimeChanged = true;
  s_milliSecTimeChanged = true;
}

void incrementReviewLapIndex() {
  ++s_reviewLapIndex;
  if (s_reviewLapIndex == s_recordLapIndex) {
    s_reviewLapIndex = 0;
  }
  
  refreshDisplay();
}

void decrementReviewLapIndex() {
  --s_reviewLapIndex;
  if (s_reviewLapIndex < 0) {
    s_reviewLapIndex = s_recordLapIndex - 1;
  }
  
  refreshDisplay();
}

void resetLapTimes() {
  for (int i = 0; i < s_maxLaps; ++i) {
    s_lapTimesInSec[i] = 0;
    s_lapTimesInMilliSec[i] = 0;
  }
  
  s_recordLapIndex = 0;
  s_reviewLapIndex = 0;
}

static void calculateTimeSinceStart() {
  time_t timeInSec;
  uint16_t timeInMilliSec;
  time_ms(&timeInSec, &timeInMilliSec);
  if (timeInMilliSec < s_startTimeInMilliSec) {
    s_recordTimingInMilliSec = timeInMilliSec + 1000 - s_startTimeInMilliSec;
    timeInSec -= 1;
  }
  else {
    s_recordTimingInMilliSec = timeInMilliSec - s_startTimeInMilliSec;
  }
  
  s_milliSecTimeChanged = true;
  
  if (s_oldTimeInSec != timeInSec) {
    s_secTimeChanged = true;
    s_recordTimingInSec = timeInSec - s_startTimeInSec;
	s_oldTimeInSec = timeInSec;
  }
}

static void setStartTime() {
  time_ms(&s_startTimeInSec, &s_startTimeInMilliSec);
}
 
static void stopWatchTimerCallback(void *data) {
  if (s_isRecording) {
    calculateTimeSinceStart();
    displayTime(s_recordTimingInSec, s_recordTimingInMilliSec, s_recordLapIndex);

    s_stopWatchTimer = app_timer_register(s_tickInterval, stopWatchTimerCallback, NULL);
  }
}

static void thawDisplayCallback(void *data) {
  s_freezeDisplay = false;
  displayTime(s_recordTimingInSec, s_recordTimingInMilliSec, s_recordLapIndex);
}

void recordLapTime(time_t lapTimeInSec, uint16_t lapTimeInMilliSec) {
  s_lapTimesInSec[s_recordLapIndex] = lapTimeInSec;
  s_lapTimesInMilliSec[s_recordLapIndex] = lapTimeInMilliSec;
  incrementRecordLapIndex();
  s_reviewLapIndex = s_recordLapIndex - 1;
}

void startStopWatch(bool isStart) {
  s_isRecording = isStart;
  if (s_isRecording) {
  	if (s_isNewSession) {
  	  setStartTime();
  	  s_isNewSession = false;
  	}
	
    refreshDisplay();
    displayTime(s_recordTimingInSec, s_recordTimingInMilliSec, s_recordLapIndex);
    s_stopWatchTimer = app_timer_register(s_tickInterval, stopWatchTimerCallback, NULL);
  }
  else {
    app_timer_cancel(s_stopWatchTimer);
    recordLapTime(s_recordTimingInSec, s_recordTimingInMilliSec);
  }
}

void upSingleClickHandler(ClickRecognizerRef recognizer, void *context) {
  if (s_isRecording) {
    startStopWatch(false);
  }
  else {
    if (s_isNewSession) {

	}
	else {
	  incrementReviewLapIndex();
      displayTime(s_lapTimesInSec[s_reviewLapIndex], s_lapTimesInMilliSec[s_reviewLapIndex], s_reviewLapIndex);
	}
  }
}

void selectSingleClickHandler(ClickRecognizerRef recognizer, void *context) {
  if (s_isRecording) {

  }
  else {
    startStopWatch(true);
  }
}

void downSingleClickHandler(ClickRecognizerRef recognizer, void *context) {
  if (s_isRecording) {
    recordLapTime(s_recordTimingInSec, s_recordTimingInMilliSec);
	s_freezeDisplay = true;
	s_lapDisplayTimer =  app_timer_register(s_lapDisplayDuration, thawDisplayCallback, NULL);
	displayTime(s_lapTimesInSec[s_reviewLapIndex], s_lapTimesInMilliSec[s_reviewLapIndex], s_reviewLapIndex);
  }
  else {
    if (s_isNewSession) {
	
	}
	else {
      decrementReviewLapIndex();
      displayTime(s_lapTimesInSec[s_reviewLapIndex], s_lapTimesInMilliSec[s_reviewLapIndex], s_reviewLapIndex);
	}
  }
}

void selectMultiClickHandler(ClickRecognizerRef recognizer, void *context) {
  if (s_isRecording) {

  }
  else {
	s_isNewSession = true;
    s_recordTimingInSec = 0;
	s_recordTimingInMilliSec = 0;
    resetLapTimes();
    refreshDisplay();
    displayTime(s_recordTimingInSec, s_recordTimingInMilliSec, s_recordLapIndex);
  }
}

void config_provider(Window *window) {
  s_recordTimingInSec = 0;
 // single click / repeat-on-hold config:
  window_single_click_subscribe(BUTTON_ID_UP, upSingleClickHandler);
  window_single_click_subscribe(BUTTON_ID_SELECT, selectSingleClickHandler);
  window_single_click_subscribe(BUTTON_ID_DOWN, downSingleClickHandler);
  window_multi_click_subscribe(BUTTON_ID_SELECT, 2, 10, 0, true, selectMultiClickHandler);
}

static void init() {
  // Create main Window element and assign to pointer
  sMainWindow = window_create();

  s_lapTimesInSec = malloc(s_maxLaps * sizeof(time_t));
  s_lapTimesInMilliSec = malloc(s_maxLaps * sizeof(uint16_t));
  resetLapTimes();
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(sMainWindow, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(sMainWindow, true);
  
  // Set up click button handler
  window_set_click_config_provider(sMainWindow, (ClickConfigProvider) config_provider);
}

static void deinit() {
  // Destroy Window
  window_destroy(sMainWindow);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
