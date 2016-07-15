#include <pebble.h>
#include "enamel.h"
#include <pebble-events/pebble-events.h>

static Layer *hands_layer;
static TextLayer *time_layer;
static Window *window;
static uint8_t color_mapping[24];
static Animation *s_animation;
static int16_t s_animation_percent;
static AppTimer *s_hide_timer;

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  static char time_text[] = "00:00";

  char *time_format;

  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }

  strftime(time_text, sizeof(time_text), time_format, tick_time);

  // Kludge to handle lack of non-padded hour format string
  // for twelve hour clock.
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }

  text_layer_set_text(time_layer, time_text);
  layer_mark_dirty(hands_layer);
}

static void hide_layer(void* data) {
  layer_set_hidden(text_layer_get_layer(time_layer),true);
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  // Process tap on ACCEL_AXIS_X, ACCEL_AXIS_Y or ACCEL_AXIS_Z
  // Direction is 1 or -1
  // blink if enabled
  layer_set_hidden(text_layer_get_layer(time_layer),false);
  s_hide_timer = app_timer_register(2000,hide_layer,NULL);
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  GColor curr_color=(GColor8){.argb=((uint8_t)(0xC0|color_mapping[t->tm_hour]))};
  GColor old_color=(GColor8){.argb=((uint8_t)(0xC0|color_mapping[(t->tm_hour-1)%24]))};

  //draw background with old color
  graphics_context_set_fill_color(ctx, old_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

#ifdef PBL_ROUND
  //draw circle with current color with a radius based on the current minute
  int radius = center.x*t->tm_min*s_animation_percent/6000;
  graphics_context_set_fill_color(ctx, curr_color);
  graphics_fill_circle(ctx,center, radius);
  if(enamel_get_border()) {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_context_set_stroke_width(ctx,2);
    graphics_draw_circle(ctx,center, radius);
  }
#else
//draw rectangle with current color with a distance to screen border based on the current minute
int inset_x = center.x-(center.x*t->tm_min*s_animation_percent/6000);
int inset_y = center.y-(center.y*t->tm_min*s_animation_percent/6000);
APP_LOG(APP_LOG_LEVEL_DEBUG,"%d %d ",inset_x,inset_y);
GRect rectangle = grect_inset(bounds,GEdgeInsets(inset_y,inset_x));
graphics_context_set_fill_color(ctx, curr_color);
graphics_fill_rect(ctx,rectangle,0,GCornerNone);
if(enamel_get_border()) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx,2);
  graphics_draw_rect(ctx,rectangle);
}
#endif
}

static void implementation_update(Animation *animation,
                                  const AnimationProgress progress) {
  // Animate some completion variable
  s_animation_percent = ((int)progress * 100) / ANIMATION_NORMALIZED_MAX;
  layer_mark_dirty(hands_layer);
}

static void settings_updated(void) {
  for(int i=0;i<24;i++) {
    if(enamel_get_colorscheme()==2) { // fancy
      if(i%2)
        color_mapping[i] = 63-i;
      else
        color_mapping[i] = i;
    } else if(enamel_get_colorscheme()==0) { // dark to light
        color_mapping[i] = i*2;
    }
    else if(enamel_get_colorscheme()==1) { // light to dark
        color_mapping[i] = 63-i*2;
    }
  }
  layer_mark_dirty(hands_layer);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // init hands
  hands_layer = layer_create(bounds);
  layer_set_update_proc(hands_layer, hands_update_proc);
  layer_add_child(window_layer, hands_layer);

  bounds.origin.y+=48;
  time_layer = text_layer_create(bounds);
  text_layer_set_text_color(time_layer,GColorWhite);
  text_layer_set_background_color(time_layer,GColorClear);
  text_layer_set_text_alignment(time_layer,GTextAlignmentCenter);
  text_layer_set_font(time_layer,fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_62)));
  text_layer_set_text(time_layer,"00:00");
  layer_add_child(window_layer,text_layer_get_layer(time_layer));
  layer_set_hidden(text_layer_get_layer(time_layer),true);
  accel_tap_service_subscribe(accel_tap_handler);

  settings_updated();
  enamel_register_settings_received(settings_updated);

  if(enamel_get_animation()) {
    // Create a new Animation
    s_animation = animation_create();
    animation_set_delay(s_animation, 100);
    animation_set_duration(s_animation, 500);

    // Create the AnimationImplementation
    static const AnimationImplementation implementation = {
      .update = implementation_update
    };
    animation_set_implementation(s_animation, &implementation);
  }
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  handle_tick(t, MINUTE_UNIT);

  events_tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
  animation_schedule(s_animation);
}

static void window_unload(Window *window) {
  layer_destroy(hands_layer);
  //tick_timer_service_unsubscribe();
}

static void init(void) {
  // Initialize Enamel to register App Message handlers and restores settings
  enamel_init();

  // call pebble-events app_message_open function
  events_app_message_open();

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
    // Push the window onto the stack
  const bool animated = true;
  window_stack_push(window, animated);

}

static void deinit(void) {
  // Deinit Enamel to unregister App Message handlers and save settings
  enamel_deinit();
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
