#include <pebble.h>

static Window *window;
TextLayer *text_date_layer;
TextLayer *text_time_layer;
TextLayer *text_temp_layer;
TextLayer *text_batt_layer;
GFont *font49;
GFont *font39;
GFont *font21;

#define NUMBER_OF_IMAGES 11
static GBitmap *image = NULL;
static BitmapLayer *image_layer;

const int IMAGE_RESOURCE_IDS[NUMBER_OF_IMAGES] = {
  RESOURCE_ID_CLEAR_DAY,
  RESOURCE_ID_CLEAR_NIGHT,
  RESOURCE_ID_CLOUDY,
  RESOURCE_ID_FOG,
  RESOURCE_ID_PARTLY_CLOUDY_DAY,
  RESOURCE_ID_PARTLY_CLOUDY_NIGHT,
  RESOURCE_ID_RAIN,
  RESOURCE_ID_SLEET,
  RESOURCE_ID_SNOW,
  RESOURCE_ID_WIND,
  RESOURCE_ID_ERROR
};

enum {
  WEATHER_TEMPERATURE_F,
  WEATHER_TEMPERATURE_C,
  WEATHER_ICON,
  WEATHER_ERROR,
  LOCATION
};

int FtoC(int f) {
  return (f - 32) * 5 / 9;
}

void in_received_handler(DictionaryIterator *received, void *context) {
  // incoming message received
  Tuple *temperature = dict_find(received,WEATHER_TEMPERATURE_C);
  Tuple *icon = dict_find(received, WEATHER_ICON);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Loop index now %s", temperature->value->cstring);
  if (temperature) {
    text_layer_set_text(text_temp_layer, strcat(temperature->value->cstring, "º"));
  }

  if (icon) {
    // figure out which resource to use
    int8_t id = icon->value->int8;
    if (image != NULL) {
      gbitmap_destroy(image);
      layer_remove_from_parent(bitmap_layer_get_layer(image_layer));
      bitmap_layer_destroy(image_layer);
    }

    Layer *window_layer = window_get_root_layer(window);

    image = gbitmap_create_with_resource(IMAGE_RESOURCE_IDS[id]);
    image_layer = bitmap_layer_create(GRect(10, 100, 60, 60));
    bitmap_layer_set_bitmap(image_layer, image);
    layer_add_child(window_layer, bitmap_layer_get_layer(image_layer));
  }
}

void handle_battery(BatteryChargeState charge) {
    static char buf[] = "123";
    snprintf(buf, sizeof(buf), "%d", charge.charge_percent);
    text_layer_set_text(text_batt_layer, strcat(buf, "%"));
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  font49 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HELVETICA_ULTRA_LIGH_49));
  font39 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HELVETICA_ULTRA_LIGH_39));
  font21 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HELVETICA_ULTRA_LIGH_21));

  // create time layer - this is where time goes
  text_time_layer = text_layer_create(GRect(8, 40, 144-7, 168-92));
  text_layer_set_text_color(text_time_layer, GColorWhite);
  text_layer_set_background_color(text_time_layer, GColorClear);
  text_layer_set_font(text_time_layer, font49);
  layer_add_child(window_layer, text_layer_get_layer(text_time_layer));

  // create date layer - this is where the date goes
  text_date_layer = text_layer_create(GRect(8, 10, 144-10, 24));
  text_layer_set_text_alignment(text_date_layer, GTextAlignmentRight);
  text_layer_set_text_color(text_date_layer, GColorWhite);
  text_layer_set_background_color(text_date_layer, GColorClear);
  text_layer_set_font(text_date_layer, font21);
  layer_add_child(window_layer, text_layer_get_layer(text_date_layer));
    
  // create batery layer
  text_batt_layer = text_layer_create(GRect(10, 10, 144-10, 24));
  text_layer_set_text_alignment(text_batt_layer, GTextAlignmentLeft);
  text_layer_set_text_color(text_batt_layer, GColorWhite);
  text_layer_set_background_color(text_batt_layer, GColorClear);
  text_layer_set_font(text_batt_layer, font21);
  layer_add_child(window_layer, text_layer_get_layer(text_batt_layer));

  // create temperature layer - this is where the temperature goes
  text_temp_layer = text_layer_create(GRect(80, 108, 144-80, 168-108));
  text_layer_set_text_color(text_temp_layer, GColorWhite);
  text_layer_set_background_color(text_temp_layer, GColorClear);
  text_layer_set_font(text_temp_layer, font39);
  layer_add_child(window_layer, text_layer_get_layer(text_temp_layer));
    
  handle_battery(battery_state_service_peek());
  battery_state_service_subscribe(&handle_battery);
}

static void window_unload(Window *window) {
  // destroy the text layers - this is good
  text_layer_destroy(text_date_layer);
  text_layer_destroy(text_time_layer);
  text_layer_destroy(text_temp_layer);
  text_layer_destroy(text_batt_layer);

  // destroy the image layers
  gbitmap_destroy(image);
  layer_remove_from_parent(bitmap_layer_get_layer(image_layer));
  bitmap_layer_destroy(image_layer);

  // unload the fonts
  fonts_unload_custom_font(font49);
  fonts_unload_custom_font(font39);
  fonts_unload_custom_font(font21);
    
  battery_state_service_unsubscribe();
}

static void app_message_init(void) {
  app_message_open(64, 16);
  app_message_register_inbox_received(in_received_handler);
}

// show the date and time every minute
void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Need to be static because they're used by the system later.
  static char time_text[] = "00:00";
  static char date_text[] = "Xxxxxxxxx 00";

  char *time_format;

  strftime(date_text, sizeof(date_text), "%B %e", tick_time);
  text_layer_set_text(text_date_layer, date_text);


  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }

  strftime(time_text, sizeof(time_text), time_format, tick_time);

  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }

  text_layer_set_text(text_time_layer, time_text);
}

static void init(void) {
  window = window_create();
  app_message_init();

  window_set_background_color(window, GColorBlack);

  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);

  // subscribe to update every minute
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
    
  
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  app_message_deregister_callbacks();

  window_stack_remove(window, true);

  window_destroy(window);
}

int main(void) {
  init();

  app_event_loop();
  deinit();
}
