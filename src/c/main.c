#include <pebble.h>

static Window *s_main_window; 
static TextLayer *s_time_layer, *s_date_layer;
static BitmapLayer *s_bluetooth_layer;
static GBitmap *s_bluetooth_bmap;
// BrightGreen, 0x55FF00
int color = 0x55FF00;
bool battery = 1;
GFont time_font, date_font;

#define KEY_BATTERY 0
#define KEY_COLOR 1

static void show_time() {
  GColor my_color;
  if (battery) {
    int charge;
    charge = battery_state_service_peek().charge_percent;
    if ( charge > 50) {
      my_color = GColorBrightGreen;
    } else if (charge > 20) {
      my_color = GColorYellow;
    } else {
      my_color = GColorRed;
    }
  } else {
    my_color = GColorFromHEX(color);  
  }
  text_layer_set_text_color(s_time_layer, my_color);
  text_layer_set_text_color(s_date_layer, my_color);
  bitmap_layer_set_background_color(s_bluetooth_layer, my_color);
  
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

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");

  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_BATTERY:
      battery = t->value->int32;
      APP_LOG(APP_LOG_LEVEL_INFO, "Saving show charge: %i", battery);
      persist_write_bool(KEY_BATTERY, battery);
      break;
    case KEY_COLOR:
      // color returned as a hex string
      if (t->value->int32 > 0) {
        color = t->value->int32;
        APP_LOG(APP_LOG_LEVEL_INFO, "Saving color: %x", color);
        persist_write_int(KEY_COLOR, color);
      }
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }   
    // Look for next item
    t = dict_read_next(iterator);
  }
  show_time();
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
} 

static void main_window_load(Window *window) {
  time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_SEVEN_62));
  date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_SEVEN_24));

  // Create TextLayers for time
  s_time_layer = text_layer_create(GRect(0, 48, 142, 64));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);
  text_layer_set_text(s_time_layer, "00:00");
  
  s_date_layer = text_layer_create(GRect(0, 32, 142, 26));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, date_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);

  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer)); 
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer)); 

  // Add a bluetooth layer
  // Having composite issues on aplite, so only use icon on basalt
  s_bluetooth_layer = bitmap_layer_create(GRect(2, 38, 18, 18));
  bitmap_layer_set_background_color(s_bluetooth_layer, GColorWhite);
  bitmap_layer_set_compositing_mode(s_bluetooth_layer, GCompOpSet);
  s_bluetooth_bmap = gbitmap_create_with_resource(RESOURCE_ID_IMG_BLUETOOTH);
  bitmap_layer_set_bitmap(s_bluetooth_layer, s_bluetooth_bmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bluetooth_layer));
  if (! bluetooth_connection_service_peek()) {
    APP_LOG(APP_LOG_LEVEL_INFO, "No bluetooth");
    layer_set_hidden(bitmap_layer_get_layer(s_bluetooth_layer), 1);
  }
}

static void main_window_unload(Window *window) {
  
  fonts_unload_custom_font(time_font);
  fonts_unload_custom_font(date_font);
  
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  
  gbitmap_destroy(s_bluetooth_bmap);
  bitmap_layer_destroy(s_bluetooth_layer);

} 

static void init () {
  
  if (persist_exists(KEY_BATTERY)) {
    battery = persist_read_bool(KEY_BATTERY);
    APP_LOG(APP_LOG_LEVEL_INFO, "Reading show charge: %d", battery);
  }
  if (persist_exists(KEY_COLOR)) {
    color = persist_read_int(KEY_COLOR);
    APP_LOG(APP_LOG_LEVEL_INFO, "Reading color: %x", color);
  }
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
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