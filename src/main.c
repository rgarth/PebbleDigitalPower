#include <pebble.h>

static Window *s_main_window; 
static TextLayer *s_time_layer, *s_date_layer;
static BitmapLayer *s_bluetooth_layer;
static GBitmap *s_bluetooth_bmap;

static void show_time() {
  #ifdef PBL_COLOR
  int8_t charge;
  GColor my_color;
  charge = battery_state_service_peek().charge_percent;
  if ( charge > 65) {
    my_color = GColorBrightGreen;
  } else if (charge > 32) {
    my_color = GColorChromeYellow;
  } else {
    my_color = GColorRed;
  }
  text_layer_set_text_color(s_time_layer, my_color);
  text_layer_set_text_color(s_date_layer, my_color);
  bitmap_layer_set_background_color(s_bluetooth_layer, my_color);
  #endif
  
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char t_buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(t_buffer, sizeof(t_buffer), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(t_buffer, sizeof(t_buffer), "%l:%M", tick_time);
  }  
  text_layer_set_text(s_time_layer, t_buffer);
  
  static char d_buffer[7];
  strftime(d_buffer, sizeof(d_buffer), "%a %d", tick_time);
  text_layer_set_text(s_date_layer, d_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  show_time();
}

static void bt_handler(bool connected) {
  if (connected) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Phone has connected!");
    vibes_short_pulse();
    layer_set_hidden(bitmap_layer_get_layer(s_bluetooth_layer), 0);
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "Phone has disconnected!");
    vibes_double_pulse();
    layer_set_hidden(bitmap_layer_get_layer(s_bluetooth_layer), 1);
  }
  
}

static void main_window_load(Window *window) {
  GFont time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TICKING_TIME_BOME_52));
  GFont date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TICKING_TIME_BOME_18));

  // Create TextLayers for time
  s_time_layer = text_layer_create(GRect(0, 64, 144, 54));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_text(s_time_layer, "00:00");
  
  GSize time_size = text_layer_get_content_size(s_time_layer);
  int date_width = 144 - ((144 - time_size.w) / 2);
  
  s_date_layer = text_layer_create(GRect(0, 52, date_width, 38));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, date_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);

  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer)); 
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer)); 

  // Add a bluetooth layer
  // Having composite issues on aplite, so only use icon on basalt
  #ifdef PBL_PLATFORM_BASALT
  int bluetooth_xorigin = 0 + ((144 - time_size.w) / 2);
  s_bluetooth_layer = bitmap_layer_create(GRect(bluetooth_xorigin, 54, 18, 18));
  bitmap_layer_set_background_color(s_bluetooth_layer, GColorWhite);
  bitmap_layer_set_compositing_mode(s_bluetooth_layer, GCompOpSet);
  s_bluetooth_bmap = gbitmap_create_with_resource(RESOURCE_ID_IMG_BLUETOOTH);
  bitmap_layer_set_bitmap(s_bluetooth_layer, s_bluetooth_bmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bluetooth_layer));
  if (! bluetooth_connection_service_peek()) {
    APP_LOG(APP_LOG_LEVEL_INFO, "No bluetooth");
    layer_set_hidden(bitmap_layer_get_layer(s_bluetooth_layer), 1);
  }
  #endif
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  
  #ifdef PBL_PLATFORM_BASALT
  gbitmap_destroy(s_bluetooth_bmap);
  bitmap_layer_destroy(s_bluetooth_layer);
  #endif
} 

static void init () {
  
  
  // Create main Window element and assign to pointer
  s_main_window = window_create();  
  window_set_background_color(s_main_window, GColorBlack); 

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Make sure the time is displayed from the start
  show_time();
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register with the BluetoothConnectionService
  bluetooth_connection_service_subscribe(bt_handler); 
}

static void deinit () {
  // Destroy Window
  window_destroy(s_main_window);
  bluetooth_connection_service_unsubscribe();
  tick_timer_service_unsubscribe();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}