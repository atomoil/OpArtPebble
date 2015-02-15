#include <pebble.h>

#define CIRCLE_WIDTH 5
#define CIRCLE_WIDTH2 (2 * CIRCLE_WIDTH)
#define LENGTH_ADJUST (CIRCLE_WIDTH - 1)

static Window *window;
static Layer *faceLayer;
static Layer *hourLayer;
static Layer *minuteLayer;
static Layer *secondLayer;

static void circleLayerUpdate(Layer *layer, GContext *context);
static void circleLayerUpdateAlt(Layer *layer, GContext *context);

static void tickHandler(struct tm *tick_time, TimeUnits units_changed);

static GPoint faceCenterForHour;
static GPoint hourCenterForMinute;
static GPoint minuteCenterForSecond;
static uint16_t hourLength;
static uint16_t minuteLength;
static uint16_t secondLength;
static uint16_t hourHalfWidth;
static uint16_t minuteHalfWidth;
static uint16_t secondHalfWidth;

static void window_load(Window *window)
{
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  GRect faceBounds = bounds;
  if (faceBounds.size.w > faceBounds.size.h)
  {
    faceBounds.origin.x += (faceBounds.size.w - faceBounds.size.h) >> 1;
    if ((1 & faceBounds.size.h) == 0)
      --faceBounds.size.h;
    faceBounds.size.w = faceBounds.size.h;
  }
  else
  {
    faceBounds.origin.y += (faceBounds.size.h - faceBounds.size.w) >> 1;
    if ((1 & faceBounds.size.w) == 0)
      --faceBounds.size.w;
    faceBounds.size.h = faceBounds.size.w;
  }
  faceLayer = layer_create(faceBounds);
  layer_set_update_proc(faceLayer,circleLayerUpdate);
  layer_add_child(window_layer, faceLayer);

  uint16_t hourSize = 1 | (((faceBounds.size.h - CIRCLE_WIDTH2) * 10) >> 4);
  hourLength = faceBounds.size.h/2 - hourSize/2 - LENGTH_ADJUST;
  GRect hourBounds = (GRect){.origin={.x=0,.y=0},.size={.w=hourSize,.h=hourSize}};
  hourLayer = layer_create(hourBounds);
  layer_set_update_proc(hourLayer,circleLayerUpdateAlt);
  layer_add_child(faceLayer, hourLayer);

  uint16_t faceCenter = (faceBounds.size.h) >> 1;
  faceCenterForHour = (GPoint){.x=faceCenter, .y=faceCenter};
  hourHalfWidth = hourSize >> 1;

  uint16_t minuteSize = 1 | (((hourBounds.size.h - CIRCLE_WIDTH2) * 10) >> 4);
  minuteLength = ((hourBounds.size.h - minuteSize) >> 1) - LENGTH_ADJUST;
  GRect minuteBounds = (GRect){.origin={.x=0,.y=0},.size={.w=minuteSize,.h=minuteSize}};
  minuteLayer = layer_create(minuteBounds);
  layer_set_update_proc(minuteLayer,circleLayerUpdate);
  layer_add_child(hourLayer, minuteLayer);

  uint16_t hourCenter = (hourBounds.size.h) >> 1;
  hourCenterForMinute = (GPoint){.x=hourCenter, .y=hourCenter};
  minuteHalfWidth = minuteSize >> 1;
  
  // seconds
  uint16_t secondSize = 1 | (((minuteBounds.size.h - CIRCLE_WIDTH2) * 10) >> 4);
  secondLength = ((minuteBounds.size.h - secondSize) >> 1) - LENGTH_ADJUST;
  GRect secondBounds = (GRect){.origin={.x=0,.y=0},.size={.w=secondSize,.h=secondSize}};
  secondLayer = layer_create(secondBounds);
  layer_set_update_proc(secondLayer,circleLayerUpdateAlt);
  layer_add_child(minuteLayer, secondLayer);

  uint16_t minuteCenter = (minuteBounds.size.h) >> 1;
  minuteCenterForSecond = (GPoint){.x=minuteCenter, .y=minuteCenter};
  secondHalfWidth = secondSize >> 1;
  
  

  tick_timer_service_subscribe(MINUTE_UNIT|HOUR_UNIT|SECOND_UNIT,tickHandler);
  time_t clock = time(NULL);
  struct tm *time = localtime(&clock);
  tickHandler(time,MINUTE_UNIT|HOUR_UNIT);
}

static void window_unload(Window *window)
{
  tick_timer_service_unsubscribe();
  layer_destroy(faceLayer);
  layer_destroy(hourLayer);
  layer_destroy(minuteLayer);
}

static void init(void)
{
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers)
  {
    .load = window_load,
    .unload = window_unload,
  });
  window_set_background_color(window, GColorBlack);
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void)
{
  window_destroy(window);
}

int main(void)
{
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}

static void circleLayerUpdate(Layer *layer, GContext *context)
{
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(context, GColorWhite);
  uint16_t radius = bounds.size.w >> 1;
  GPoint center = (GPoint){.x = radius,.y=radius};
  graphics_fill_circle(context, center, radius);
}

static void circleLayerUpdateAlt(Layer *layer, GContext *context)
{
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(context, GColorBlack);
  uint16_t radius = bounds.size.w >> 1;
  GPoint center = (GPoint){.x = radius,.y=radius};
  graphics_fill_circle(context, center, radius);
}

static void tickHandler(struct tm *tick_time, TimeUnits units_changed)
{
  if ((units_changed & MINUTE_UNIT) == MINUTE_UNIT)
  {
  	// update hour...
  	int32_t hourAngle = (tick_time->tm_hour * TRIG_MAX_ANGLE / 12) + (tick_time->tm_min * TRIG_MAX_ANGLE / 60)/12;
    GPoint hourPosition;
    hourPosition.x = faceCenterForHour.x + sin_lookup(hourAngle) * hourLength / TRIG_MAX_RATIO - hourHalfWidth;
    hourPosition.y = faceCenterForHour.y - cos_lookup(hourAngle) * hourLength / TRIG_MAX_RATIO - hourHalfWidth;
    GRect hourFrame = layer_get_frame(hourLayer);
    hourFrame.origin = hourPosition;
    layer_set_frame(hourLayer, hourFrame);
  
  
  
  	// update minute
    int32_t minuteAngle = tick_time->tm_min * TRIG_MAX_ANGLE / 60;
    GPoint minutePosition;
    minutePosition.x = hourCenterForMinute.x + sin_lookup(minuteAngle) * minuteLength / TRIG_MAX_RATIO - minuteHalfWidth;
    minutePosition.y = hourCenterForMinute.y - cos_lookup(minuteAngle) * minuteLength / TRIG_MAX_RATIO - minuteHalfWidth;
    GRect minuteFrame = layer_get_frame(minuteLayer);
    minuteFrame.origin = minutePosition;
    layer_set_frame(minuteLayer, minuteFrame);
    
    
    
  }
  if ((units_changed & SECOND_UNIT) == SECOND_UNIT){
  	// update minute
    int32_t secondAngle = tick_time->tm_sec * TRIG_MAX_ANGLE / 60;
    GPoint secondPosition;
    secondPosition.x = minuteCenterForSecond.x + sin_lookup(secondAngle) * secondLength / TRIG_MAX_RATIO - secondHalfWidth;
    secondPosition.y = minuteCenterForSecond.y - cos_lookup(secondAngle) * secondLength / TRIG_MAX_RATIO - secondHalfWidth;
    GRect secondFrame = layer_get_frame(secondLayer);
    secondFrame.origin = secondPosition;
    layer_set_frame(secondLayer, secondFrame);
    
  }
}
