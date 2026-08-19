#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_text_layer;

// State
static AppTimer *s_light_show_timer;
static bool bl_enabled;

#if defined(PBL_PLATFORM_FLINT)
static bool bl_toggle;
#endif

static uint16_t blink_interval = 0;
static GColor random_color = GColorWhite;
static time_t last_timestamp=0;
static int16_t last_millis=0;
static char text_BPM[16]; // "BPM: 1000"
static const char string_interval[] = "interval: %d ms";
static char text_interval[20]; // "Interval: 9000ms"


GColor get_new_random_color(GColor oldColor){
  GColor new_random_color;
  do {
    new_random_color = GColorFromRGB(rand() % 2 * 255, rand() % 2 * 255, rand() % 2 * 255);
    if(gcolor_equal(new_random_color,GColorFromRGB(0,0,0))) {
      /* Replacing black with orange */
      new_random_color = GColorFromRGB(255,127,0);
    } else if (gcolor_equal(new_random_color,GColorFromRGB(255,255,255))){
      /* Replacing white with a less blue white */
      new_random_color = GColorFromRGB(255,205,128);
    }
  } while(gcolor_equal(oldColor,new_random_color));
  return new_random_color;
}

static void light_show_callback(void *data) {
  
  #if defined(PBL_PLATFORM_FLINT)
  bl_toggle = !bl_toggle;
  
  GCompOp mode = bl_toggle ? GCompOpAssignInverted : GCompOpAssign; 
  if (bl_enabled) {
    light_enable_interaction();
  }

  #elif defined(PBL_PLATFORM_EMERY) 
  random_color = get_new_random_color(random_color);
  if (bl_enabled) {
    light_set_color(random_color);
    light_enable_interaction();
  } else {
    /* TODO: fallback to changing the background color if the backlight is off */
    bl_enabled = light_is_on();
  }
  #endif
  
  if(blink_interval) {
    s_light_show_timer = app_timer_register(blink_interval, light_show_callback, NULL);
  }
}
void make_interval_text(uint16_t val) {
  snprintf(text_interval, sizeof(text_interval), string_interval, (unsigned)val);
}

void make_BPM_text(uint16_t interval) {
  if (interval > 0) {
    uint16_t bpm_val = 60000 / interval;
    snprintf(text_BPM, sizeof(text_BPM), "BPM: %d", (unsigned)bpm_val);
  } else {
    snprintf(text_BPM, sizeof(text_BPM), "Invalid BPM");
  }  
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  time_t time_now;
  uint16_t millis_now = time_ms(&time_now,NULL);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "time_now: %d", time_now);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "millis_now: %d", millis_now);
  if(!last_timestamp){
    last_timestamp=time_now;
    millis_now=last_millis;
  }
  unsigned int delta_time = time_now - last_timestamp;
  int16_t delta_millis = millis_now - last_millis;
  if(delta_millis<0){
    delta_millis+=1000;
  }
  delta_time=delta_time*1000+delta_millis;
  APP_LOG(APP_LOG_LEVEL_INFO, "4 beat tempo ms: %d", delta_time);
  if (delta_time > 0 && delta_time < 5000) {
    blink_interval = (uint16_t)delta_time>>2;  
  }
  last_timestamp=time_now;
  last_millis=millis_now;
  
  if (s_light_show_timer) {
      app_timer_cancel(s_light_show_timer);
      s_light_show_timer = NULL;
  }
  if (blink_interval > 0) {
    s_light_show_timer = app_timer_register(blink_interval, light_show_callback, NULL);
    //text_layer_set_text(s_text_layer, "Taped 4 beat tempo:");
    //make_interval_text(blink_interval);
    //text_layer_set_text(s_text_layer, text_interval);
    make_BPM_text(blink_interval);
    text_layer_set_text(s_text_layer, text_BPM);
  }

}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  //text_layer_set_text(s_text_layer, "Increase Tempo");
  if(blink_interval == 0){
    blink_interval=500;
    s_light_show_timer = app_timer_register(blink_interval, light_show_callback, NULL);
  }else if(blink_interval > 10){
    blink_interval-=2;
  }
  make_BPM_text(blink_interval);
  text_layer_set_text(s_text_layer, text_BPM);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "interval ms: %d", blink_interval);  
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  //text_layer_set_text(s_text_layer, "Decrease Tempo");
  if(blink_interval == 0){
    blink_interval=500;
    s_light_show_timer = app_timer_register(blink_interval, light_show_callback, NULL);
  }else if(blink_interval < 8000){
    blink_interval+=2;
  }
  make_BPM_text(blink_interval);
  text_layer_set_text(s_text_layer, text_BPM);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "interval ms: %d", blink_interval);
  
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_click_handler);
  
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_text_layer = text_layer_create(GRect(0, 72, bounds.size.w, 20));
  text_layer_set_text(s_text_layer, "click 4 beat tempo!");
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
  if (s_light_show_timer) {
      app_timer_cancel(s_light_show_timer);
      s_light_show_timer = NULL;
      APP_LOG(APP_LOG_LEVEL_INFO, "Stop s_light_show_timer", NULL);
  }
}

static void init(void) {
  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
}

static void deinit(void) {
  
  window_destroy(s_main_window);
  if (s_light_show_timer) {
      app_timer_cancel(s_light_show_timer);
      s_light_show_timer = NULL;
      APP_LOG(APP_LOG_LEVEL_INFO, "Stop s_light_show_timer", NULL);
  }
}


int main(void) {
  init();
  s_light_show_timer = NULL;
  app_event_loop();
  deinit();
}
