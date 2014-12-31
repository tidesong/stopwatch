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
static bool sIsRecording = false;
static bool sIsNewSession = true;
static time_t sOldTimeInSec = 0;
static time_t sRecordTimingInSec = 0;
static uint16_t sRecordTimingInMilliSec = 0;
static time_t sStartTimeInSec = 0; 
static uint16_t sStartTimeInMilliSec = 0;
static const int sMaxLaps = MAXIMUM_NUMBER_OF_LAPS;
static time_t *sLapTimesInSec;
static uint16_t *sLapTimesInMilliSec;
static int sRecordLapIndex = 0;
static int sReviewLapIndex = 0;
static AppTimer *sStopWatchTimer;
static AppTimer *sLapDisplayTimer;
static uint32_t sTickInterval = TICK_INTERVAL;
static uint32_t sLapDisplayDuration = LAP_DISPLAY_DURATION;
static bool sLapIndexChanged = false;
static bool sSecTimeChanged = false;
static bool sMilliSecTimeChanged = false;
static bool sFreezeDisplay = false;

//Main Window Functions
static void mainWindowLoad(Window *window) {
  // Create time TextLayer
  sLapLayer = text_layer_create(GRect(0, 10, 144, 40));
  sTimeSecLayer = text_layer_create(GRect(0, 45, 144, 40));
  sTimeMilliSecLayer = text_layer_create(GRect(0, 80, 144, 40));

  // Load the time layer
  loadTimeLayer(window, sLapLayer, LAP_LAYER_TEXT);
  loadTimeLayer(window, sTimeSecLayer, TIME_SEC_LAYER_TEXT);
  loadTimeLayer(window, sTimeMilliSecLayer, TIME_MILLI_SEC_LAYER_TEXT);
}

static void mainWindowUnload(Window *window) {
  // Destroy TextLayer
  destroyTimeLayer(sLapLayer);
  destroyTimeLayer(sTimeSecLayer);
  destroyTimeLayer(sTimeMilliSecLayer);
}

void displayTime(time_t displayTimeInSec, uint16_t displayTimeInMilliSec, int lapIndex) {
  if (sFreezeDisplay) {

  }
  else {
    if (sLapIndexChanged) {
      static char layerBuffer[] = LAP_LAYER_TEXT;
      snprintf(layerBuffer, sizeof("Lap 001"), "Lap %03d", (lapIndex + 1));
      text_layer_set_text(sLapLayer, layerBuffer);
      //reset LapIndexChanged
      sLapIndexChanged = false;
    }
    
    if (sSecTimeChanged) {
      static char secBuffer[] = TIME_SEC_LAYER_TEXT;
      struct tm *tick_time = localtime(&displayTimeInSec);
      strftime(secBuffer, sizeof(TIME_SEC_LAYER_TEXT), "%H:%M:%S", tick_time);
      text_layer_set_text(sTimeSecLayer, secBuffer);
      //reset SecTimeChanged
    	sSecTimeChanged = false;
    }
    
    if (sMilliSecTimeChanged) {
      static char milliSecBuffer[] = TIME_MILLI_SEC_LAYER_TEXT;
      snprintf(milliSecBuffer, sizeof(TIME_MILLI_SEC_LAYER_TEXT), "%03d", displayTimeInMilliSec);
      text_layer_set_text(sTimeMilliSecLayer, milliSecBuffer);
      //reset MillisecTimeChanged
    	sMilliSecTimeChanged = false;
    }
  }
}

void incrementRecordLapIndex() {
  if (++sRecordLapIndex == sMaxLaps) {
    --sRecordLapIndex;
  }  
  //set LapIndexChanged
  sLapIndexChanged = true;
}

void refreshDisplay() {
  sLapIndexChanged = true;
  sSecTimeChanged = true;
  sMilliSecTimeChanged = true;
}

void incrementReviewLapIndex() {
  ++sReviewLapIndex;
  if (sReviewLapIndex == sRecordLapIndex) {
    sReviewLapIndex = 0;
  }
  
  refreshDisplay();
}

void decrementReviewLapIndex() {
  --sReviewLapIndex;
  if (sReviewLapIndex < 0) {
    sReviewLapIndex = sRecordLapIndex - 1;
  }
  
  refreshDisplay();
}

void resetLapTimes() {
  for (int i = 0; i < sMaxLaps; ++i) {
    sLapTimesInSec[i] = 0;
    sLapTimesInMilliSec[i] = 0;
  }
  
  sRecordLapIndex = 0;
  sReviewLapIndex = 0;
}

static void calculateTimeSinceStart() {
  time_t timeInSec;
  uint16_t timeInMilliSec;
  time_ms(&timeInSec, &timeInMilliSec);
  if (timeInMilliSec < sStartTimeInMilliSec) {
    sRecordTimingInMilliSec = timeInMilliSec + 1000 - sStartTimeInMilliSec;
    timeInSec -= 1;
  }
  else {
    sRecordTimingInMilliSec = timeInMilliSec - sStartTimeInMilliSec;
  }
  
  sMilliSecTimeChanged = true;
  
  if (sOldTimeInSec != timeInSec) {
    sSecTimeChanged = true;
    sRecordTimingInSec = timeInSec - sStartTimeInSec;
	sOldTimeInSec = timeInSec;
  }
}

static void setStartTime() {
  time_ms(&sStartTimeInSec, &sStartTimeInMilliSec);
}
 
static void stopWatchTimerCallback(void *data) {
  if (sIsRecording) {
    calculateTimeSinceStart();
    displayTime(sRecordTimingInSec, sRecordTimingInMilliSec, sRecordLapIndex);

    sStopWatchTimer = app_timer_register(sTickInterval, stopWatchTimerCallback, NULL);
  }
}

static void thawDisplayCallback(void *data) {
  sFreezeDisplay = false;
  displayTime(sRecordTimingInSec, sRecordTimingInMilliSec, sRecordLapIndex);
}

void recordLapTime(time_t lapTimeInSec, uint16_t lapTimeInMilliSec) {
  sLapTimesInSec[sRecordLapIndex] = lapTimeInSec;
  sLapTimesInMilliSec[sRecordLapIndex] = lapTimeInMilliSec;
  incrementRecordLapIndex();
  sReviewLapIndex = sRecordLapIndex - 1;
}

void startStopWatch(bool isStart) {
  sIsRecording = isStart;
  if (sIsRecording) {
  	if (sIsNewSession) {
  	  setStartTime();
  	  sIsNewSession = false;
  	}
	
    refreshDisplay();
    displayTime(sRecordTimingInSec, sRecordTimingInMilliSec, sRecordLapIndex);
    sStopWatchTimer = app_timer_register(sTickInterval, stopWatchTimerCallback, NULL);
  }
  else {
    app_timer_cancel(sStopWatchTimer);
    recordLapTime(sRecordTimingInSec, sRecordTimingInMilliSec);
  }
}

void upSingleClickHandler(ClickRecognizerRef recognizer, void *context) {
  if (sIsRecording) {
    startStopWatch(false);
  }
  else {
    if (sIsNewSession) {

	}
	else {
	  incrementReviewLapIndex();
      displayTime(sLapTimesInSec[sReviewLapIndex], sLapTimesInMilliSec[sReviewLapIndex], sReviewLapIndex);
	}
  }
}

void selectSingleClickHandler(ClickRecognizerRef recognizer, void *context) {
  if (sIsRecording) {

  }
  else {
    startStopWatch(true);
  }
}

void downSingleClickHandler(ClickRecognizerRef recognizer, void *context) {
  if (sIsRecording) {
    recordLapTime(sRecordTimingInSec, sRecordTimingInMilliSec);
	sFreezeDisplay = true;
	sLapDisplayTimer =  app_timer_register(sLapDisplayDuration, thawDisplayCallback, NULL);
	displayTime(sLapTimesInSec[sReviewLapIndex], sLapTimesInMilliSec[sReviewLapIndex], sReviewLapIndex);
  }
  else {
    if (sIsNewSession) {
	
	}
	else {
      decrementReviewLapIndex();
      displayTime(sLapTimesInSec[sReviewLapIndex], sLapTimesInMilliSec[sReviewLapIndex], sReviewLapIndex);
	}
  }
}

void selectMultiClickHandler(ClickRecognizerRef recognizer, void *context) {
  if (sIsRecording) {

  }
  else {
  	sIsNewSession = true;
    sRecordTimingInSec = 0;
  	sRecordTimingInMilliSec = 0;
    resetLapTimes();
    refreshDisplay();
    displayTime(sRecordTimingInSec, sRecordTimingInMilliSec, sRecordLapIndex);
  }
}

void config_provider(Window *window) {
  sRecordTimingInSec = 0;
 // single click / repeat-on-hold config:
  window_single_click_subscribe(BUTTON_ID_UP, upSingleClickHandler);
  window_single_click_subscribe(BUTTON_ID_SELECT, selectSingleClickHandler);
  window_single_click_subscribe(BUTTON_ID_DOWN, downSingleClickHandler);
  window_multi_click_subscribe(BUTTON_ID_SELECT, 2, 10, 0, true, selectMultiClickHandler);
}

static void init() {
  // Create main Window element and assign to pointer
  sMainWindow = window_create();

  sLapTimesInSec = malloc(sMaxLaps * sizeof(time_t));
  sLapTimesInMilliSec = malloc(sMaxLaps * sizeof(uint16_t));
  resetLapTimes();
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(sMainWindow, (WindowHandlers) {
    .load = mainWindowLoad,
    .unload = mainWindowUnload
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
